# SPDX-License-Identifier: GPL-2.0+
#
# Copyright 2025 Canonical Ltd.
# Written by Simon Glass <simon.glass@canonical.com>
#
# pylint: disable=too-many-lines
"""Control module for pickman - handles the main logic."""

from collections import namedtuple
from datetime import date
import os
import re
import sys
import tempfile
import time
import unittest

# Allow 'from pickman import xxx' to work via symlink
our_path = os.path.dirname(os.path.realpath(__file__))
sys.path.insert(0, os.path.join(our_path, '..'))

# pylint: disable=wrong-import-position,import-error
from pickman import agent
from pickman import database
from pickman import ftest
from pickman import gitlab_api
from u_boot_pylib import command
from u_boot_pylib import terminal
from u_boot_pylib import tout

# Default database filename
DB_FNAME = '.pickman.db'

# Branch names to compare
BRANCH_MASTER = 'ci/master'
BRANCH_SOURCE = 'us/next'

# Git stat output parsing patterns
RE_GIT_STAT_SUMMARY = re.compile(
    r'(\d+)\s+files?\s+changed'
    r'(?:,\s*(\d+)\s+insertions?\([+]\))?'
    r'(?:,\s*(\d+)\s+deletions?\([-]\))?'
)
RE_GIT_STAT_FILE = re.compile(r'^([^|]+)\s*\|')

# Extract hash from line like "(cherry picked from commit abc123def)"
RE_CHERRY_PICK = re.compile(r'cherry picked from commit ([a-f0-9]+)')

# Named tuple for commit info
Commit = namedtuple('Commit', ['hash', 'chash', 'subject', 'date'])

# Named tuple for git stat output
# files: Number of files changed
# inserted: Number of lines inserted
# deleted: Number of lines deleted
# file_set: Set of modified file paths
GitStat = namedtuple('GitStat', ['files', 'inserted', 'deleted', 'file_set'])

# Named tuple for check results
# chash: Cherry-pick commit hash (full)
# orig_hash: Original commit hash that was cherry-picked
# subject: Commit subject line
# delta_ratio: Ratio of differences between original and cherry-pick
#   (0.0=identical, 1.0=completely different)
# orig_stats: Stats from original commit (files, insertions, deletions,
#   file_set)
# cherry_stats: Stats from cherry-pick commit
# reason: Reason for skipping analysis (None if analyzed)
CheckResult = namedtuple('CheckResult', [
    'chash', 'orig_hash', 'subject', 'delta_ratio',
    'orig_stats', 'cherry_stats', 'reason'
])

# Named tuple for commit with author
# hash: Full SHA-1 commit hash (40 characters)
# chash: Abbreviated commit hash (typically 7-8 characters)
# subject: First line of commit message (commit subject)
# author: Commit author name and email in format "Name <email>"
CommitInfo = namedtuple('CommitInfo',
                        ['hash', 'chash', 'subject', 'author'])

# Named tuple for simplified commit data passed to agent
# hash: Full SHA-1 commit hash (40 characters)
# chash: Abbreviated commit hash (typically 7-8 characters)
# subject: First line of commit message (commit subject)
# applied_as: Short hash if potentially already applied, None otherwise
AgentCommit = namedtuple('AgentCommit',
                         ['hash', 'chash', 'subject', 'applied_as'])

# Named tuple for get_next_commits() result
#
# commits: list of CommitInfo to cherry-pick
# merge_found: True if these commits came from a merge on the source branch
# advance_to: hash to advance the source position to, or None to stay put
NextCommitsInfo = namedtuple('NextCommitsInfo',
                             ['commits', 'merge_found', 'advance_to'])

# Named tuple for prepare_apply() result
#
# commits: list of AgentCommit to cherry-pick
# branch_name: name of the branch to create for the MR
# original_branch: branch name before any conflict suffix
# merge_found: True if these commits came from a merge on the source branch
# advance_to: hash to advance the source position to, or None to stay put
ApplyInfo = namedtuple('ApplyInfo',
                       ['commits', 'branch_name', 'original_branch',
                        'merge_found', 'advance_to'])


def parse_log_output(log_output, has_parents=False):
    """Parse git log output to extract CommitInfo tuples

    Args:
        log_output (str): Output from git log with format '%H|%h|%an|%s'
            or '%H|%h|%an|%s|%P' if has_parents is True
        has_parents (bool): If True, expects parents field at end and
            excludes it from subject parsing

    Returns:
        list: List of CommitInfo tuples
    """
    commits = []
    for line in log_output.split('\n'):
        if not line:
            continue
        parts = line.split('|')
        commit_hash = parts[0]
        chash = parts[1]
        author = parts[2]
        if has_parents:
            subject = '|'.join(parts[3:-1])
        else:
            subject = '|'.join(parts[3:])
        commits.append(CommitInfo(commit_hash, chash, subject, author))
    return commits


def run_git(args):
    """Run a git command and return output."""
    return command.output('git', *args).strip()


def compare_branches(master, source):
    """Compare two branches and return commit difference info.

    Args:
        master (str): Main branch to compare against
        source (str): Source branch to check for unique commits

    Returns:
        tuple: (count, Commit) where count is number of commits and Commit
            is the last common commit
    """
    # Find commits in source that are not in master
    count = int(run_git(['rev-list', '--count', f'{master}..{source}']))

    # Find the merge base (last common commit)
    base = run_git(['merge-base', master, source])

    # Get details about the merge-base commit
    info = run_git(['log', '-1', '--format=%H%n%h%n%s%n%ci', base])
    full_hash, chash, subject, commit_date = info.split('\n')

    return count, Commit(full_hash, chash, subject, commit_date)


def do_add_source(args, dbs):
    """Add a source branch to the database

    Finds the merge-base commit between master and source and stores it.

    Args:
        args (Namespace): Parsed arguments with 'source' attribute
        dbs (Database): Database instance

    Returns:
        int: 0 on success
    """
    source = args.source

    # Find the merge base commit
    base_hash = run_git(['merge-base', BRANCH_MASTER, source])

    # Get commit details for display
    info = run_git(['log', '-1', '--format=%h%n%s', base_hash])
    chash, subject = info.split('\n')

    # Store in database
    dbs.source_set(source, base_hash)
    dbs.commit()

    tout.info(f"Added source '{source}' with base commit:")
    tout.info(f'  Hash:    {chash}')
    tout.info(f'  Subject: {subject}')

    return 0


def do_list_sources(args, dbs):  # pylint: disable=unused-argument
    """List all tracked source branches

    Args:
        args (Namespace): Parsed arguments
        dbs (Database): Database instance

    Returns:
        int: 0 on success
    """
    sources = dbs.source_get_all()

    if not sources:
        tout.info('No source branches tracked')
    else:
        tout.info('Tracked source branches:')
        for name, last_commit in sources:
            tout.info(f'  {name}: {last_commit[:12]}')

    return 0


def do_compare(args, dbs):  # pylint: disable=unused-argument
    """Compare branches and print results.

    Args:
        args (Namespace): Parsed arguments
        dbs (Database): Database instance
    """
    count, base = compare_branches(BRANCH_MASTER, BRANCH_SOURCE)

    tout.info(f'Commits in {BRANCH_SOURCE} not in {BRANCH_MASTER}: {count}')
    tout.info('')
    tout.info('Last common commit:')
    tout.info(f'  Hash:    {base.chash}')
    tout.info(f'  Subject: {base.subject}')
    tout.info(f'  Date:    {base.date}')

    return 0


def parse_git_stat_output(stat_output):
    """Parse git show --stat output to extract file change statistics

    Args:
        stat_output (str): Output from 'git show --stat <hash>'

    Returns:
        GitStat: Named tuple with files, insertions, deletions, file_set
    """
    lines = stat_output.strip().split('\n')
    files_changed = 0
    insertions = 0
    deletions = 0
    changed_files = set()

    # Parse summary line: "5 files changed, 42 insertions(+), 13 deletions(-)"
    for line in lines:
        match = RE_GIT_STAT_SUMMARY.search(line)
        if match:
            files_changed = int(match.group(1))
            insertions = int(match.group(2)) if match.group(2) else 0
            deletions = int(match.group(3)) if match.group(3) else 0
            break

    # Parse individual file lines: "path/to/file.ext | 42 ++++----"
    for line in lines:
        match = RE_GIT_STAT_FILE.match(line)
        if match:
            filename = match.group(1).strip()
            if filename:
                changed_files.add(filename)

    return GitStat(files_changed, insertions, deletions, changed_files)


def calc_ratio(orig, cherry):
    """Get the ratio of differences between original and cherry-picked commits

    Args:
        orig (GitStat): Stats for original commit
        cherry (GitStat): Stats for cherry-pick commit

    Returns:
        float: Delta ratio (0.0 = identical, 1.0 = completely
            different)
    """
    # If both commits have no changes, they're identical
    if not (orig.inserted + orig.deleted) and not (cherry.inserted +
                                                    cherry.deleted):
        return 0.0

    # Calculate file set difference
    if orig.file_set or cherry.file_set:
        union = orig.file_set | cherry.file_set
        intersection = orig.file_set & cherry.file_set
        similarity = (len(intersection) / len(union) if union else 1.0)
    else:
        similarity = 1.0

    # Calculate line change difference
    orig_lines = orig.inserted + orig.deleted
    cherry_lines = cherry.inserted + cherry.deleted

    if not orig_lines and not cherry_lines:
        line_similarity = 1.0
    elif not orig_lines or not cherry_lines:
        line_similarity = 0.0
    else:
        line_ratio = (min(orig_lines, cherry_lines) /
                      max(orig_lines, cherry_lines))
        line_similarity = line_ratio

    # Overall similarity is the minimum of file and line similarity
    overall_similarity = min(similarity, line_similarity)

    # Delta ratio is 1 - similarity
    return 1.0 - overall_similarity


def get_orig_commit(cherry_commit_hash):
    """Find the original commit hash from a cherry-pick commit

    Args:
        cherry_commit_hash (str): Hash of the cherry-picked commit

    Returns:
        str: Original commit hash, or None if not found
    """
    try:
        # Get the commit message
        commit_msg = run_git(['log', '-1', '--format=%B', cherry_commit_hash])

        # Look for "(cherry picked from commit <hash>)" line
        for line in commit_msg.split('\n'):
            if 'cherry picked from commit' in line:
                match = RE_CHERRY_PICK.search(line)
                if match:
                    return match.group(1)

        return None
    except Exception:  # pylint: disable=broad-except
        return None


def check_commits(commits, min_lines):
    """Yield CheckResult entries for commits with delta analysis

    Args:
        commits (list): List of (commit_hash, chash, subject) tuples
        min_lines (int): Minimum lines changed to analyze

    Yields:
        CheckResult: Analysis result for each commit
    """
    for chash, _, subject in commits:
        # Skip merge commits
        is_merge = False
        try:
            parents = run_git(['log', '-1', '--format=%P', chash]).split()
            if len(parents) > 1:
                is_merge = True
        except Exception:  # pylint: disable=broad-except
            pass

        # Also check subject for merge indicators
        if not is_merge and (subject.startswith('Merge ') or
                             'Merge branch' in subject or
                             'Merge tag' in subject):
            is_merge = True

        if is_merge:
            yield CheckResult(
                chash, None, subject, 0.0,
                None, None, 'merge_commit'
            )
            continue

        # Find original commit
        orig_hash = get_orig_commit(chash)
        if not orig_hash:
            yield CheckResult(
                chash, None, subject, 0.0,
                None, None, 'not_cherry_pick'
            )
            continue

        # Get stats for both commits
        orig_stat = run_git(['show', '--stat', orig_hash])
        cherry_stat = run_git(['show', '--stat', chash])

        # Parse statistics
        orig_stats = parse_git_stat_output(orig_stat)
        cherry_stats = parse_git_stat_output(cherry_stat)

        # Skip small commits
        orig_total_lines = orig_stats.inserted + orig_stats.deleted
        cherry_total_lines = cherry_stats.inserted + cherry_stats.deleted
        max_lines = max(orig_total_lines, cherry_total_lines)

        if max_lines < min_lines:
            yield CheckResult(
                chash, orig_hash, subject, 0.0,
                orig_stats, cherry_stats, f'small_commit_{max_lines}_lines'
            )
            continue

        # Calculate delta ratio
        delta_ratio = calc_ratio(orig_stats, cherry_stats)

        yield CheckResult(
            chash, orig_hash, subject, delta_ratio,
            orig_stats, cherry_stats, None
        )


def check_verbose(result, threshold):
    """Print verbose output for a single check result

    Args:
        result (CheckResult): The check result to print
        threshold (float): Delta threshold for highlighting problems
    """
    chash_short = result.chash[:10]

    if result.reason:
        if result.reason == 'merge_commit':
            tout.info(f'{chash_short}: {result.subject}')
            tout.info('  → Skipped (merge commit)')
            tout.info('')
        elif result.reason == 'not_cherry_pick':
            tout.info(f'{chash_short}: {result.subject}')
            tout.info('  → Not a cherry-pick (no original commit found)')
            tout.info('')
        elif result.reason.startswith('small_commit'):
            lines = result.reason.split('_')[2]
            tout.info(f'{chash_short}: {result.subject}')
            tout.info(f'  → Skipped (only {lines} lines changed)')
            tout.info('')
        elif result.reason.startswith('error'):
            error = result.reason[6:]  # Remove 'error_' prefix
            tout.info(f'{chash_short}: {result.subject}')
            tout.info(f'  → Error checking delta: {error}')
            tout.info('')
    else:
        # Valid result with analysis
        tout.info(f'{chash_short}: {result.subject}')
        tout.info(f'  → Original: {result.orig_hash[:12]} '
                  f'({result.orig_stats.files} files, '
                  f'{result.orig_stats.inserted}+/'
                  f'{result.orig_stats.deleted}- lines)')
        tout.info(f'  → Cherry-pick: {result.cherry_stats.files} files, '
                  f'{result.cherry_stats.inserted}+/'
                  f'{result.cherry_stats.deleted}- lines')
        if result.delta_ratio > threshold:
            tout.info(f'  → Delta ratio: {result.delta_ratio:.1%} '
                      f'⚠️  LARGE DELTA!')
        else:
            tout.info(f'  → Delta ratio: {result.delta_ratio:.1%} ✓')
        tout.info('')


def print_check_header():
    """Print the standard header for check output table"""
    header = (f'{"Cherry-pick":<11} {"Delta%":>6} '
              f'{"Original":<10} Subject')
    dashes = f'{"-" * 11} {"-" * 6} {"-" * 10} -------'
    tout.info(header)
    tout.info(dashes)


def format_problem_commit(result, threshold):
    """Format a problematic commit in the standard table format

    Args:
        result (CheckResult): The check result to format
        threshold (float): Delta threshold for coloring

    Returns:
        str: Formatted commit line
    """
    delta_pct_val = result.delta_ratio * 100
    delta_pct = f'{delta_pct_val:.0f}'
    pct_field = f'{delta_pct:>6}'

    # Apply color
    col = terminal.Color()
    threshold_pct = threshold * 100
    if delta_pct_val >= 50:
        pct_field = col.build(terminal.Color.RED, pct_field)
    elif delta_pct_val >= threshold_pct:
        pct_field = col.build(terminal.Color.YELLOW, pct_field)

    return (f'{result.chash[:10]}  {pct_field} '
            f'{result.orig_hash[:10]} {result.subject}')


def get_branch_commits():
    """Get commits on current branch that differ from ci/master

    Returns:
        tuple: (current_branch, commits) where commits is a list of
            (full_hash, short_hash, subject) tuples
    """
    current_branch = run_git(['rev-parse', '--abbrev-ref', 'HEAD'])

    # Get all commits on current branch that aren't in ci/master
    commit_list = run_git(['log', '--reverse', '--format=%H|%h|%s',
                          f'{BRANCH_MASTER}..HEAD'])

    if not commit_list:
        return current_branch, []

    # Parse commit_list format: "full_hash|short_hash|subject" per line
    commits = []
    for line in commit_list.split('\n'):
        if line:
            parts = line.split('|', 2)
            commits.append((parts[0], parts[1], parts[2]))

    return current_branch, commits


def check_already_applied(commits, target_branch='ci/master'):
    """Check which commits are already applied to the target branch

    Args:
        commits (list): List of CommitInfo tuples to check
        target_branch (str): Branch to check against (default: ci/master)

    Returns:
        tuple: (new_commits, applied) where:
            new_commits: list of CommitInfo for commits not yet applied
            applied: list of CommitInfo for commits already applied
    """
    new_commits = []
    applied = []

    for commit in commits:
        # Check if a commit with the same subject exists in target branch
        try:
            # Use git log with --grep to search for the subject
            # Escape any special characters in the subject for grep
            escaped_subject = commit.subject.replace('"', '\\"')
            result = run_git(['log', '--oneline', target_branch,
                             f'--grep={escaped_subject}', '-1'])
            if result.strip():
                # Found a commit with the same subject
                applied.append(commit)
                tout.info(f'Skipping {commit.chash} (already applied): '
                         f'{commit.subject}')
            else:
                new_commits.append(commit)
        except Exception:  # pylint: disable=broad-except
            # If grep fails, assume the commit is not applied
            new_commits.append(commit)

    return new_commits, applied


def build_applied_map(commits):
    """Build a mapping of commit hashes to their applied counterparts

    Checks which commits have already been applied to the target branch
    and returns a dict mapping original hashes to the applied hashes.

    Args:
        commits (list): List of CommitInfo tuples to check

    Returns:
        dict: Mapping of original commit hash to applied commit hash
    """
    _, applied = check_already_applied(commits)

    applied_map = {}
    if applied:
        for c in applied:
            escaped_subject = c.subject.replace('"', '\\"')
            result = run_git(['log', '--oneline', 'ci/master',
                             f'--grep={escaped_subject}', '-1'])
            if result.strip():
                applied_hash = result.split()[0]
                applied_map[c.hash] = applied_hash
        tout.info(f'Found {len(applied)} potentially already applied'
                  ' commit(s)')
    return applied_map


def show_commit_diff(res, no_colour=False):
    """Show the difference between original and cherry-picked commit patches

    Args:
        res (CheckResult): Check result with commit hashes
        no_colour (bool): Disable colour output
    """
    tout.info(f'\n--- Patch diff between original {res.orig_hash[:8]} and '
              f'cherry-picked {res.chash[:8]} ---')

    # Get the patch content of each commit
    orig_patch = run_git(['show', '--no-ext-diff', res.orig_hash])
    cherry_patch = run_git(['show', '--no-ext-diff', res.chash])

    # Create temporary files and diff them
    with tempfile.NamedTemporaryFile(mode='w', suffix='_orig.patch',
                                     delete=False) as orig_file:
        orig_file.write(orig_patch)
        orig_path = orig_file.name

    with tempfile.NamedTemporaryFile(mode='w', suffix='_cherry.patch',
                                     delete=False) as cherry_file:
        cherry_file.write(cherry_patch)
        cherry_path = cherry_file.name

    try:
        # Diff the two patch files using system diff
        diff_args = ['diff', '-u']
        if not no_colour:
            diff_args.append('--color=always')
        diff_args.extend([orig_path, cherry_path])

        patch_diff = command.output(*diff_args, raise_on_error=False)
        if patch_diff:
            print(patch_diff)
        else:
            tout.info('(Patches are identical)')
    finally:
        # Clean up temporary files
        os.unlink(orig_path)
        os.unlink(cherry_path)

    tout.info('--- End patch diff ---\n')


def show_check_summary(bad, verbose, threshold, show_diff, no_colour):
    """Show summary of check results

    Args:
        bad (list): List of CheckResult objects with problems
        verbose (bool): Whether to show verbose output
        threshold (float): Delta threshold for problems
        show_diff (bool): Whether to show diffs for problems
        no_colour (bool): Whether to disable colour in diffs

    Returns:
        int: 0 if no problems, 1 if problems found
    """
    if bad:
        if verbose:
            tout.info(f'Found {len(bad)} commit(s) with large deltas:')
            tout.info('')
            print_check_header()
            for res in bad:
                tout.info(format_problem_commit(res, threshold))
                if show_diff:
                    show_commit_diff(res, no_colour)
        else:
            tout.info(f'{len(bad)} problem commit(s) found')
        return 1
    if verbose:
        tout.info('All cherry-picks have acceptable deltas ✓')
    return 0


def do_check(args, dbs):  # pylint: disable=unused-argument
    """Check current branch for cherry-picks with large deltas

    Args:
        args (Namespace): Parsed arguments with 'threshold', 'min_lines',
            'verbose', and 'diff' attributes
        dbs (Database): Database instance

    Returns:
        int: 0 on success, 1 on failure
    """
    threshold = args.threshold
    min_lines = args.min_lines
    verbose = args.verbose
    show_diff = args.diff

    current_branch, commits = get_branch_commits()

    if verbose:
        tout.info(f'Checking branch: {current_branch}')
        tout.info(f'Delta threshold: {threshold:.1%}')
        tout.info(f'Minimum lines to check: {min_lines}')
        tout.info(f'Found {len(commits)} commits to check')
        tout.info('')

    bad = []
    header_printed = False

    # Process commits using the generator
    for res in check_commits(commits, min_lines):
        is_problem = not res.reason and res.delta_ratio > threshold

        if verbose:
            check_verbose(res, threshold)
        elif is_problem:
            # Non-verbose: only show problems on one line
            if not header_printed:
                print_check_header()
                header_printed = True
            tout.info(format_problem_commit(res, threshold))

        if is_problem:
            bad.append(res)
            if show_diff:
                show_commit_diff(res, args.no_colour)

    return show_check_summary(bad, verbose, threshold, show_diff,
                              args.no_colour)


def do_check_gitlab(args, dbs):  # pylint: disable=unused-argument
    """Check GitLab permissions for the configured token

    Args:
        args (Namespace): Parsed arguments with 'remote' attribute
        dbs (Database): Database instance (unused)

    Returns:
        int: 0 on success with sufficient permissions, 1 otherwise
    """
    remote = args.remote

    perms = gitlab_api.check_permissions(remote)
    if not perms:
        return 1

    tout.info(f"GitLab permission check for remote '{remote}':")
    tout.info(f"  Host:         {perms.host}")
    tout.info(f"  Project:      {perms.project}")
    tout.info(f"  User:         {perms.user}")
    tout.info(f"  Access level: {perms.access_name}")
    tout.info('')
    tout.info('Permissions:')
    tout.info(f"  Push branches:    {'Yes' if perms.can_push else 'No'}")
    tout.info(f"  Create MRs:       {'Yes' if perms.can_create_mr else 'No'}")
    tout.info(f"  Merge MRs:        {'Yes' if perms.can_merge else 'No'}")

    if not perms.can_create_mr:
        tout.warning('')
        tout.warning('Insufficient permissions to create merge requests!')
        tout.warning('The user needs at least Developer access level.')
        return 1

    tout.info('')
    tout.info('All required permissions are available.')
    return 0


def find_unprocessed_commits(dbs, last_commit, source, merge_hashes):
    """Find the first merge with unprocessed commits

    Walks through the merge hashes in order, looking for one that has
    commits not yet tracked in the database. Decomposes mega-merges
    (merges containing sub-merges) into individual batches.

    Args:
        dbs (Database): Database instance
        last_commit (str): Hash of the last cherry-picked commit
        source (str): Source branch name
        merge_hashes (list): List of merge commit hashes to check

    Returns:
        NextCommitsInfo: Info about the next commits to process
    """
    prev_commit = last_commit
    skipped_merges = False
    for merge_hash in merge_hashes:
        # Check for mega-merge (contains sub-merges)
        sub_merges = detect_sub_merges(merge_hash)
        if sub_merges:
            commits, advance_to = decompose_mega_merge(
                dbs, prev_commit, merge_hash, sub_merges)
            if commits:
                return NextCommitsInfo(commits, True, advance_to)
            # All sub-merges done, skip past this mega-merge
            prev_commit = merge_hash
            skipped_merges = True
            continue

        # Get all commits from prev_commit to this merge
        log_output = run_git([
            'log', '--reverse', '--format=%H|%h|%an|%s|%P',
            f'{prev_commit}..{merge_hash}'
        ])

        if not log_output:
            prev_commit = merge_hash
            continue

        # Parse commits, filtering out those already in database
        all_commits = parse_log_output(log_output, has_parents=True)
        commits = [c for c in all_commits
                   if not dbs.commit_get(c.hash)]

        if commits:
            return NextCommitsInfo(commits, True, merge_hash)

        # All commits in this merge are processed, skip to next
        prev_commit = merge_hash
        skipped_merges = True

    # No merges with unprocessed commits, check remaining commits
    log_output = run_git([
        'log', '--reverse', '--format=%H|%h|%an|%s|%P',
        f'{prev_commit}..{source}'
    ])

    if not log_output:
        # If we skipped merges, advance past them
        advance_to = prev_commit if skipped_merges else None
        return NextCommitsInfo([], False, advance_to)

    all_commits = parse_log_output(log_output, has_parents=True)
    commits = [c for c in all_commits if not dbs.commit_get(c.hash)]

    return NextCommitsInfo(commits, False, None)


def get_next_commits(dbs, source):
    """Get the next set of commits to cherry-pick from a source

    Finds commits between the last cherry-picked commit and the next merge
    commit on the first-parent (mainline) chain of the source branch.
    Skips merges whose commits are already tracked in the database (from
    pending MRs). Decomposes mega-merges (merges containing sub-merges)
    into individual sub-merge batches.

    Args:
        dbs (Database): Database instance
        source (str): Source branch name

    Returns:
        tuple: (NextCommitsInfo, error_msg) where error_msg is None
            on success
    """
    # Get the last cherry-picked commit from database
    last_commit = dbs.source_get(source)

    if not last_commit:
        return None, f"Source '{source}' not found in database"

    # Get all first-parent commits to find merges
    fp_output = run_git([
        'log', '--reverse', '--first-parent', '--format=%H|%h|%an|%s|%P',
        f'{last_commit}..{source}'
    ])

    if not fp_output:
        return NextCommitsInfo([], False, None), None

    # Build list of merge hashes on the first-parent chain
    merge_hashes = []
    for line in fp_output.split('\n'):
        if not line:
            continue
        parts = line.split('|')
        parents = parts[-1].split()
        if len(parents) > 1:
            merge_hashes.append(parts[0])

    return find_unprocessed_commits(
        dbs, last_commit, source, merge_hashes), None


def get_commits_for_pick(commit_spec):
    """Get commits to cherry-pick from a commit specification

    Supports two formats:
    - Commit range: 'hash1..hash2' returns all commits in that range
    - Merge commit: Returns all non-merge commits that were part of the merge

    Args:
        commit_spec (str): Either 'hash1..hash2' for a range, or a single
            hash (which if it's a merge, gets all its child commits)

    Returns:
        tuple: (list of CommitInfo, error_message) - error_message is None
            on success
    """
    commits = None
    err = None

    if '..' in commit_spec:
        # Commit range format: hash1..hash2
        try:
            log_output = run_git([
                'log', '--reverse', '--format=%H|%h|%an|%s',
                commit_spec
            ])
            if log_output:
                commits = parse_log_output(log_output)
            else:
                commits, err = [], f"No commits found in range: {commit_spec}"
        except Exception:  # pylint: disable=broad-except
            err = f"Invalid commit range: {commit_spec}"
    else:
        # Single commit - check if it's a merge
        try:
            parents = run_git(['rev-parse', f'{commit_spec}^@'])
            parent_list = parents.strip().split('\n') if parents.strip() else []

            if len(parent_list) < 2:
                # Not a merge - return just this commit
                log_output = run_git(['log', '-1', '--format=%H|%h|%an|%s',
                                      commit_spec])
                commits = parse_log_output(log_output)
            else:
                # It's a merge - get commits from the merged branch
                # parent_list[0] is main branch, parent_list[1] is merged branch
                log_output = run_git([
                    'log', '--reverse', '--format=%H|%h|%an|%s',
                    f'^{parent_list[0]}', parent_list[1]
                ])
                if log_output:
                    commits = parse_log_output(log_output)
                else:
                    commits = []
                    err = f"No commits found in merge: {commit_spec}"
        except Exception:  # pylint: disable=broad-except
            err = f"Invalid commit: {commit_spec}"

    return commits, err


def detect_sub_merges(merge_hash):
    """Check if a merge commit contains sub-merges

    Examines the second parent's first-parent chain to find merge commits
    (sub-merges) within a larger merge.

    Args:
        merge_hash (str): Hash of the merge commit to check

    Returns:
        list: List of sub-merge hashes in chronological order, or empty
            list if not a merge or has no sub-merges
    """
    # Get parents of the merge
    try:
        parents = run_git(['rev-parse', f'{merge_hash}^@'])
    except command.CommandExc:
        return []

    parent_list = parents.strip().split('\n')
    if len(parent_list) < 2:
        return []

    first_parent = parent_list[0]
    second_parent = parent_list[1]

    # Find merges on the second parent's first-parent chain
    try:
        out = run_git([
            'log', '--reverse', '--first-parent', '--merges',
            '--format=%H', f'^{first_parent}', second_parent
        ])
    except command.CommandExc:
        return []

    if not out:
        return []

    return [line for line in out.split('\n') if line]


def decompose_mega_merge(dbs, prev_commit, merge_hash, sub_merges):
    """Return the next unprocessed batch from a mega-merge

    Handles three phases:
    1. Mainline commits before the merge (prev_commit..merge^1)
    2. Sub-merge batches (one at a time, skipping processed ones)
    3. Remainder commits after the last sub-merge

    Pre-adds the mega-merge commit itself to DB as 'skipped' so it does
    not appear as an orphan commit.

    Args:
        dbs (Database): Database instance
        prev_commit (str): Hash of the last processed commit
        merge_hash (str): Hash of the mega-merge commit
        sub_merges (list): List of sub-merge hashes in chronological order

    Returns:
        tuple: (commits, advance_to) where:
            commits: list of CommitInfo tuples for the next batch
            advance_to: hash to advance source to, or None to stay put
    """
    parents = run_git(['rev-parse', f'{merge_hash}^@']).strip().split('\n')
    first_parent = parents[0]
    second_parent = parents[1]

    # Pre-add the mega-merge commit itself as skipped
    if not dbs.commit_get(merge_hash):
        source_id = None
        sources = dbs.source_get_all()
        if sources:
            source_id = dbs.source_get_id(sources[0][0])
        if source_id:
            info = run_git(['log', '-1', '--format=%s|%an', merge_hash])
            parts = info.split('|', 1)
            subject = parts[0]
            author = parts[1] if len(parts) > 1 else ''
            dbs.commit_add(merge_hash, source_id, subject, author,
                           status='skipped')
            dbs.commit()

    # Phase 1: mainline commits before the merge
    log_output = run_git([
        'log', '--reverse', '--format=%H|%h|%an|%s|%P',
        f'{prev_commit}..{first_parent}'
    ])
    if log_output:
        all_commits = parse_log_output(log_output, has_parents=True)
        commits = [c for c in all_commits if not dbs.commit_get(c.hash)]
        if commits:
            return commits, first_parent

    # Phase 2: sub-merge batches
    prev_sub = first_parent
    for sub_hash in sub_merges:
        # Get commits for this sub-merge
        log_output = run_git([
            'log', '--reverse', '--format=%H|%h|%an|%s|%P',
            f'^{prev_sub}', sub_hash
        ])
        if log_output:
            all_commits = parse_log_output(log_output, has_parents=True)
            commits = [c for c in all_commits if not dbs.commit_get(c.hash)]
            if commits:
                return commits, None
        prev_sub = sub_hash

    # Phase 3: remainder after the last sub-merge
    last_sub = sub_merges[-1] if sub_merges else first_parent
    log_output = run_git([
        'log', '--reverse', '--format=%H|%h|%an|%s|%P',
        f'^{last_sub}', second_parent
    ])
    if log_output:
        all_commits = parse_log_output(log_output, has_parents=True)
        commits = [c for c in all_commits if not dbs.commit_get(c.hash)]
        if commits:
            return commits, None

    # All done
    return [], None


def do_next_set(args, dbs):
    """Show the next set of commits to cherry-pick from a source

    Args:
        args (Namespace): Parsed arguments with 'source' attribute
        dbs (Database): Database instance

    Returns:
        int: 0 on success, 1 if source not found
    """
    source = args.source
    info, err = get_next_commits(dbs, source)

    if err:
        tout.error(err)
        return 1

    if not info.commits:
        tout.info('No new commits to cherry-pick')
        return 0

    if info.merge_found:
        tout.info(f'Next set from {source} '
                  f'({len(info.commits)} commits):')
    else:
        tout.info(f'Remaining commits from {source} '
                  f'({len(info.commits)} commits, no merge found):')

    for commit in info.commits:
        tout.info(f'  {commit.chash} {commit.subject}')

    return 0


def do_next_merges(args, dbs):
    """Show the next N merges to be applied from a source

    Args:
        args (Namespace): Parsed arguments with 'source' and 'count' attributes
        dbs (Database): Database instance

    Returns:
        int: 0 on success, 1 if source not found
    """
    source = args.source
    count = args.count

    # Get the last cherry-picked commit from database
    last_commit = dbs.source_get(source)

    if not last_commit:
        tout.error(f"Source '{source}' not found in database")
        return 1

    # Find merge commits on the first-parent chain
    out = run_git([
        'log', '--reverse', '--first-parent', '--merges',
        '--format=%H|%h|%s',
        f'{last_commit}..{source}'
    ])

    if not out:
        tout.info('No merges remaining')
        return 0

    merges = []
    for line in out.split('\n'):
        if not line:
            continue
        parts = line.split('|', 2)
        commit_hash = parts[0]
        chash = parts[1]
        subject = parts[2] if len(parts) > 2 else ''
        merges.append((commit_hash, chash, subject))
        if len(merges) >= count:
            break

    # Build display list, expanding mega-merges into sub-merges
    # Each entry is (chash, subject, is_mega, sub_list) where sub_list
    # is a list of (chash, subject) for mega-merge sub-merges
    display = []
    total_sub = 0
    for commit_hash, chash, subject in merges:
        sub_merges = detect_sub_merges(commit_hash)
        if sub_merges:
            sub_list = []
            for sub_hash in sub_merges:
                try:
                    info = run_git(
                        ['log', '-1', '--format=%h|%s', sub_hash])
                    parts = info.strip().split('|', 1)
                    sub_chash = parts[0]
                    sub_subject = parts[1] if len(parts) > 1 else ''
                except Exception:  # pylint: disable=broad-except
                    sub_chash = sub_hash[:11]
                    sub_subject = '(unknown)'
                sub_list.append((sub_chash, sub_subject))
            display.append((chash, subject, True, sub_list))
            total_sub += len(sub_list)
        else:
            display.append((chash, subject, False, None))

    n_items = total_sub + len(merges) - len(
        [d for d in display if d[2]])
    tout.info(f'Next merges from {source} '
              f'({n_items} from {len(merges)} first-parent):')
    idx = 1
    for chash, subject, is_mega, sub_list in display:
        if is_mega:
            tout.info(f'  {chash} {subject} '
                      f'({len(sub_list)} sub-merges):')
            for sub_chash, sub_subject in sub_list:
                tout.info(f'    {idx}. {sub_chash} {sub_subject}')
                idx += 1
        else:
            tout.info(f'  {idx}. {chash} {subject}')
            idx += 1

    return 0


def do_count_merges(args, dbs):
    """Count total remaining merges to be applied from a source

    Args:
        args (Namespace): Parsed arguments with 'source' attribute
        dbs (Database): Database instance

    Returns:
        int: 0 on success, 1 if source not found
    """
    source = args.source

    # Get the last cherry-picked commit from database
    last_commit = dbs.source_get(source)

    if not last_commit:
        tout.error(f"Source '{source}' not found in database")
        return 1

    # Count merge commits on the first-parent chain
    fp_output = run_git([
        'log', '--first-parent', '--merges', '--oneline',
        f'{last_commit}..{source}'
    ])

    if not fp_output:
        tout.info('0 merges remaining')
        return 0

    count = len([line for line in fp_output.split('\n') if line])
    tout.info(f'{count} merges remaining from {source}')

    return 0


HISTORY_FILE = '.pickman-history'

# Tag added to MR title when skipped
SKIPPED_TAG = '[skipped]'


def parse_instruction(body):
    """Parse a pickman instruction from a comment body

    Recognizes instructions in these formats:
    - pickman <instruction>
    - pickman: <instruction>
    - @pickman <instruction>
    - @pickman: <instruction>

    Args:
        body (str): Comment body text

    Returns:
        str: The instruction (e.g., 'skip', 'unskip'), or None if not found
    """
    # Pattern matches: optional @, 'pickman', optional colon, then the command
    pattern = r'@?pickman:?\s+(\w+)'
    match = re.search(pattern, body.lower())
    if match:
        return match.group(1)
    return None


def has_instruction(body, instruction):
    """Check if a comment body contains a specific pickman instruction

    Args:
        body (str): Comment body text
        instruction (str): Instruction to check for (e.g., 'skip', 'unskip')

    Returns:
        bool: True if the comment contains the specified instruction
    """
    return parse_instruction(body) == instruction


def handle_unskip_comments(remote, mr_iid, title, unresolved, dbs):
    """Handle unskip comments on an MR

    Args:
        remote (str): Remote name
        mr_iid (int): Merge request IID
        title (str): Current MR title
        unresolved (list): List of unresolved comments
        dbs (Database): Database instance

    Returns:
        tuple: (handled, new_unresolved) where handled is True if unskip was
            processed and new_unresolved is the filtered comment list
    """
    unskip_comments = [c for c in unresolved
                       if has_instruction(c.body, 'unskip')]
    if not unskip_comments:
        return False, unresolved

    tout.info(f'MR !{mr_iid} has unskip request')

    # Update MR title to remove [skipped] tag
    if SKIPPED_TAG in title:
        new_title = title.replace(f'{SKIPPED_TAG} ', '')
        new_title = new_title.replace(SKIPPED_TAG, '')
        gitlab_api.update_mr_title(remote, mr_iid, new_title)
        tout.info(f'MR !{mr_iid} unskipped, will resume processing')

    # Mark unskip comments as processed
    for comment in unskip_comments:
        dbs.comment_mark_processed(mr_iid, comment.id)
    dbs.commit()

    # Reply to confirm the unskip
    gitlab_api.reply_to_mr(
        remote, mr_iid,
        'MR unskipped. Processing will resume on next poll.'
    )

    # Remove unskip comments from unresolved list for further processing
    new_unresolved = [c for c in unresolved
                      if not has_instruction(c.body, 'unskip')]
    return True, new_unresolved


def handle_skip_comments(remote, mr_iid, title, unresolved, dbs):
    """Handle skip comments on an MR

    Args:
        remote (str): Remote name
        mr_iid (int): Merge request IID
        title (str): Current MR title
        unresolved (list): List of unresolved comments
        dbs (Database): Database instance

    Returns:
        bool: True if skip was processed
    """
    skip_comments = [c for c in unresolved
                     if has_instruction(c.body, 'skip')]
    if not skip_comments:
        return False

    tout.info(f'MR !{mr_iid} has skip request, marking as skipped')

    # Update MR title to add [skipped] tag
    if SKIPPED_TAG not in title:
        new_title = f'{SKIPPED_TAG} {title}'
        gitlab_api.update_mr_title(remote, mr_iid, new_title)

    # Mark skip comments as processed
    for comment in skip_comments:
        dbs.comment_mark_processed(mr_iid, comment.id)
    dbs.commit()

    # Reply to confirm the skip
    gitlab_api.reply_to_mr(
        remote, mr_iid,
        'MR marked as skipped. Use `pickman unskip` or manually '
        'remove [skipped] from the title to resume processing.'
    )
    return True


def format_history(source, commits, branch_name):
    """Format a summary of the cherry-pick operation

    Args:
        source (str): Source branch name
        commits (list): list of CommitInfo tuples
        branch_name (str): Name of the cherry-pick branch

    Returns:
        str: Formatted summary text
    """
    commit_list = '\n'.join(
        f'- {c.chash} {c.subject}'
        for c in commits
    )

    return f"""## {date.today()}: {source}

Branch: {branch_name}

Commits:
{commit_list}"""


def get_history(fname, source, commits, branch_name, conv_log):
    """Read, update and write history file for a cherry-pick operation

    Args:
        fname (str): History filename to read/write
        source (str): Source branch name
        commits (list): list of CommitInfo tuples
        branch_name (str): Name of the cherry-pick branch
        conv_log (str): The agent's conversation output

    Returns:
        tuple: (content, commit_msg) where content is the updated history
            and commit_msg is the git commit message
    """
    summary = format_history(source, commits, branch_name)
    entry = f"""{summary}

### Conversation log
{conv_log}

---

"""

    # Read existing content
    existing = ''
    if os.path.exists(fname):
        with open(fname, 'r', encoding='utf-8') as fhandle:
            existing = fhandle.read()
        # Remove existing entry for this branch (from ## header to ---)
        pattern = rf'## [^\n]+\n\nBranch: {re.escape(branch_name)}\n.*?---\n\n'
        existing = re.sub(pattern, '', existing, flags=re.DOTALL)

    content = existing + entry

    # Write updated history file
    with open(fname, 'w', encoding='utf-8') as fhandle:
        fhandle.write(content)

    # Generate commit message
    commit_msg = (f'pickman: Record cherry-pick of {len(commits)} commits '
                  f'from {source}\n\n')
    commit_msg += '\n'.join(f'- {c.chash} {c.subject}' for c in commits)

    return content, commit_msg


def write_history(source, commits, branch_name, conv_log):
    """Write an entry to the pickman history file and commit it

    Args:
        source (str): Source branch name
        commits (list): list of CommitInfo tuples
        branch_name (str): Name of the cherry-pick branch
        conv_log (str): The agent's conversation output
    """
    _, commit_msg = get_history(HISTORY_FILE, source, commits, branch_name,
                                conv_log)

    # Commit the history file (use -f in case .gitignore patterns match)
    run_git(['add', '-f', HISTORY_FILE])
    run_git(['commit', '-m', commit_msg])

    tout.info(f'Updated {HISTORY_FILE}')


def push_mr(args, branch_name, title, description):
    """Push branch and create merge request

    Args:
        args (Namespace): Parsed arguments with 'remote' and 'target'
        branch_name (str): Branch name to push
        title (str): MR title
        description (str): MR description

    Returns:
        bool: True on success, False on failure
    """
    mr_url = gitlab_api.push_and_create_mr(
        args.remote, branch_name, args.target, title, description
    )
    return bool(mr_url)


def prepare_apply(dbs, source, branch):
    """Prepare for applying commits from a source branch

    Gets the next commits, sets up the branch name, and prints info about
    what will be applied.

    Args:
        dbs (Database): Database instance
        source (str): Source branch name
        branch (str): Branch name to use, or None to auto-generate

    Returns:
        tuple: (ApplyInfo, return_code) where ApplyInfo is set if there are
            commits to apply, or None with return_code indicating the result
            (0 for no commits, 1 for error)
    """
    info, err = get_next_commits(dbs, source)

    if err:
        tout.error(err)
        return None, 1

    if not info.commits:
        # If advance_to is set, advance source past fully-processed merges
        if info.advance_to:
            dbs.source_set(source, info.advance_to)
            dbs.commit()
            tout.info(f"Advanced source '{source}' to "
                      f'{info.advance_to[:12]}')
            # Retry with updated position
            return prepare_apply(dbs, source, branch)
        tout.info('No new commits to cherry-pick')
        return None, 0

    commits = info.commits

    # Save current branch to return to later
    original_branch = run_git(['rev-parse', '--abbrev-ref', 'HEAD'])

    # Generate branch name if not provided
    branch_name = branch
    if not branch_name:
        # Use first commit's short hash as part of branch name
        branch_name = f'cherry-{commits[0].chash}'

    # Delete branch if it already exists
    if run_git(['branch', '--list', branch_name]).strip():
        tout.info(f'Deleting existing branch {branch_name}')
        run_git(['branch', '-D', branch_name])

    if info.merge_found:
        tout.info(f'Applying next set from {source} ({len(commits)} commits):')
    else:
        tout.info(f'Applying remaining commits from {source} '
                  f'({len(commits)} commits, no merge found):')

    tout.info(f'  Branch: {branch_name}')
    for commit in commits:
        tout.info(f'  {commit.chash} {commit.subject}')
    tout.info('')

    return ApplyInfo(commits, branch_name, original_branch,
                     info.merge_found, info.advance_to), 0


# pylint: disable=too-many-arguments
def handle_already_applied(dbs, source, commits, branch_name, conv_log, args,
                           signal_commit, advance_to=None):
    """Handle the case where commits are already applied to the target branch

    Creates an MR with [skip] prefix to record the attempt and updates the
    source position in the database.

    Args:
        dbs (Database): Database instance
        source (str): Source branch name
        commits (list): List of CommitInfo namedtuples
        branch_name (str): Branch name that was attempted
        conv_log (str): Conversation log from the agent
        args (Namespace): Parsed arguments with 'push', 'remote', 'target'
        signal_commit (str): Last commit hash from signal file
        advance_to (str): Hash to advance source to, or None to use last
            commit. If explicitly None (sub-merge batch), source is not
            advanced.

    Returns:
        int: 0 on success, 1 on failure
    """
    tout.info('Commits already applied to target branch - creating skip MR')

    # Mark commits as 'skipped' in database
    for commit in commits:
        dbs.commit_set_status(commit.hash, 'skipped')
    dbs.commit()

    # Update source position
    if advance_to is not None:
        dbs.source_set(source, advance_to)
        dbs.commit()
        tout.info(f"Updated source '{source}' to {advance_to[:12]}")
    elif signal_commit:
        dbs.source_set(source, signal_commit)
        dbs.commit()
        tout.info(f"Updated source '{source}' to {signal_commit[:12]}")
    else:
        last_hash = commits[-1].hash
        dbs.source_set(source, last_hash)
        dbs.commit()
        tout.info(f"Updated source '{source}' to {last_hash[:12]}")

    # Push and create MR with [skip] prefix if requested
    if args.push:
        remote = args.remote
        target = args.target

        # Create a skip branch from ci/master (no changes)
        try:
            run_git(['checkout', '-b', branch_name, f'{remote}/{target}'])
        except Exception:  # pylint: disable=broad-except
            # Branch may already exist from failed attempt
            try:
                run_git(['checkout', branch_name])
            except Exception:  # pylint: disable=broad-except
                tout.error(f'Could not create/checkout branch {branch_name}')
                return 1

        # Use merge commit subject as title with [skip] prefix
        title = f'{SKIPPED_TAG} [pickman] {commits[-1].subject}'
        summary = format_history(source, commits, branch_name)
        description = (f'{summary}\n\n'
                       f'**Status:** Commits already applied to {target} '
                       f'with different hashes.\n\n'
                       f'### Conversation log\n{conv_log}')

        mr_url = gitlab_api.push_and_create_mr(
            remote, branch_name, target, title, description
        )
        if not mr_url:
            return 1

    return 0


def execute_apply(dbs, source, commits, branch_name, args, advance_to=None):  # pylint: disable=too-many-locals
    """Execute the apply operation: run agent, update database, push MR

    Args:
        dbs (Database): Database instance
        source (str): Source branch name
        commits (list): List of CommitInfo namedtuples
        branch_name (str): Branch name for cherry-picks
        args (Namespace): Parsed arguments with 'push', 'remote', 'target'
        advance_to (str): Hash to advance source to after success, or None
            to skip source advancement (sub-merge batch)

    Returns:
        tuple: (ret, success, conv_log) where ret is 0 on success,
            1 on failure
    """
    # Check for already applied commits before proceeding
    applied_map = build_applied_map(commits)

    # Add all commits to database with 'pending' status (agent updates later)
    source_id = dbs.source_get_id(source)
    for commit in commits:
        dbs.commit_add(commit.hash, source_id, commit.subject, commit.author,
                       status='pending')
    dbs.commit()

    # Convert CommitInfo to AgentCommit format expected by agent
    agent_commits = [AgentCommit(c.hash, c.chash, c.subject,
                                 applied_map.get(c.hash)) for c in commits]
    success, conv_log = agent.cherry_pick_commits(agent_commits, source,
                                                  branch_name)

    # Check for signal file from agent
    signal_status, signal_commit = agent.read_signal_file()
    if signal_status == agent.SIGNAL_APPLIED:
        ret = handle_already_applied(dbs, source, commits, branch_name,
                                     conv_log, args, signal_commit,
                                     advance_to)
        return ret, False, conv_log

    # Verify the branch actually exists - agent may have aborted and deleted it
    if success:
        try:
            exists = run_git(['branch', '--list', branch_name]).strip()
        except Exception:  # pylint: disable=broad-except
            exists = ''
        if not exists:
            tout.warning(f'Branch {branch_name} does not exist - '
                         'agent may have aborted')
            success = False

    # Update commit status based on result
    status = 'applied' if success else 'conflict'
    for commit in commits:
        dbs.commit_set_status(commit.hash, status)
    dbs.commit()

    ret = 0 if success else 1

    if success:
        # Push and create MR if requested
        if args.push:
            title = f'[pickman] {commits[-1].subject}'
            summary = format_history(source, commits, branch_name)
            description = f'{summary}\n\n### Conversation log\n{conv_log}'
            if not push_mr(args, branch_name, title, description):
                ret = 1
        else:
            tout.info(f"Use 'pickman commit-source {source} "
                      f"{commits[-1].chash}' to update the database")

    # Update database with the last processed commit if successful
    if success and advance_to is not None:
        dbs.source_set(source, advance_to)
        dbs.commit()

    return ret, success, conv_log


def do_apply(args, dbs):
    """Apply the next set of commits using Claude agent

    Args:
        args (Namespace): Parsed arguments with 'source' and 'branch' attributes
        dbs (Database): Database instance

    Returns:
        int: 0 on success, 1 on failure
    """
    source = args.source
    info, ret = prepare_apply(dbs, source, args.branch)
    if not info:
        return ret

    commits = info.commits
    branch_name = info.branch_name
    original_branch = info.original_branch

    ret, success, conv_log = execute_apply(dbs, source, commits,
                                           branch_name, args,
                                           info.advance_to)

    # Write history file if successful
    if success:
        write_history(source, commits, branch_name, conv_log)

    # Return to original branch
    current_branch = run_git(['rev-parse', '--abbrev-ref', 'HEAD'])
    if current_branch != original_branch:
        tout.info(f'Returning to {original_branch}')
        run_git(['checkout', original_branch])

    return ret


def do_pick(args, dbs):  # pylint: disable=unused-argument,too-many-locals
    """Cherry-pick commits ad-hoc using Claude agent

    This allows cherry-picking a commit range or merge commit children without
    tracking in the database. Useful for one-off cherry-picks.

    Args:
        args (Namespace): Parsed arguments with 'commits', 'branch', etc.
        dbs (Database): Database instance (unused for ad-hoc picks)

    Returns:
        int: 0 on success, 1 on failure
    """
    commit_spec = args.commits

    # Get commits to cherry-pick
    commits, err = get_commits_for_pick(commit_spec)
    if err:
        tout.error(err)
        return 1

    if not commits:
        tout.info('No commits to cherry-pick')
        return 0

    # Save current branch to return to later
    original_branch = run_git(['rev-parse', '--abbrev-ref', 'HEAD'])

    # Generate branch name if not provided
    branch_name = args.branch
    if not branch_name:
        branch_name = f'pick-{commits[0].chash}'

    # Delete branch if it already exists
    if run_git(['branch', '--list', branch_name]).strip():
        tout.info(f'Deleting existing branch {branch_name}')
        run_git(['branch', '-D', branch_name])

    tout.info(f'Cherry-picking {len(commits)} commit(s):')
    tout.info(f'  Branch: {branch_name}')
    for commit in commits:
        tout.info(f'  {commit.chash} {commit.subject}')
    tout.info('')

    # Convert CommitInfo to AgentCommit format (no applied_as for ad-hoc)
    agent_commits = [AgentCommit(c.hash, c.chash, c.subject, None)
                     for c in commits]

    # Run the agent to cherry-pick
    success, conv_log = agent.cherry_pick_commits(agent_commits, 'ad-hoc',
                                                  branch_name)

    # Verify the branch actually exists - agent may have aborted and deleted it
    if success:
        try:
            exists = run_git(['branch', '--list', branch_name]).strip()
        except Exception:  # pylint: disable=broad-except
            exists = ''
        if not exists:
            tout.warning(f'Branch {branch_name} does not exist - '
                         'agent may have aborted')
            success = False

    ret = 0 if success else 1

    if success and args.push:
        title = f'[pick] {commits[-1].subject}'
        commit_list = '\n'.join(f'- {c.chash} {c.subject}' for c in commits)
        description = (f'Ad-hoc cherry-pick of {len(commits)} commit(s)\n\n'
                       f'### Commits\n{commit_list}\n\n'
                       f'### Conversation log\n{conv_log}')
        if not push_mr(args, branch_name, title, description):
            ret = 1
    elif success:
        tout.info(f'Commits cherry-picked to branch {branch_name}')

    # Return to original branch
    current_branch = run_git(['rev-parse', '--abbrev-ref', 'HEAD'])
    if current_branch != original_branch:
        tout.info(f'Returning to {original_branch}')
        run_git(['checkout', original_branch])

    return ret


def do_push_branch(args, dbs):  # pylint: disable=unused-argument
    """Push a branch using the GitLab API token for authentication

    This allows pushing as the token owner (e.g., a bot account) rather than
    using the user's configured git credentials. Useful for ensuring all
    pickman commits come from the same account.

    Args:
        args (Namespace): Parsed arguments with 'remote', 'branch', 'force',
            'run_ci'
        dbs (Database): Database instance

    Returns:
        int: 0 on success, 1 on failure
    """
    skip_ci = not args.run_ci
    success = gitlab_api.push_branch(args.remote, args.branch, args.force,
                                     skip_ci=skip_ci)
    return 0 if success else 1


def do_commit_source(args, dbs):
    """Update the database with the last cherry-picked commit

    Args:
        args (Namespace): Parsed arguments with 'source' and 'commit' attributes
        dbs (Database): Database instance

    Returns:
        int: 0 on success, 1 on failure
    """
    source = args.source
    commit = args.commit

    # Resolve the commit to a full hash
    try:
        full_hash = run_git(['rev-parse', commit])
    except Exception:  # pylint: disable=broad-except
        tout.error(f"Could not resolve commit '{commit}'")
        return 1

    old_commit = dbs.source_get(source)
    if not old_commit:
        tout.error(f"Source '{source}' not found in database")
        return 1

    dbs.source_set(source, full_hash)
    dbs.commit()

    short_old = old_commit[:12]
    short_new = full_hash[:12]
    tout.info(f"Updated '{source}': {short_old} -> {short_new}")

    return 0


def _rewind_fetch_merges(current, count):
    """Fetch first-parent merges and find target index.

    Returns:
        tuple: (merges, target_idx) where merges is a list of
            (hash, short_hash, subject) tuples, or None on error
    """
    try:
        out = run_git([
            'log', '--first-parent', '--merges', '--format=%H|%h|%s',
            f'-{count + 1}', current
        ])
    except Exception:  # pylint: disable=broad-except
        tout.error(f'Could not read merge history for {current[:12]}')
        return None

    if not out:
        tout.error('No merges found in history')
        return None

    merges = []
    for line in out.strip().split('\n'):
        if not line:
            continue
        parts = line.split('|', 2)
        merges.append((parts[0], parts[1],
                        parts[2] if len(parts) > 2 else ''))

    if len(merges) < 2:
        tout.error(f'Not enough merges to rewind by {count}')
        return None

    target_idx = min(count, len(merges) - 1)
    return merges, target_idx


def _rewind_get_range_commits(dbs, target_hash, current):
    """Get commits in range and filter to those in database.

    Returns:
        tuple: (range_hashes_str, db_commits_list) or None on error
    """
    try:
        range_hashes = run_git([
            'rev-list', f'{target_hash}..{current}'
        ])
    except Exception:  # pylint: disable=broad-except
        tout.error(f'Could not list commits in range '
                   f'{target_hash[:12]}..{current[:12]}')
        return None

    db_commits = []
    if range_hashes:
        for chash in range_hashes.strip().split('\n'):
            if chash and dbs.commit_get(chash):
                db_commits.append(chash)

    return range_hashes, db_commits


def _rewind_find_branches(range_hashes, remote):
    """Find cherry-pick branches matching commits in the range.

    Returns:
        list: Branch names (without remote prefix) that match
    """
    if not range_hashes:
        return []

    hash_set = set(range_hashes.strip().split('\n'))
    try:
        branch_out = run_git(
            ['branch', '-r', '--list', f'{remote}/cherry-*'])
    except Exception:  # pylint: disable=broad-except
        branch_out = ''

    mr_branches = []
    remote_prefix = f'{remote}/'
    for line in branch_out.strip().split('\n'):
        branch = line.strip()
        if not branch:
            continue
        # Branch is like 'ci/cherry-abc1234'; extract the hash part
        short = branch.removeprefix(f'{remote_prefix}cherry-')
        # Check if any commit in the range starts with this hash
        for chash in hash_set:
            if chash.startswith(short):
                mr_branches.append(
                    branch.removeprefix(remote_prefix))
                break

    return mr_branches


def _rewind_find_mrs(mr_branches, remote):
    """Look up MR details for matching branches.

    Returns:
        list: PickmanMr objects whose source_branch matches
    """
    if not mr_branches:
        return []

    matched_mrs = []
    mrs = gitlab_api.get_open_pickman_mrs(remote)
    if mrs:
        branch_set = set(mr_branches)
        for merge_req in mrs:
            if merge_req.source_branch in branch_set:
                matched_mrs.append(merge_req)

    return matched_mrs


def _rewind_show_summary(source, current, merges, target_idx,
                         db_commits, matched_mrs, mr_branches,
                         force):
    """Display rewind summary."""
    current_short = current[:12]
    target_chash = merges[target_idx][1]
    target_subject = merges[target_idx][2]

    prefix = '' if force else '[dry run] '
    tout.info(f"{prefix}Rewind '{source}': "
              f'{current_short} -> {target_chash}')
    tout.info(f'  Target: {target_chash} {target_subject}')
    tout.info('  Merges being rewound:')
    for i in range(target_idx):
        tout.info(f'    {merges[i][1]} {merges[i][2]}')
    tout.info(f'  Commits to delete from database: {len(db_commits)}')

    if matched_mrs:
        tout.info('  MRs to delete on GitLab:')
        for merge_req in matched_mrs:
            tout.info(f'    !{merge_req.iid}: {merge_req.title}')
            tout.info(f'      {merge_req.web_url}')
    elif mr_branches:
        tout.info('  Branches to check for MRs:')
        for branch in mr_branches:
            tout.info(f'    {branch}')


def do_rewind(args, dbs):
    """Rewind the source position back by N merges

    By default performs a dry run, showing what would happen. Use --force
    to actually execute the rewind.

    Walks back N merges on the first-parent chain from the current source
    position, deletes the commits in that range from the database, and
    resets the source to the earlier position.

    Args:
        args (Namespace): Parsed arguments with 'source', 'count', 'force'
        dbs (Database): Database instance

    Returns:
        int: 0 on success, 1 on failure
    """
    source = args.source
    count = args.count
    force = args.force

    current = dbs.source_get(source)
    if not current:
        tout.error(f"Source '{source}' not found in database")
        return 1

    result = _rewind_fetch_merges(current, count)
    if not result:
        return 1
    merges, target_idx = result
    target_hash = merges[target_idx][0]

    result = _rewind_get_range_commits(dbs, target_hash, current)
    if not result:
        return 1
    range_hashes, db_commits = result

    mr_branches = _rewind_find_branches(range_hashes, args.remote)
    matched_mrs = _rewind_find_mrs(mr_branches, args.remote)

    _rewind_show_summary(source, current, merges, target_idx,
                         db_commits, matched_mrs, mr_branches, force)

    if not force:
        tout.info('Use --force to execute this rewind')
        return 0

    for chash in db_commits:
        dbs.commit_delete(chash)

    dbs.source_set(source, target_hash)
    dbs.commit()

    tout.info(f'  Deleted {len(db_commits)} commit(s) from database')

    return 0


# pylint: disable=too-many-locals,too-many-branches,too-many-statements
def process_single_mr(remote, merge_req, dbs, target):
    """Process review comments on a single MR

    Args:
        remote (str): Remote name
        merge_req (PickmanMr): MR object from get_open_pickman_mrs()
        dbs (Database): Database instance for tracking processed comments
        target (str): Target branch for rebase operations

    Returns:
        int: 1 if MR was processed, 0 otherwise
    """
    mr_iid = merge_req.iid
    comments = gitlab_api.get_mr_comments(remote, mr_iid)
    if comments is None:
        comments = []

    # Filter to unresolved comments that haven't been processed
    unresolved = []
    for com in comments:
        if com.resolved:
            continue
        if dbs.comment_is_processed(mr_iid, com.id):
            continue
        unresolved.append(com)

    # Check for unskip comments first (takes precedence)
    handled, unresolved = handle_unskip_comments(
        remote, mr_iid, merge_req.title, unresolved, dbs)
    processed = 1 if handled else 0

    # Check for skip comments
    if handle_skip_comments(remote, mr_iid, merge_req.title, unresolved, dbs):
        return processed + 1

    # If MR is currently skipped, don't process rebases or other comments
    if SKIPPED_TAG in merge_req.title:
        return processed

    # Check if rebase is needed
    needs_rebase = merge_req.needs_rebase or merge_req.has_conflicts

    # Skip if no comments and no rebase needed
    if not unresolved and not needs_rebase:
        return processed

    tout.info('')
    if needs_rebase:
        if merge_req.has_conflicts:
            tout.info(f"MR !{mr_iid} has merge conflicts - rebasing...")
        else:
            tout.info(f"MR !{mr_iid} needs rebase...")
    if unresolved:
        tout.info(f"MR !{mr_iid} has {len(unresolved)} new comment(s):")
        for comment in unresolved:
            tout.info(f'  [{comment.author}]: {comment.body[:80]}...')

    # Run agent to handle comments and/or rebase
    success, conversation_log = agent.handle_mr_comments(
        mr_iid,
        merge_req.source_branch,
        unresolved,
        remote,
        target,
        needs_rebase=needs_rebase,
        has_conflicts=merge_req.has_conflicts,
        mr_description=merge_req.description,
    )

    if success:
        # Mark comments as processed
        for comment in unresolved:
            dbs.comment_mark_processed(mr_iid, comment.id)
        dbs.commit()

        # Update MR description with comments and conversation log
        old_desc = merge_req.description
        comment_summary = '\n'.join(
            f"- [{c.author}]: {c.body}"
            for c in unresolved
        )
        new_desc = (f"{old_desc}\n\n### Review response\n\n"
                    f"**Comments addressed:**\n{comment_summary}\n\n"
                    f"**Response:**\n{conversation_log}")
        gitlab_api.update_mr_desc(remote, mr_iid, new_desc)

        # Update .pickman-history
        update_history(merge_req.source_branch,
                                   unresolved, conversation_log)

        tout.info(f'Updated MR !{mr_iid} description and history')
    else:
        tout.error(f"Failed to handle comments for MR !{mr_iid}")

    return processed + 1


def process_mr_reviews(remote, mrs, dbs, target='master'):
    """Process review comments on open MRs

    Checks each MR for unresolved comments and uses Claude agent to address
    them. Updates MR description and .pickman-history with conversation log.

    Args:
        remote (str): Remote name
        mrs (list): List of MR dicts from get_open_pickman_mrs()
        dbs (Database): Database instance for tracking processed comments
        target (str): Target branch for rebase operations

    Returns:
        int: Number of MRs with comments processed
    """
    # Save current branch to restore later
    original_branch = run_git(['rev-parse', '--abbrev-ref', 'HEAD'])

    # Fetch to get latest remote state (needed for rebase)
    tout.info(f'Fetching {remote}...')
    run_git(['fetch', remote])

    processed = 0
    for merge_req in mrs:
        processed += process_single_mr(remote, merge_req, dbs, target)

    # Restore original branch
    if processed:
        tout.info(f'Returning to {original_branch}')
        run_git(['checkout', original_branch])

    return processed


def update_history(branch_name, comments, conversation_log):
    """Append review handling to .pickman-history

    Args:
        branch_name (str): Branch name for the MR
        comments (list): List of comments that were addressed
        conversation_log (str): Agent conversation log
    """
    comment_summary = '\n'.join(
        f'- [{c.author}]: {c.body[:100]}...'
        for c in comments
    )

    entry = f'''### Review: {date.today()}

Branch: {branch_name}

Comments addressed:
{comment_summary}

### Conversation log
{conversation_log}

---

'''

    # Append to history file
    existing = ''
    if os.path.exists(HISTORY_FILE):
        with open(HISTORY_FILE, 'r', encoding='utf-8') as fhandle:
            existing = fhandle.read()

    with open(HISTORY_FILE, 'w', encoding='utf-8') as fhandle:
        fhandle.write(existing + entry)

    # Commit the history file
    run_git(['add', '-f', HISTORY_FILE])
    run_git(['commit', '-m',
             f'pickman: Record review handling for {branch_name}'])


def do_review(args, dbs):
    """Check open pickman MRs and handle comments

    Lists open MRs created by pickman, checks for human comments, and uses
    Claude agent to address them.

    Args:
        args (Namespace): Parsed arguments with 'remote' attribute
        dbs (Database): Database instance

    Returns:
        int: 0 on success, 1 on failure
    """
    remote = args.remote

    # Get open pickman MRs
    mrs = gitlab_api.get_open_pickman_mrs(remote)
    if mrs is None:
        return 1

    if not mrs:
        tout.info('No open pickman MRs found')
        return 0

    tout.info(f'Found {len(mrs)} open pickman MR(s):')
    for merge_req in mrs:
        tout.info(f"  !{merge_req.iid}: {merge_req.title}")

    process_mr_reviews(remote, mrs, dbs)

    return 0


def parse_mr_description(desc):
    """Parse a pickman MR description to extract source and last commit

    Args:
        desc (str): MR description text

    Returns:
        tuple: (source_branch, last_commit_hash) or (None, None)
            if not parseable
    """
    # Extract source branch from "## date: source_branch" line
    source_match = re.search(r'^## [^:]+: (.+)$', desc, re.MULTILINE)
    if not source_match:
        return None, None
    source = source_match.group(1)

    # Extract commits from '- hash subject' lines (must be at least 7 chars)
    commit_matches = re.findall(r'^- ([a-f0-9]{7,}) ', desc, re.MULTILINE)
    if not commit_matches:
        return None, None

    # Last commit is the last one in the list
    last_hash = commit_matches[-1]

    return source, last_hash


def process_merged_mrs(remote, source, dbs):
    """Check for merged MRs and update the database

    Args:
        remote (str): Remote name
        source (str): Source branch name to filter by
        dbs (Database): Database instance

    Returns:
        int: Number of MRs processed, or -1 on error
    """
    merged_mrs = gitlab_api.get_merged_pickman_mrs(remote)
    if merged_mrs is None:
        return -1

    processed = 0
    for merge_req in merged_mrs:
        mr_source, last_hash = parse_mr_description(merge_req.description)

        # Only process MRs for the requested source branch
        if mr_source != source:
            continue

        # Check if this MR's last commit is newer than current database
        current = dbs.source_get(source)
        if not current:
            continue

        # Resolve the short hash to full hash
        try:
            full_hash = run_git(['rev-parse', last_hash])
        except Exception:  # pylint: disable=broad-except
            tout.warning(f"Could not resolve commit '{last_hash}' from "
                         f"MR !{merge_req.iid}")
            continue

        # Skip if already at this position
        if full_hash == current:
            continue

        # Check if this commit is newer than current (current is ancestor of it)
        try:
            # Is current an ancestor of last_hash? (meaning last_hash is newer)
            run_git(['merge-base', '--is-ancestor', current, full_hash])
        except Exception:  # pylint: disable=broad-except
            continue  # current is not an ancestor, so last_hash is not newer

        # Update database
        short_old = current[:12]
        short_new = full_hash[:12]
        tout.info(f"MR !{merge_req.iid} merged, updating '{source}': "
                  f'{short_old} -> {short_new}')
        dbs.source_set(source, full_hash)
        dbs.commit()
        processed += 1

    return processed


def do_step(args, dbs):
    """Create an MR if below the max allowed

    Checks for merged pickman MRs and updates the database, then checks for
    open pickman MRs. If open MRs exist, processes any review comments. If
    the number of open MRs is below max_mrs, runs apply with push to create
    a new one.

    Args:
        args (Namespace): Parsed arguments with 'source', 'remote', 'target',
            'max_mrs'
        dbs (Database): Database instance

    Returns:
        int: 0 on success, 1 on failure
    """
    remote = args.remote
    source = args.source

    # First check for merged MRs and update database
    processed = process_merged_mrs(remote, source, dbs)
    if processed < 0:
        return 1

    # Check for open pickman MRs
    mrs = gitlab_api.get_open_pickman_mrs(remote)
    if mrs is None:
        return 1

    # Separate skipped and active MRs
    active_mrs = [m for m in mrs if SKIPPED_TAG not in m.title]
    skipped_mrs = [m for m in mrs if SKIPPED_TAG in m.title]

    if mrs:
        if active_mrs:
            tout.info(f'Found {len(active_mrs)} open pickman MR(s):')
            for merge_req in active_mrs:
                tout.info(f"  !{merge_req.iid}: {merge_req.title}")
        if skipped_mrs:
            tout.info(f'Found {len(skipped_mrs)} skipped pickman MR(s):')
            for merge_req in skipped_mrs:
                tout.info(f"  !{merge_req.iid}: {merge_req.title}")

        # Process any review comments on all open MRs (including skipped,
        # in case they have an unskip request)
        process_mr_reviews(remote, mrs, dbs, args.target)

    # Only block new MR creation if we've reached the max allowed open MRs
    max_mrs = args.max_mrs
    if len(active_mrs) >= max_mrs:
        tout.info('')
        tout.info(f'Already have {len(active_mrs)} open MR(s) (max: {max_mrs})')
        return 0

    # No pending MRs, run apply with push
    # First fetch to get latest remote state
    tout.info(f'Fetching {remote}...')
    run_git(['fetch', remote])

    if active_mrs:
        tout.info('Creating another MR...')
    else:
        tout.info('No pending pickman MRs, creating new one...')
    args.push = True
    args.branch = None  # Let do_apply generate branch name
    return do_apply(args, dbs)


def do_poll(args, dbs):
    """Run step repeatedly until stopped

    Runs the step command in a loop with a configurable interval. Useful for
    automated workflows that continuously process cherry-picks.

    Args:
        args (Namespace): Parsed arguments with 'source', 'interval', 'remote',
            'target'
        dbs (Database): Database instance

    Returns:
        int: 0 on success (never returns unless interrupted)
    """
    interval = args.interval
    tout.info(f'Polling every {interval} seconds (Ctrl+C to stop)...')
    tout.info('')

    while True:
        try:
            ret = do_step(args, dbs)
            if ret != 0:
                tout.warning(f'step returned {ret}')
            tout.info('')
            tout.info(f'Sleeping {interval} seconds...')
            time.sleep(interval)
            tout.info('')
        except KeyboardInterrupt:
            tout.info('')
            tout.info('Polling stopped by user')
            return 0


def do_test(args, dbs):  # pylint: disable=unused-argument
    """Run tests for this module.

    Args:
        args (Namespace): Parsed arguments
        dbs (Database): Database instance

    Returns:
        int: 0 if tests passed, 1 otherwise
    """
    loader = unittest.TestLoader()
    suite = loader.loadTestsFromModule(ftest)
    runner = unittest.TextTestRunner()
    result = runner.run(suite)

    return 0 if result.wasSuccessful() else 1


# Command dispatch table
COMMANDS = {
    'add-source': do_add_source,
    'apply': do_apply,
    'check': do_check,
    'check-gitlab': do_check_gitlab,
    'commit-source': do_commit_source,
    'compare': do_compare,
    'count-merges': do_count_merges,
    'list-sources': do_list_sources,
    'next-merges': do_next_merges,
    'next-set': do_next_set,
    'pick': do_pick,
    'poll': do_poll,
    'push-branch': do_push_branch,
    'review': do_review,
    'rewind': do_rewind,
    'step': do_step,
    'test': do_test,
}


def do_pickman(args):
    """Main entry point for pickman commands.

    Args:
        args (Namespace): Parsed arguments

    Returns:
        int: 0 on success, 1 on failure
    """
    tout.init(tout.INFO)

    handler = COMMANDS.get(args.cmd)
    if handler:
        dbs = database.Database(DB_FNAME)
        dbs.start()
        try:
            return handler(args, dbs)
        finally:
            dbs.close()
    return 1

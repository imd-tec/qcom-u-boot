# SPDX-License-Identifier: GPL-2.0+
#
# Copyright 2025 Canonical Ltd.
# Written by Simon Glass <simon.glass@canonical.com>
#
"""Control module for pickman - handles the main logic."""

from collections import namedtuple
from datetime import date
import os
import re
import sys
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
from u_boot_pylib import tout

# Default database filename
DB_FNAME = '.pickman.db'

# Branch names to compare
BRANCH_MASTER = 'ci/master'
BRANCH_SOURCE = 'us/next'

# Named tuple for commit info
Commit = namedtuple('Commit', ['hash', 'short_hash', 'subject', 'date'])

# Named tuple for commit with author
CommitInfo = namedtuple('CommitInfo',
                        ['hash', 'short_hash', 'subject', 'author'])

# Named tuple for prepare_apply result
ApplyInfo = namedtuple('ApplyInfo',
                       ['commits', 'branch_name', 'original_branch',
                        'merge_found'])


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
    full_hash, short_hash, subject, commit_date = info.split('\n')

    return count, Commit(full_hash, short_hash, subject, commit_date)


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
    short_hash, subject = info.split('\n')

    # Store in database
    dbs.source_set(source, base_hash)
    dbs.commit()

    tout.info(f"Added source '{source}' with base commit:")
    tout.info(f'  Hash:    {short_hash}')
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
    tout.info(f'  Hash:    {base.short_hash}')
    tout.info(f'  Subject: {base.subject}')
    tout.info(f'  Date:    {base.date}')

    return 0


def get_next_commits(dbs, source):
    """Get the next set of commits to cherry-pick from a source

    Finds commits between the last cherry-picked commit and the next merge
    commit on the first-parent (mainline) chain of the source branch.

    Args:
        dbs (Database): Database instance
        source (str): Source branch name

    Returns:
        tuple: (commits, merge_found, error_msg) where:
            commits: list of CommitInfo tuples
            merge_found: bool, True if stopped at a merge commit
            error_msg: str or None, error message if failed
    """
    # Get the last cherry-picked commit from database
    last_commit = dbs.source_get(source)

    if not last_commit:
        return None, False, f"Source '{source}' not found in database"

    # First, find the next merge commit on the first-parent chain
    # This ensures we follow the mainline and find merges in order
    fp_output = run_git([
        'log', '--reverse', '--first-parent', '--format=%H|%h|%an|%s|%P',
        f'{last_commit}..{source}'
    ])

    if not fp_output:
        return [], False, None

    # Find the first merge on the first-parent chain
    merge_hash = None
    for line in fp_output.split('\n'):
        if not line:
            continue
        parts = line.split('|')
        parents = parts[-1].split()
        if len(parents) > 1:
            merge_hash = parts[0]
            break

    # Now get all commits from last_commit to the merge (or end of branch)
    # Without --first-parent to include commits from merged branches
    log_output = run_git([
        'log', '--reverse', '--format=%H|%h|%an|%s|%P',
        f'{last_commit}..{merge_hash or source}'
    ])

    if not log_output:
        return [], False, None

    commits = []
    for line in log_output.split('\n'):
        if not line:
            continue
        parts = line.split('|')
        commit_hash = parts[0]
        short_hash = parts[1]
        author = parts[2]
        subject = '|'.join(parts[3:-1])  # Subject may contain separator

        commits.append(CommitInfo(commit_hash, short_hash, subject, author))

    return commits, bool(merge_hash), None


def do_next_set(args, dbs):
    """Show the next set of commits to cherry-pick from a source

    Args:
        args (Namespace): Parsed arguments with 'source' attribute
        dbs (Database): Database instance

    Returns:
        int: 0 on success, 1 if source not found
    """
    source = args.source
    commits, merge_found, error = get_next_commits(dbs, source)

    if error:
        tout.error(error)
        return 1

    if not commits:
        tout.info('No new commits to cherry-pick')
        return 0

    if merge_found:
        tout.info(f'Next set from {source} ({len(commits)} commits):')
    else:
        tout.info(f'Remaining commits from {source} ({len(commits)} commits, '
                  'no merge found):')

    for commit in commits:
        tout.info(f'  {commit.short_hash} {commit.subject}')

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
        short_hash = parts[1]
        subject = parts[2] if len(parts) > 2 else ''
        merges.append((commit_hash, short_hash, subject))
        if len(merges) >= count:
            break

    tout.info(f'Next {len(merges)} merges from {source}:')
    for i, (_, short_hash, subject) in enumerate(merges, 1):
        tout.info(f'  {i}. {short_hash} {subject}')

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


def format_history_summary(source, commits, branch_name):
    """Format a summary of the cherry-pick operation

    Args:
        source (str): Source branch name
        commits (list): list of CommitInfo tuples
        branch_name (str): Name of the cherry-pick branch

    Returns:
        str: Formatted summary text
    """
    commit_list = '\n'.join(
        f'- {c.short_hash} {c.subject}'
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
    summary = format_history_summary(source, commits, branch_name)
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
    commit_msg = f'pickman: Record cherry-pick of {len(commits)} commits from {source}\n\n'
    commit_msg += '\n'.join(f'- {c.short_hash} {c.subject}' for c in commits)

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
    commits, merge_found, error = get_next_commits(dbs, source)

    if error:
        tout.error(error)
        return None, 1

    if not commits:
        tout.info('No new commits to cherry-pick')
        return None, 0

    # Save current branch to return to later
    original_branch = run_git(['rev-parse', '--abbrev-ref', 'HEAD'])

    # Generate branch name if not provided
    branch_name = branch
    if not branch_name:
        # Use first commit's short hash as part of branch name
        branch_name = f'cherry-{commits[0].short_hash}'

    # Delete branch if it already exists
    try:
        run_git(['rev-parse', '--verify', branch_name])
        tout.info(f'Deleting existing branch {branch_name}')
        run_git(['branch', '-D', branch_name])
    except Exception:  # pylint: disable=broad-except
        pass  # Branch doesn't exist, which is fine

    if merge_found:
        tout.info(f'Applying next set from {source} ({len(commits)} commits):')
    else:
        tout.info(f'Applying remaining commits from {source} '
                  f'({len(commits)} commits, no merge found):')

    tout.info(f'  Branch: {branch_name}')
    for commit in commits:
        tout.info(f'  {commit.short_hash} {commit.subject}')
    tout.info('')

    return ApplyInfo(commits, branch_name, original_branch, merge_found), 0


def execute_apply(dbs, source, commits, branch_name, args):  # pylint: disable=too-many-locals
    """Execute the apply operation: run agent, update database, push MR

    Args:
        dbs (Database): Database instance
        source (str): Source branch name
        commits (list): List of CommitInfo namedtuples
        branch_name (str): Branch name for cherry-picks
        args (Namespace): Parsed arguments with 'push', 'remote', 'target'

    Returns:
        tuple: (ret, success, conv_log) where ret is 0 on success,
            1 on failure
    """
    # Add commits to database with 'pending' status
    source_id = dbs.source_get_id(source)
    for commit in commits:
        dbs.commit_add(commit.hash, source_id, commit.subject, commit.author,
                       status='pending')
    dbs.commit()

    # Convert CommitInfo to tuple format expected by agent
    commit_tuples = [(c.hash, c.short_hash, c.subject) for c in commits]
    success, conv_log = agent.cherry_pick_commits(commit_tuples, source,
                                                          branch_name)

    # Update commit status based on result
    status = 'applied' if success else 'conflict'
    for commit in commits:
        dbs.commit_set_status(commit.hash, status)
    dbs.commit()

    ret = 0 if success else 1

    if success:
        # Push and create MR if requested
        if args.push:
            remote = args.remote
            target = args.target
            # Use merge commit subject as title (last commit is the merge)
            title = f'[pickman] {commits[-1].subject}'
            # Description matches .pickman-history entry (summary + conversation)
            summary = format_history_summary(source, commits, branch_name)
            description = f'{summary}\n\n### Conversation log\n{conv_log}'

            mr_url = gitlab_api.push_and_create_mr(
                remote, branch_name, target, title, description
            )
            if not mr_url:
                ret = 1
        else:
            tout.info(f"Use 'pickman commit-source {source} "
                      f"{commits[-1].short_hash}' to update the database")

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
                                                   branch_name, args)

    # Write history file if successful
    if success:
        write_history(source, commits, branch_name, conv_log)

    # Return to original branch
    current_branch = run_git(['rev-parse', '--abbrev-ref', 'HEAD'])
    if current_branch != original_branch:
        tout.info(f'Returning to {original_branch}')
        run_git(['checkout', original_branch])

    return ret


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


def process_mr_reviews(remote, mrs, dbs):
    """Process review comments on open MRs

    Checks each MR for unresolved comments and uses Claude agent to address
    them. Updates MR description and .pickman-history with conversation log.

    Args:
        remote (str): Remote name
        mrs (list): List of MR dicts from get_open_pickman_mrs()
        dbs (Database): Database instance for tracking processed comments

    Returns:
        int: Number of MRs with comments processed
    """
    processed = 0

    for merge_req in mrs:
        mr_iid = merge_req.iid
        comments = gitlab_api.get_mr_comments(remote, mr_iid)
        if comments is None:
            continue

        # Filter to unresolved comments that haven't been processed
        unresolved = []
        for com in comments:
            if com.resolved:
                continue
            if dbs.comment_is_processed(mr_iid, com.id):
                continue
            unresolved.append(com)
        if not unresolved:
            continue

        tout.info('')
        tout.info(f"MR !{mr_iid} has {len(unresolved)} new comment(s):")
        for comment in unresolved:
            tout.info(f'  [{comment.author}]: {comment.body[:80]}...')

        # Run agent to handle comments
        success, conversation_log = agent.handle_mr_comments(
            mr_iid,
            merge_req.source_branch,
            unresolved,
            remote,
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
            gitlab_api.update_mr_description(remote, mr_iid, new_desc)

            # Update .pickman-history
            update_history_with_review(merge_req.source_branch,
                                       unresolved, conversation_log)

            tout.info(f'Updated MR !{mr_iid} description and history')
        else:
            tout.error(f"Failed to handle comments for MR !{mr_iid}")
        processed += 1

    return processed


def update_history_with_review(branch_name, comments, conversation_log):
    """Append review handling to .pickman-history

    Args:
        branch_name (str): Branch name for the MR
        comments (list): List of comments that were addressed
        conversation_log (str): Agent conversation log
    """
    comment_summary = '\n'.join(
        f"- [{c.author}]: {c.body[:100]}..."
        for c in comments
    )

    entry = f"""### Review: {date.today()}

Branch: {branch_name}

Comments addressed:
{comment_summary}

### Conversation log
{conversation_log}

---

"""

    # Append to history file
    existing = ''
    if os.path.exists(HISTORY_FILE):
        with open(HISTORY_FILE, 'r', encoding='utf-8') as fhandle:
            existing = fhandle.read()

    with open(HISTORY_FILE, 'w', encoding='utf-8') as fhandle:
        fhandle.write(existing + entry)

    # Commit the history file
    run_git(['add', '-f', HISTORY_FILE])
    run_git(['commit', '-m', f'pickman: Record review handling for {branch_name}'])


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
        tuple: (source_branch, last_commit_hash) or (None, None) if not parseable
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
    """Create an MR if none is pending

    Checks for merged pickman MRs and updates the database, then checks for
    open pickman MRs. If open MRs exist, processes any review comments. If no
    open MRs exist, runs apply with push to create a new one.

    Args:
        args (Namespace): Parsed arguments with 'source', 'remote', 'target'
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

    if mrs:
        tout.info(f'Found {len(mrs)} open pickman MR(s):')
        for merge_req in mrs:
            tout.info(f"  !{merge_req.iid}: {merge_req.title}")

        # Process any review comments on open MRs
        process_mr_reviews(remote, mrs, dbs)

        tout.info('')
        tout.info('Not creating new MR while others are pending')
        return 0

    # No pending MRs, run apply with push
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
                tout.warning(f'Step returned {ret}, continuing anyway...')
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
    'commit-source': do_commit_source,
    'compare': do_compare,
    'count-merges': do_count_merges,
    'list-sources': do_list_sources,
    'next-merges': do_next_merges,
    'next-set': do_next_set,
    'poll': do_poll,
    'review': do_review,
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

# SPDX-License-Identifier: GPL-2.0+
#
# Copyright 2025 Canonical Ltd.
# Written by Simon Glass <simon.glass@canonical.com>
#
"""Control module for pickman - handles the main logic."""

from collections import namedtuple
import os
import sys
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
    full_hash, short_hash, subject, date = info.split('\n')

    return count, Commit(full_hash, short_hash, subject, date)


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
    commit in the source branch.

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

    # Get commits between last_commit and source HEAD (oldest first)
    # Format: hash|short_hash|author|subject|parents
    # Using | as separator since subject may contain colons
    log_output = run_git([
        'log', '--reverse', '--format=%H|%h|%an|%s|%P',
        f'{last_commit}..{source}'
    ])

    if not log_output:
        return [], False, None

    commits = []
    merge_found = False

    for line in log_output.split('\n'):
        if not line:
            continue
        parts = line.split('|')
        commit_hash = parts[0]
        short_hash = parts[1]
        author = parts[2]
        subject = '|'.join(parts[3:-1])  # Subject may contain separator
        parents = parts[-1].split()

        commits.append(CommitInfo(commit_hash, short_hash, subject, author))

        # Check if this is a merge commit (has multiple parents)
        if len(parents) > 1:
            merge_found = True
            break

    return commits, merge_found, None


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
    from datetime import date

    commit_list = '\n'.join(
        f'- {c.short_hash} {c.subject}'
        for c in commits
    )

    return f"""## {date.today()}: {source}

Branch: {branch_name}

Commits:
{commit_list}"""


def write_history(source, commits, branch_name, conversation_log):
    """Write an entry to the pickman history file

    Args:
        source (str): Source branch name
        commits (list): list of CommitInfo tuples
        branch_name (str): Name of the cherry-pick branch
        conversation_log (str): The agent's conversation output
    """
    import os
    import re

    summary = format_history_summary(source, commits, branch_name)
    entry = f"""{summary}

### Conversation log
{conversation_log}

---

"""

    # Read existing content and remove any entry for this branch
    existing = ''
    if os.path.exists(HISTORY_FILE):
        with open(HISTORY_FILE, 'r', encoding='utf-8') as fhandle:
            existing = fhandle.read()
        # Remove existing entry for this branch (from ## header to ---)
        pattern = rf'## [^\n]+\n\nBranch: {re.escape(branch_name)}\n.*?---\n\n'
        existing = re.sub(pattern, '', existing, flags=re.DOTALL)

    # Write updated history file
    with open(HISTORY_FILE, 'w', encoding='utf-8') as fhandle:
        fhandle.write(existing + entry)

    # Commit the history file (use -f in case .gitignore patterns match)
    run_git(['add', '-f', HISTORY_FILE])
    msg = f'pickman: Record cherry-pick of {len(commits)} commits from {source}\n\n'
    msg += '\n'.join(f'- {c.short_hash} {c.subject}' for c in commits)
    run_git(['commit', '-m', msg])

    tout.info(f'Updated {HISTORY_FILE}')


def do_apply(args, dbs):  # pylint: disable=too-many-locals
    """Apply the next set of commits using Claude agent

    Args:
        args (Namespace): Parsed arguments with 'source' and 'branch' attributes
        dbs (Database): Database instance

    Returns:
        int: 0 on success, 1 on failure
    """
    source = args.source
    commits, merge_found, error = get_next_commits(dbs, source)

    if error:
        tout.error(error)
        return 1

    if not commits:
        tout.info('No new commits to cherry-pick')
        return 0

    # Save current branch to return to later
    original_branch = run_git(['rev-parse', '--abbrev-ref', 'HEAD'])

    # Generate branch name if not provided
    branch_name = args.branch
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

    # Add commits to database with 'pending' status
    source_id = dbs.source_get_id(source)
    for commit in commits:
        dbs.commit_add(commit.hash, source_id, commit.subject, commit.author,
                       status='pending')
    dbs.commit()

    # Convert CommitInfo to tuple format expected by agent
    commit_tuples = [(c.hash, c.short_hash, c.subject) for c in commits]
    success, conversation_log = agent.cherry_pick_commits(commit_tuples, source,
                                                          branch_name)

    # Update commit status based on result
    status = 'applied' if success else 'conflict'
    for commit in commits:
        dbs.commit_set_status(commit.hash, status)
    dbs.commit()

    # Write history file if successful
    if success:
        write_history(source, commits, branch_name, conversation_log)

    # Return to original branch
    current_branch = run_git(['rev-parse', '--abbrev-ref', 'HEAD'])
    if current_branch != original_branch:
        tout.info(f'Returning to {original_branch}')
        run_git(['checkout', original_branch])

    if success:
        # Push and create MR if requested
        if args.push:
            remote = args.remote
            target = args.target
            # Use merge commit subject as title (last commit is the merge)
            title = f'[pickman] {commits[-1].subject}'
            # Description matches .pickman-history entry (summary + conversation)
            summary = format_history_summary(source, commits, branch_name)
            description = f'{summary}\n\n### Conversation log\n{conversation_log}'

            mr_url = gitlab_api.push_and_create_mr(
                remote, branch_name, target, title, description
            )
            if not mr_url:
                return 1
        else:
            tout.info(f"Use 'pickman commit-source {source} "
                      f"{commits[-1].short_hash}' to update the database")

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


def process_mr_reviews(remote, mrs):
    """Process review comments on open MRs

    Checks each MR for unresolved comments and uses Claude agent to address
    them.

    Args:
        remote (str): Remote name
        mrs (list): List of MR dicts from get_open_pickman_mrs()

    Returns:
        int: Number of MRs with comments processed
    """
    processed = 0

    for merge_req in mrs:
        comments = gitlab_api.get_mr_comments(remote, merge_req['iid'])
        if comments is None:
            continue

        # Filter to unresolved comments
        unresolved = [c for c in comments if not c.get('resolved', True)]
        if not unresolved:
            continue

        tout.info('')
        tout.info(f"MR !{merge_req['iid']} has {len(unresolved)} comment(s):")
        for comment in unresolved:
            tout.info(f"  [{comment['author']}]: {comment['body'][:80]}...")

        # Run agent to handle comments
        success, _ = agent.handle_mr_comments(
            merge_req['iid'],
            merge_req['source_branch'],
            unresolved,
            remote,
        )
        if not success:
            tout.error(f"Failed to handle comments for MR !{merge_req['iid']}")
        processed += 1

    return processed


def do_review(args, dbs):  # pylint: disable=unused-argument
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
        tout.info(f"  !{merge_req['iid']}: {merge_req['title']}")

    process_mr_reviews(remote, mrs)

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
    'list-sources': do_list_sources,
    'next-set': do_next_set,
    'review': do_review,
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

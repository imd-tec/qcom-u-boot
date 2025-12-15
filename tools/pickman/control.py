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
from pickman import database
from pickman import ftest
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
    'compare': do_compare,
    'list-sources': do_list_sources,
    'next-set': do_next_set,
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

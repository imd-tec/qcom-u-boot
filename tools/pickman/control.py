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
from pickman import ftest
from u_boot_pylib import command
from u_boot_pylib import tout

# Branch names to compare
BRANCH_MASTER = 'ci/master'
BRANCH_SOURCE = 'us/next'

# Named tuple for commit info
Commit = namedtuple('Commit', ['hash', 'short_hash', 'subject', 'date'])


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


def do_compare(args):  # pylint: disable=unused-argument
    """Compare branches and print results.

    Args:
        args (Namespace): Parsed arguments
    """
    count, base = compare_branches(BRANCH_MASTER, BRANCH_SOURCE)

    tout.info(f'Commits in {BRANCH_SOURCE} not in {BRANCH_MASTER}: {count}')
    tout.info('')
    tout.info('Last common commit:')
    tout.info(f'  Hash:    {base.short_hash}')
    tout.info(f'  Subject: {base.subject}')
    tout.info(f'  Date:    {base.date}')

    return 0


def do_test(args):  # pylint: disable=unused-argument
    """Run tests for this module.

    Args:
        args (Namespace): Parsed arguments

    Returns:
        int: 0 if tests passed, 1 otherwise
    """
    loader = unittest.TestLoader()
    suite = loader.loadTestsFromModule(ftest)
    runner = unittest.TextTestRunner()
    result = runner.run(suite)

    return 0 if result.wasSuccessful() else 1


def do_pickman(args):
    """Main entry point for pickman commands.

    Args:
        args (Namespace): Parsed arguments

    Returns:
        int: 0 on success, 1 on failure
    """
    tout.init(tout.INFO)

    if args.cmd == 'compare':
        return do_compare(args)
    if args.cmd == 'test':
        return do_test(args)

    return 1

#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0+
#
# Copyright 2025 Canonical Ltd.
# Written by Simon Glass <simon.glass@canonical.com>
#
"""Entry point for pickman - parses arguments and dispatches to control."""

import argparse
import os
import sys
import unittest

# Allow 'from pickman import xxx' to work via symlink
our_path = os.path.dirname(os.path.realpath(__file__))
sys.path.insert(0, os.path.join(our_path, '..'))

# pylint: disable=wrong-import-position,import-error
from pickman import control
from pickman import ftest
from u_boot_pylib import test_util


def parse_args(argv):
    """Parse command line arguments.

    Args:
        argv (list): Command line arguments

    Returns:
        Namespace: Parsed arguments
    """
    parser = argparse.ArgumentParser(description='Check commit differences')
    subparsers = parser.add_subparsers(dest='cmd', required=True)

    add_source = subparsers.add_parser('add-source',
                                        help='Add a source branch to track')
    add_source.add_argument('source', help='Source branch name')

    apply_cmd = subparsers.add_parser('apply',
                                       help='Apply next commits using Claude')
    apply_cmd.add_argument('source', help='Source branch name')
    apply_cmd.add_argument('-b', '--branch', help='Branch name to create')
    apply_cmd.add_argument('-p', '--push', action='store_true',
                           help='Push branch and create GitLab MR')
    apply_cmd.add_argument('-r', '--remote', default='ci',
                           help='Git remote for push (default: ci)')
    apply_cmd.add_argument('-t', '--target', default='master',
                           help='Target branch for MR (default: master)')

    commit_src = subparsers.add_parser('commit-source',
                                        help='Update database with last commit')
    commit_src.add_argument('source', help='Source branch name')
    commit_src.add_argument('commit', help='Commit hash to record')

    subparsers.add_parser('compare', help='Compare branches')
    subparsers.add_parser('list-sources', help='List tracked source branches')

    next_set = subparsers.add_parser('next-set',
                                     help='Show next set of commits to cherry-pick')
    next_set.add_argument('source', help='Source branch name')

    review_cmd = subparsers.add_parser('review',
                                       help='Check open MRs and handle comments')
    review_cmd.add_argument('-r', '--remote', default='ci',
                            help='Git remote (default: ci)')

    step_cmd = subparsers.add_parser('step',
                                     help='Create MR if none pending')
    step_cmd.add_argument('source', help='Source branch name')
    step_cmd.add_argument('-r', '--remote', default='ci',
                          help='Git remote (default: ci)')
    step_cmd.add_argument('-t', '--target', default='master',
                          help='Target branch for MR (default: master)')

    poll_cmd = subparsers.add_parser('poll',
                                     help='Run step repeatedly until stopped')
    poll_cmd.add_argument('source', help='Source branch name')
    poll_cmd.add_argument('-i', '--interval', type=int, default=300,
                          help='Interval between steps in seconds (default: 300)')
    poll_cmd.add_argument('-r', '--remote', default='ci',
                          help='Git remote (default: ci)')
    poll_cmd.add_argument('-t', '--target', default='master',
                          help='Target branch for MR (default: master)')

    test_cmd = subparsers.add_parser('test', help='Run tests')
    test_cmd.add_argument('-P', '--processes', type=int,
                          help='Number of processes to run tests (default: all)')
    test_cmd.add_argument('-T', '--test-coverage', action='store_true',
                          help='Run tests and check for 100%% coverage')
    test_cmd.add_argument('-v', '--verbosity', type=int, default=1,
                          help='Verbosity level (0-4, default: 1)')
    test_cmd.add_argument('tests', nargs='*', help='Specific tests to run')

    return parser.parse_args(argv)


def get_test_classes():
    """Get all test classes from the ftest module.

    Returns:
        list: List of test class objects
    """
    return [getattr(ftest, name) for name in dir(ftest)
            if name.startswith('Test') and
            isinstance(getattr(ftest, name), type) and
            issubclass(getattr(ftest, name), unittest.TestCase)]


def run_tests(processes, verbosity, test_name):
    """Run the pickman test suite.

    Args:
        processes (int): Number of processes for concurrent tests
        verbosity (int): Verbosity level (0-4)
        test_name (str): Specific test to run, or None for all

    Returns:
        int: 0 if tests passed, 1 otherwise
    """
    result = test_util.run_test_suites(
        'pickman', False, verbosity, False, False, processes,
        test_name, None, get_test_classes())

    return 0 if result.wasSuccessful() else 1


def run_test_coverage(args):
    """Run tests with coverage checking.

    Args:
        args (list): Specific tests to run, or None for all
    """
    # agent.py and gitlab_api.py require external services (Claude, GitLab)
    # so they can't achieve 100% coverage in unit tests
    test_util.run_test_coverage(
        'tools/pickman/pickman', None,
        ['*test*', '*__main__.py', 'tools/u_boot_pylib/*'],
        None, extra_args=None, args=args,
        allow_failures=['tools/pickman/agent.py',
                        'tools/pickman/gitlab_api.py',
                        'tools/pickman/control.py'])


def main(argv=None):
    """Main function to parse args and run commands.

    Args:
        argv (list): Command line arguments (None for sys.argv[1:])
    """
    args = parse_args(argv)

    if args.cmd == 'test':
        if args.test_coverage:
            run_test_coverage(args.tests or None)
            return 0
        test_name = args.tests[0] if args.tests else None
        return run_tests(args.processes, args.verbosity, test_name)

    return control.do_pickman(args)


if __name__ == '__main__':
    sys.exit(main())

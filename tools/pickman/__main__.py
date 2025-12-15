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

# Allow 'from pickman import xxx' to work via symlink
our_path = os.path.dirname(os.path.realpath(__file__))
sys.path.insert(0, os.path.join(our_path, '..'))

# pylint: disable=wrong-import-position,import-error
from pickman import control


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

    subparsers.add_parser('test', help='Run tests')

    return parser.parse_args(argv)


def main(argv=None):
    """Main function to parse args and run commands.

    Args:
        argv (list): Command line arguments (None for sys.argv[1:])
    """
    args = parse_args(argv)
    return control.do_pickman(args)


if __name__ == '__main__':
    sys.exit(main())

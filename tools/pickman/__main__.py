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

    subparsers.add_parser('compare', help='Compare branches')
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

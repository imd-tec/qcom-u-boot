# SPDX-License-Identifier: GPL-2.0+
#
# Copyright 2025 Canonical Ltd.
# Written by Simon Glass <simon.glass@canonical.com>
#
"""Tests for pickman."""

import os
import sys
import tempfile
import unittest

# Allow 'from pickman import xxx' to work via symlink
our_path = os.path.dirname(os.path.realpath(__file__))
sys.path.insert(0, os.path.join(our_path, '..'))

# pylint: disable=wrong-import-position,import-error,cyclic-import
from u_boot_pylib import command
from u_boot_pylib import terminal

from pickman import __main__ as pickman
from pickman import control
from pickman import database


class TestCommit(unittest.TestCase):
    """Tests for the Commit namedtuple."""

    def test_commit_fields(self):
        """Test Commit namedtuple has correct fields."""
        commit = control.Commit(
            'abc123def456',
            'abc123d',
            'Test commit subject',
            '2024-01-15 10:30:00 -0600'
        )
        self.assertEqual(commit.hash, 'abc123def456')
        self.assertEqual(commit.short_hash, 'abc123d')
        self.assertEqual(commit.subject, 'Test commit subject')
        self.assertEqual(commit.date, '2024-01-15 10:30:00 -0600')


class TestRunGit(unittest.TestCase):
    """Tests for run_git function."""

    def test_run_git(self):
        """Test run_git returns stripped output."""
        result = command.CommandResult(stdout='  output with spaces  \n')
        command.TEST_RESULT = result
        try:
            out = control.run_git(['status'])
            self.assertEqual(out, 'output with spaces')
        finally:
            command.TEST_RESULT = None


class TestCompareBranches(unittest.TestCase):
    """Tests for compare_branches function."""

    def test_compare_branches(self):
        """Test compare_branches returns correct count and commit."""
        results = iter([
            '42',  # rev-list --count
            'abc123def456789',  # merge-base
            'abc123def456789\nabc123d\nTest subject\n2024-01-15 10:30:00 -0600',
        ])

        def handle_command(**_):
            return command.CommandResult(stdout=next(results))

        command.TEST_RESULT = handle_command
        try:
            count, commit = control.compare_branches('master', 'source')

            self.assertEqual(count, 42)
            self.assertEqual(commit.hash, 'abc123def456789')
            self.assertEqual(commit.short_hash, 'abc123d')
            self.assertEqual(commit.subject, 'Test subject')
            self.assertEqual(commit.date, '2024-01-15 10:30:00 -0600')
        finally:
            command.TEST_RESULT = None

    def test_compare_branches_zero_commits(self):
        """Test compare_branches with zero commit difference."""
        results = iter([
            '0',
            'def456abc789',
            'def456abc789\ndef456a\nMerge commit\n2024-02-20 14:00:00 -0500',
        ])

        def handle_command(**_):
            return command.CommandResult(stdout=next(results))

        command.TEST_RESULT = handle_command
        try:
            count, commit = control.compare_branches('branch1', 'branch2')

            self.assertEqual(count, 0)
            self.assertEqual(commit.short_hash, 'def456a')
        finally:
            command.TEST_RESULT = None


class TestParseArgs(unittest.TestCase):
    """Tests for parse_args function."""

    def test_parse_compare(self):
        """Test parsing compare command."""
        args = pickman.parse_args(['compare'])
        self.assertEqual(args.cmd, 'compare')

    def test_parse_test(self):
        """Test parsing test command."""
        args = pickman.parse_args(['test'])
        self.assertEqual(args.cmd, 'test')

    def test_parse_no_command(self):
        """Test parsing with no command raises error."""
        with terminal.capture():
            with self.assertRaises(SystemExit):
                pickman.parse_args([])


class TestMain(unittest.TestCase):
    """Tests for main function."""

    def test_main_compare(self):
        """Test main with compare command."""
        results = iter([
            '10',
            'abc123',
            'abc123\nabc\nSubject\n2024-01-01 00:00:00 -0000',
        ])

        def handle_command(**_):
            return command.CommandResult(stdout=next(results))

        command.TEST_RESULT = handle_command
        try:
            with terminal.capture() as (stdout, _):
                ret = pickman.main(['compare'])
            self.assertEqual(ret, 0)
            lines = iter(stdout.getvalue().splitlines())
            self.assertEqual('Commits in us/next not in ci/master: 10',
                             next(lines))
            self.assertEqual('', next(lines))
            self.assertEqual('Last common commit:', next(lines))
            self.assertEqual('  Hash:    abc', next(lines))
            self.assertEqual('  Subject: Subject', next(lines))
            self.assertEqual('  Date:    2024-01-01 00:00:00 -0000',
                             next(lines))
            self.assertRaises(StopIteration, next, lines)
        finally:
            command.TEST_RESULT = None


class TestDatabase(unittest.TestCase):
    """Tests for Database class."""

    def setUp(self):
        """Set up test fixtures."""
        fd, self.db_path = tempfile.mkstemp(suffix='.db')
        os.close(fd)
        os.unlink(self.db_path)  # Remove so database creates it fresh
        database.Database.instances.clear()

    def tearDown(self):
        """Clean up test fixtures."""
        if os.path.exists(self.db_path):
            os.unlink(self.db_path)
        database.Database.instances.clear()

    def test_create_database(self):
        """Test creating a new database."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            self.assertTrue(dbs.is_open)
            self.assertEqual(dbs.get_schema_version(), database.LATEST)
            dbs.close()

    def test_source_get_empty(self):
        """Test getting source from empty database."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            result = dbs.source_get('us/next')
            self.assertIsNone(result)
            dbs.close()

    def test_source_set_and_get(self):
        """Test setting and getting source commit."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'abc123def456')
            dbs.commit()
            result = dbs.source_get('us/next')
            self.assertEqual(result, 'abc123def456')
            dbs.close()

    def test_source_update(self):
        """Test updating source commit."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'abc123')
            dbs.commit()
            dbs.source_set('us/next', 'def456')
            dbs.commit()
            result = dbs.source_get('us/next')
            self.assertEqual(result, 'def456')
            dbs.close()

    def test_get_instance(self):
        """Test get_instance returns same database."""
        with terminal.capture():
            dbs1, created1 = database.Database.get_instance(self.db_path)
            dbs1.start()
            dbs2, created2 = database.Database.get_instance(self.db_path)
            self.assertTrue(created1)
            self.assertFalse(created2)
            self.assertIs(dbs1, dbs2)
            dbs1.close()


if __name__ == '__main__':
    unittest.main()

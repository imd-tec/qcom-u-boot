# SPDX-License-Identifier: GPL-2.0+
#
# Copyright 2025 Canonical Ltd.
# Written by Simon Glass <simon.glass@canonical.com>
#
"""Tests for pickman."""

import argparse
import os
import sys
import tempfile
import unittest
from unittest import mock

# Allow 'from pickman import xxx' to work via symlink
our_path = os.path.dirname(os.path.realpath(__file__))
sys.path.insert(0, os.path.join(our_path, '..'))

# pylint: disable=wrong-import-position,import-error,cyclic-import
from u_boot_pylib import command
from u_boot_pylib import terminal

from pickman import __main__ as pickman
from pickman import control
from pickman import database
from pickman import gitlab_api


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

    def test_parse_add_source(self):
        """Test parsing add-source command."""
        args = pickman.parse_args(['add-source', 'us/next'])
        self.assertEqual(args.cmd, 'add-source')
        self.assertEqual(args.source, 'us/next')

    def test_parse_apply(self):
        """Test parsing apply command."""
        args = pickman.parse_args(['apply', 'us/next'])
        self.assertEqual(args.cmd, 'apply')
        self.assertEqual(args.source, 'us/next')
        self.assertIsNone(args.branch)

    def test_parse_apply_with_branch(self):
        """Test parsing apply command with branch."""
        args = pickman.parse_args(['apply', 'us/next', '-b', 'my-branch'])
        self.assertEqual(args.cmd, 'apply')
        self.assertEqual(args.source, 'us/next')
        self.assertEqual(args.branch, 'my-branch')

    def test_parse_commit_source(self):
        """Test parsing commit-source command."""
        args = pickman.parse_args(['commit-source', 'us/next', 'abc123'])
        self.assertEqual(args.cmd, 'commit-source')
        self.assertEqual(args.source, 'us/next')
        self.assertEqual(args.commit, 'abc123')

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

    def test_add_source(self):
        """Test add-source command"""
        results = iter([
            'abc123def456',  # merge-base
            'abc123d\nTest subject',  # log
        ])

        def handle_command(**_):
            return command.CommandResult(stdout=next(results))

        # Use a temp database file
        fd, db_path = tempfile.mkstemp(suffix='.db')
        os.close(fd)
        os.unlink(db_path)
        old_db_fname = control.DB_FNAME
        control.DB_FNAME = db_path
        database.Database.instances.clear()

        command.TEST_RESULT = handle_command
        try:
            args = argparse.Namespace(cmd='add-source', source='us/next')
            with terminal.capture() as (stdout, _):
                ret = control.do_pickman(args)
            self.assertEqual(ret, 0)
            output = stdout.getvalue()
            self.assertIn("Added source 'us/next' with base commit:", output)
            self.assertIn('Hash:    abc123d', output)
            self.assertIn('Subject: Test subject', output)

            # Verify database was updated
            database.Database.instances.clear()
            dbs = database.Database(db_path)
            dbs.start()
            self.assertEqual(dbs.source_get('us/next'), 'abc123def456')
            dbs.close()
        finally:
            command.TEST_RESULT = None
            control.DB_FNAME = old_db_fname
            if os.path.exists(db_path):
                os.unlink(db_path)
            database.Database.instances.clear()

    def test_main_compare(self):
        """Test main with compare command."""
        results = iter([
            '10',
            'abc123',
            'abc123\nabc\nSubject\n2024-01-01 00:00:00 -0000',
        ])

        def handle_command(**_):
            return command.CommandResult(stdout=next(results))

        # Use a temp database file
        fd, db_path = tempfile.mkstemp(suffix='.db')
        os.close(fd)
        os.unlink(db_path)
        old_db_fname = control.DB_FNAME
        control.DB_FNAME = db_path
        database.Database.instances.clear()

        command.TEST_RESULT = handle_command
        try:
            with terminal.capture() as (stdout, _):
                ret = pickman.main(['compare'])
            self.assertEqual(ret, 0)
            # Filter out database migration messages
            output_lines = [l for l in stdout.getvalue().splitlines()
                            if not l.startswith(('Update database', 'Creating'))]
            lines = iter(output_lines)
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
            control.DB_FNAME = old_db_fname
            if os.path.exists(db_path):
                os.unlink(db_path)
            database.Database.instances.clear()


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

    def test_source_get_all(self):
        """Test getting all sources."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()

            # Empty initially
            self.assertEqual(dbs.source_get_all(), [])

            # Add some sources
            dbs.source_set('branch-a', 'abc123')
            dbs.source_set('branch-b', 'def456')
            dbs.commit()

            # Should be sorted by name
            sources = dbs.source_get_all()
            self.assertEqual(len(sources), 2)
            self.assertEqual(sources[0], ('branch-a', 'abc123'))
            self.assertEqual(sources[1], ('branch-b', 'def456'))
            dbs.close()


class TestDatabaseCommit(unittest.TestCase):
    """Tests for Database commit functions."""

    def setUp(self):
        """Set up test fixtures."""
        fd, self.db_path = tempfile.mkstemp(suffix='.db')
        os.close(fd)
        os.unlink(self.db_path)
        database.Database.instances.clear()

    def tearDown(self):
        """Clean up test fixtures."""
        if os.path.exists(self.db_path):
            os.unlink(self.db_path)
        database.Database.instances.clear()

    def test_commit_add_and_get(self):
        """Test adding and getting a commit."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()

            # First add a source
            dbs.source_set('us/next', 'base123')
            dbs.commit()
            source_id = dbs.source_get_id('us/next')

            # Add a commit
            dbs.commit_add('abc123def456', source_id, 'Test subject',
                           'Author Name')
            dbs.commit()

            # Get the commit
            result = dbs.commit_get('abc123def456')
            self.assertIsNotNone(result)
            self.assertEqual(result[1], 'abc123def456')  # chash
            self.assertEqual(result[2], source_id)  # source_id
            self.assertIsNone(result[3])  # mergereq_id
            self.assertEqual(result[4], 'Test subject')  # subject
            self.assertEqual(result[5], 'Author Name')  # author
            self.assertEqual(result[6], 'pending')  # status
            dbs.close()

    def test_commit_get_not_found(self):
        """Test getting a non-existent commit."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            result = dbs.commit_get('nonexistent')
            self.assertIsNone(result)
            dbs.close()

    def test_commit_get_by_source(self):
        """Test getting commits by source."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()

            # Add a source
            dbs.source_set('us/next', 'base123')
            dbs.commit()
            source_id = dbs.source_get_id('us/next')

            # Add commits
            dbs.commit_add('commit1', source_id, 'Subject 1', 'Author 1')
            dbs.commit_add('commit2', source_id, 'Subject 2', 'Author 2',
                           status='applied')
            dbs.commit_add('commit3', source_id, 'Subject 3', 'Author 3')
            dbs.commit()

            # Get all commits for source
            commits = dbs.commit_get_by_source(source_id)
            self.assertEqual(len(commits), 3)

            # Get only pending commits
            pending = dbs.commit_get_by_source(source_id, status='pending')
            self.assertEqual(len(pending), 2)

            # Get only applied commits
            applied = dbs.commit_get_by_source(source_id, status='applied')
            self.assertEqual(len(applied), 1)
            self.assertEqual(applied[0][1], 'commit2')
            dbs.close()

    def test_commit_set_status(self):
        """Test updating commit status."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()

            dbs.source_set('us/next', 'base123')
            dbs.commit()
            source_id = dbs.source_get_id('us/next')

            dbs.commit_add('abc123', source_id, 'Subject', 'Author')
            dbs.commit()

            # Update status
            dbs.commit_set_status('abc123', 'applied')
            dbs.commit()

            result = dbs.commit_get('abc123')
            self.assertEqual(result[6], 'applied')
            dbs.close()

    def test_commit_set_status_with_cherry_hash(self):
        """Test updating commit status with cherry hash."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()

            dbs.source_set('us/next', 'base123')
            dbs.commit()
            source_id = dbs.source_get_id('us/next')

            dbs.commit_add('abc123', source_id, 'Subject', 'Author')
            dbs.commit()

            # Update status with cherry hash
            dbs.commit_set_status('abc123', 'applied', cherry_hash='xyz789')
            dbs.commit()

            result = dbs.commit_get('abc123')
            self.assertEqual(result[6], 'applied')
            self.assertEqual(result[7], 'xyz789')  # cherry_hash
            dbs.close()

    def test_source_get_id(self):
        """Test getting source id by name."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()

            # Not found initially
            self.assertIsNone(dbs.source_get_id('us/next'))

            # Add source and get id
            dbs.source_set('us/next', 'abc123')
            dbs.commit()

            source_id = dbs.source_get_id('us/next')
            self.assertIsNotNone(source_id)
            self.assertIsInstance(source_id, int)
            dbs.close()


class TestDatabaseMergereq(unittest.TestCase):
    """Tests for Database mergereq functions."""

    def setUp(self):
        """Set up test fixtures."""
        fd, self.db_path = tempfile.mkstemp(suffix='.db')
        os.close(fd)
        os.unlink(self.db_path)
        database.Database.instances.clear()

    def tearDown(self):
        """Clean up test fixtures."""
        if os.path.exists(self.db_path):
            os.unlink(self.db_path)
        database.Database.instances.clear()

    def test_mergereq_add_and_get(self):
        """Test adding and getting a merge request."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()

            # Add a source
            dbs.source_set('us/next', 'base123')
            dbs.commit()
            source_id = dbs.source_get_id('us/next')

            # Add a merge request
            dbs.mergereq_add(source_id, 'cherry-abc123', 42, 'open',
                             'https://gitlab.com/mr/42', '2025-01-15')
            dbs.commit()

            # Get the merge request
            result = dbs.mergereq_get(42)
            self.assertIsNotNone(result)
            self.assertEqual(result[1], source_id)  # source_id
            self.assertEqual(result[2], 'cherry-abc123')  # branch_name
            self.assertEqual(result[3], 42)  # mr_id
            self.assertEqual(result[4], 'open')  # status
            self.assertEqual(result[5], 'https://gitlab.com/mr/42')  # url
            self.assertEqual(result[6], '2025-01-15')  # created_at
            dbs.close()

    def test_mergereq_get_not_found(self):
        """Test getting a non-existent merge request."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            result = dbs.mergereq_get(999)
            self.assertIsNone(result)
            dbs.close()

    def test_mergereq_get_by_source(self):
        """Test getting merge requests by source."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()

            # Add a source
            dbs.source_set('us/next', 'base123')
            dbs.commit()
            source_id = dbs.source_get_id('us/next')

            # Add merge requests
            dbs.mergereq_add(source_id, 'branch-1', 1, 'open',
                             'https://gitlab.com/mr/1', '2025-01-01')
            dbs.mergereq_add(source_id, 'branch-2', 2, 'merged',
                             'https://gitlab.com/mr/2', '2025-01-02')
            dbs.mergereq_add(source_id, 'branch-3', 3, 'open',
                             'https://gitlab.com/mr/3', '2025-01-03')
            dbs.commit()

            # Get all merge requests for source
            mrs = dbs.mergereq_get_by_source(source_id)
            self.assertEqual(len(mrs), 3)

            # Get only open merge requests
            open_mrs = dbs.mergereq_get_by_source(source_id, status='open')
            self.assertEqual(len(open_mrs), 2)

            # Get only merged
            merged = dbs.mergereq_get_by_source(source_id, status='merged')
            self.assertEqual(len(merged), 1)
            self.assertEqual(merged[0][3], 2)  # mr_id
            dbs.close()

    def test_mergereq_set_status(self):
        """Test updating merge request status."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()

            dbs.source_set('us/next', 'base123')
            dbs.commit()
            source_id = dbs.source_get_id('us/next')

            dbs.mergereq_add(source_id, 'branch-1', 42, 'open',
                             'https://gitlab.com/mr/42', '2025-01-15')
            dbs.commit()

            # Update status
            dbs.mergereq_set_status(42, 'merged')
            dbs.commit()

            result = dbs.mergereq_get(42)
            self.assertEqual(result[4], 'merged')
            dbs.close()


class TestDatabaseCommitMergereq(unittest.TestCase):
    """Tests for commit-mergereq relationship."""

    def setUp(self):
        """Set up test fixtures."""
        fd, self.db_path = tempfile.mkstemp(suffix='.db')
        os.close(fd)
        os.unlink(self.db_path)
        database.Database.instances.clear()

    def tearDown(self):
        """Clean up test fixtures."""
        if os.path.exists(self.db_path):
            os.unlink(self.db_path)
        database.Database.instances.clear()

    def test_commit_set_mergereq(self):
        """Test setting merge request for a commit."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()

            # Add source
            dbs.source_set('us/next', 'base123')
            dbs.commit()
            source_id = dbs.source_get_id('us/next')

            # Add merge request
            dbs.mergereq_add(source_id, 'branch-1', 42, 'open',
                             'https://gitlab.com/mr/42', '2025-01-15')
            dbs.commit()
            mr = dbs.mergereq_get(42)
            mr_id = mr[0]  # id field

            # Add commit without mergereq
            dbs.commit_add('abc123', source_id, 'Subject', 'Author')
            dbs.commit()

            # Set mergereq
            dbs.commit_set_mergereq('abc123', mr_id)
            dbs.commit()

            result = dbs.commit_get('abc123')
            self.assertEqual(result[3], mr_id)  # mergereq_id
            dbs.close()

    def test_commit_get_by_mergereq(self):
        """Test getting commits by merge request."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()

            # Add source
            dbs.source_set('us/next', 'base123')
            dbs.commit()
            source_id = dbs.source_get_id('us/next')

            # Add merge request
            dbs.mergereq_add(source_id, 'branch-1', 42, 'open',
                             'https://gitlab.com/mr/42', '2025-01-15')
            dbs.commit()
            mr = dbs.mergereq_get(42)
            mr_id = mr[0]

            # Add commits with mergereq_id
            dbs.commit_add('commit1', source_id, 'Subject 1', 'Author 1',
                           mergereq_id=mr_id)
            dbs.commit_add('commit2', source_id, 'Subject 2', 'Author 2',
                           mergereq_id=mr_id)
            dbs.commit_add('commit3', source_id, 'Subject 3', 'Author 3')
            dbs.commit()

            # Get commits for merge request
            commits = dbs.commit_get_by_mergereq(mr_id)
            self.assertEqual(len(commits), 2)
            hashes = [c[1] for c in commits]
            self.assertIn('commit1', hashes)
            self.assertIn('commit2', hashes)
            self.assertNotIn('commit3', hashes)
            dbs.close()


class TestListSources(unittest.TestCase):
    """Tests for list-sources command."""

    def setUp(self):
        """Set up test fixtures."""
        fd, self.db_path = tempfile.mkstemp(suffix='.db')
        os.close(fd)
        os.unlink(self.db_path)
        self.old_db_fname = control.DB_FNAME
        control.DB_FNAME = self.db_path
        database.Database.instances.clear()

    def tearDown(self):
        """Clean up test fixtures."""
        control.DB_FNAME = self.old_db_fname
        if os.path.exists(self.db_path):
            os.unlink(self.db_path)
        database.Database.instances.clear()

    def test_list_sources_empty(self):
        """Test list-sources with no sources"""
        args = argparse.Namespace(cmd='list-sources')
        with terminal.capture() as (stdout, _):
            ret = control.do_pickman(args)
        self.assertEqual(ret, 0)
        self.assertIn('No source branches tracked', stdout.getvalue())

    def test_list_sources(self):
        """Test list-sources with sources"""
        # Add some sources first
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'abc123def456')
            dbs.source_set('other/branch', 'def456abc789')
            dbs.commit()
            dbs.close()

        database.Database.instances.clear()
        args = argparse.Namespace(cmd='list-sources')
        with terminal.capture() as (stdout, _):
            ret = control.do_pickman(args)
        self.assertEqual(ret, 0)
        output = stdout.getvalue()
        self.assertIn('Tracked source branches:', output)
        self.assertIn('other/branch: def456abc789', output)
        self.assertIn('us/next: abc123def456', output)


class TestNextSet(unittest.TestCase):
    """Tests for next-set command."""

    def setUp(self):
        """Set up test fixtures."""
        fd, self.db_path = tempfile.mkstemp(suffix='.db')
        os.close(fd)
        os.unlink(self.db_path)
        self.old_db_fname = control.DB_FNAME
        control.DB_FNAME = self.db_path
        database.Database.instances.clear()

    def tearDown(self):
        """Clean up test fixtures."""
        control.DB_FNAME = self.old_db_fname
        if os.path.exists(self.db_path):
            os.unlink(self.db_path)
        database.Database.instances.clear()
        command.TEST_RESULT = None

    def test_next_set_source_not_found(self):
        """Test next-set with unknown source"""
        # Create empty database first
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.close()

        database.Database.instances.clear()

        args = argparse.Namespace(cmd='next-set', source='unknown')
        with terminal.capture() as (_, stderr):
            ret = control.do_pickman(args)
        self.assertEqual(ret, 1)
        # Error goes to stderr
        self.assertIn("Source 'unknown' not found", stderr.getvalue())

    def test_next_set_no_commits(self):
        """Test next-set with no new commits"""
        # Add source to database
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'abc123')
            dbs.commit()
            dbs.close()

        database.Database.instances.clear()

        # Mock git log returning empty
        command.TEST_RESULT = command.CommandResult(stdout='')

        args = argparse.Namespace(cmd='next-set', source='us/next')
        with terminal.capture() as (stdout, _):
            ret = control.do_pickman(args)
        self.assertEqual(ret, 0)
        self.assertIn('No new commits to cherry-pick', stdout.getvalue())

    def test_next_set_with_merge(self):
        """Test next-set finding commits up to merge"""
        # Add source to database
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'abc123')
            dbs.commit()
            dbs.close()

        database.Database.instances.clear()

        # Mock git log with commits including a merge
        log_output = (
            'aaa111|aaa111a|Author 1|First commit|abc123\n'
            'bbb222|bbb222b|Author 2|Second commit|aaa111\n'
            'ccc333|ccc333c|Author 3|Merge branch feature|bbb222 ddd444\n'
            'eee555|eee555e|Author 4|After merge|ccc333\n'
        )
        command.TEST_RESULT = command.CommandResult(stdout=log_output)

        args = argparse.Namespace(cmd='next-set', source='us/next')
        with terminal.capture() as (stdout, _):
            ret = control.do_pickman(args)
        self.assertEqual(ret, 0)
        output = stdout.getvalue()
        self.assertIn('Next set from us/next (3 commits):', output)
        self.assertIn('aaa111a First commit', output)
        self.assertIn('bbb222b Second commit', output)
        self.assertIn('ccc333c Merge branch feature', output)
        # Should not include commits after the merge
        self.assertNotIn('eee555e', output)

    def test_next_set_no_merge(self):
        """Test next-set with no merge commit found"""
        # Add source to database
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'abc123')
            dbs.commit()
            dbs.close()

        database.Database.instances.clear()

        # Mock git log without merge commits
        log_output = (
            'aaa111|aaa111a|Author 1|First commit|abc123\n'
            'bbb222|bbb222b|Author 2|Second commit|aaa111\n'
        )
        command.TEST_RESULT = command.CommandResult(stdout=log_output)

        args = argparse.Namespace(cmd='next-set', source='us/next')
        with terminal.capture() as (stdout, _):
            ret = control.do_pickman(args)
        self.assertEqual(ret, 0)
        output = stdout.getvalue()
        self.assertIn('Remaining commits from us/next (2 commits, '
                      'no merge found):', output)
        self.assertIn('aaa111a First commit', output)
        self.assertIn('bbb222b Second commit', output)


class TestGetNextCommits(unittest.TestCase):
    """Tests for get_next_commits function."""

    def setUp(self):
        """Set up test fixtures."""
        fd, self.db_path = tempfile.mkstemp(suffix='.db')
        os.close(fd)
        os.unlink(self.db_path)
        self.old_db_fname = control.DB_FNAME
        control.DB_FNAME = self.db_path
        database.Database.instances.clear()

    def tearDown(self):
        """Clean up test fixtures."""
        control.DB_FNAME = self.old_db_fname
        if os.path.exists(self.db_path):
            os.unlink(self.db_path)
        database.Database.instances.clear()
        command.TEST_RESULT = None

    def test_get_next_commits_source_not_found(self):
        """Test get_next_commits with unknown source"""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            commits, merge_found, error = control.get_next_commits(dbs,
                                                                   'unknown')
            self.assertIsNone(commits)
            self.assertFalse(merge_found)
            self.assertIn('not found', error)
            dbs.close()

    def test_get_next_commits_with_merge(self):
        """Test get_next_commits finding commits up to merge"""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'abc123')
            dbs.commit()

            log_output = (
                'aaa111|aaa111a|Author 1|First commit|abc123\n'
                'bbb222|bbb222b|Author 2|Merge branch|aaa111 ccc333\n'
            )
            command.TEST_RESULT = command.CommandResult(stdout=log_output)

            commits, merge_found, error = control.get_next_commits(dbs,
                                                                   'us/next')
            self.assertIsNone(error)
            self.assertTrue(merge_found)
            self.assertEqual(len(commits), 2)
            self.assertEqual(commits[0].short_hash, 'aaa111a')
            self.assertEqual(commits[1].short_hash, 'bbb222b')
            dbs.close()


class TestApply(unittest.TestCase):
    """Tests for apply command."""

    def setUp(self):
        """Set up test fixtures."""
        fd, self.db_path = tempfile.mkstemp(suffix='.db')
        os.close(fd)
        os.unlink(self.db_path)
        self.old_db_fname = control.DB_FNAME
        control.DB_FNAME = self.db_path
        database.Database.instances.clear()

    def tearDown(self):
        """Clean up test fixtures."""
        control.DB_FNAME = self.old_db_fname
        if os.path.exists(self.db_path):
            os.unlink(self.db_path)
        database.Database.instances.clear()
        command.TEST_RESULT = None

    def test_apply_source_not_found(self):
        """Test apply with unknown source"""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.close()

        database.Database.instances.clear()

        args = argparse.Namespace(cmd='apply', source='unknown')
        with terminal.capture() as (_, stderr):
            ret = control.do_pickman(args)
        self.assertEqual(ret, 1)
        self.assertIn("Source 'unknown' not found", stderr.getvalue())

    def test_apply_no_commits(self):
        """Test apply with no new commits"""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'abc123')
            dbs.commit()
            dbs.close()

        database.Database.instances.clear()
        command.TEST_RESULT = command.CommandResult(stdout='')

        args = argparse.Namespace(cmd='apply', source='us/next')
        with terminal.capture() as (stdout, _):
            ret = control.do_pickman(args)
        self.assertEqual(ret, 0)
        self.assertIn('No new commits to cherry-pick', stdout.getvalue())


class TestCommitSource(unittest.TestCase):
    """Tests for commit-source command."""

    def setUp(self):
        """Set up test fixtures."""
        fd, self.db_path = tempfile.mkstemp(suffix='.db')
        os.close(fd)
        os.unlink(self.db_path)
        self.old_db_fname = control.DB_FNAME
        control.DB_FNAME = self.db_path
        database.Database.instances.clear()

    def tearDown(self):
        """Clean up test fixtures."""
        control.DB_FNAME = self.old_db_fname
        if os.path.exists(self.db_path):
            os.unlink(self.db_path)
        database.Database.instances.clear()
        command.TEST_RESULT = None

    def test_commit_source_not_found(self):
        """Test commit-source with unknown source."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.close()

        database.Database.instances.clear()
        command.TEST_RESULT = command.CommandResult(stdout='fullhash123')

        args = argparse.Namespace(cmd='commit-source', source='unknown',
                                  commit='abc123')
        with terminal.capture() as (_, stderr):
            ret = control.do_pickman(args)
        self.assertEqual(ret, 1)
        self.assertIn("Source 'unknown' not found", stderr.getvalue())

    def test_commit_source_success(self):
        """Test commit-source updates database."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'oldcommit12345')
            dbs.commit()
            dbs.close()

        database.Database.instances.clear()
        command.TEST_RESULT = command.CommandResult(stdout='newcommit67890')

        args = argparse.Namespace(cmd='commit-source', source='us/next',
                                  commit='abc123')
        with terminal.capture() as (stdout, _):
            ret = control.do_pickman(args)
        self.assertEqual(ret, 0)
        self.assertIn('oldcommit123', stdout.getvalue())
        self.assertIn('newcommit678', stdout.getvalue())


class TestParseUrl(unittest.TestCase):
    """Tests for parse_url function."""

    def test_parse_ssh_url(self):
        """Test parsing SSH URL."""
        host, path = gitlab_api.parse_url(
            'git@gitlab.com:group/project.git')
        self.assertEqual(host, 'gitlab.com')
        self.assertEqual(path, 'group/project')

    def test_parse_ssh_url_no_git_suffix(self):
        """Test parsing SSH URL without .git suffix."""
        host, path = gitlab_api.parse_url(
            'git@gitlab.com:group/project')
        self.assertEqual(host, 'gitlab.com')
        self.assertEqual(path, 'group/project')

    def test_parse_ssh_url_nested_group(self):
        """Test parsing SSH URL with nested group."""
        host, path = gitlab_api.parse_url(
            'git@gitlab.denx.de:u-boot/custodians/u-boot-dm.git')
        self.assertEqual(host, 'gitlab.denx.de')
        self.assertEqual(path, 'u-boot/custodians/u-boot-dm')

    def test_parse_https_url(self):
        """Test parsing HTTPS URL."""
        host, path = gitlab_api.parse_url(
            'https://gitlab.com/group/project.git')
        self.assertEqual(host, 'gitlab.com')
        self.assertEqual(path, 'group/project')

    def test_parse_https_url_no_git_suffix(self):
        """Test parsing HTTPS URL without .git suffix."""
        host, path = gitlab_api.parse_url(
            'https://gitlab.com/group/project')
        self.assertEqual(host, 'gitlab.com')
        self.assertEqual(path, 'group/project')

    def test_parse_http_url(self):
        """Test parsing HTTP URL."""
        host, path = gitlab_api.parse_url(
            'http://gitlab.example.com/group/project.git')
        self.assertEqual(host, 'gitlab.example.com')
        self.assertEqual(path, 'group/project')

    def test_parse_invalid_url(self):
        """Test parsing invalid URL."""
        host, path = gitlab_api.parse_url('not-a-valid-url')
        self.assertIsNone(host)
        self.assertIsNone(path)

    def test_parse_empty_url(self):
        """Test parsing empty URL."""
        host, path = gitlab_api.parse_url('')
        self.assertIsNone(host)
        self.assertIsNone(path)


class TestCheckAvailable(unittest.TestCase):
    """Tests for GitLab availability checks."""

    def test_check_available_false(self):
        """Test check_available returns False when gitlab not installed."""
        with mock.patch.object(gitlab_api, 'AVAILABLE', False):
            result = gitlab_api.check_available()
            self.assertFalse(result)

    def test_check_available_true(self):
        """Test check_available returns True when gitlab is installed."""
        with mock.patch.object(gitlab_api, 'AVAILABLE', True):
            result = gitlab_api.check_available()
            self.assertTrue(result)


class TestParseApplyWithPush(unittest.TestCase):
    """Tests for apply command with push options."""

    def test_parse_apply_with_push(self):
        """Test parsing apply command with push option."""
        args = pickman.parse_args(['apply', 'us/next', '-p'])
        self.assertEqual(args.cmd, 'apply')
        self.assertEqual(args.source, 'us/next')
        self.assertTrue(args.push)
        self.assertEqual(args.remote, 'ci')
        self.assertEqual(args.target, 'master')

    def test_parse_apply_with_push_options(self):
        """Test parsing apply command with all push options."""
        args = pickman.parse_args([
            'apply', 'us/next', '-p',
            '-r', 'origin', '-t', 'main'
        ])
        self.assertEqual(args.cmd, 'apply')
        self.assertTrue(args.push)
        self.assertEqual(args.remote, 'origin')
        self.assertEqual(args.target, 'main')


if __name__ == '__main__':
    unittest.main()

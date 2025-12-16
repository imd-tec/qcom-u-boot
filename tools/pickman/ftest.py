# SPDX-License-Identifier: GPL-2.0+
#
# Copyright 2025 Canonical Ltd.
# Written by Simon Glass <simon.glass@canonical.com>
#
# pylint: disable=too-many-lines
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
from u_boot_pylib import tout

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
            with terminal.capture():
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
            with terminal.capture():
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
            with terminal.capture():
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

    def test_duplicate_database_error(self):
        """Test creating duplicate database raises error."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            with self.assertRaises(ValueError) as ctx:
                database.Database(self.db_path)
            self.assertIn('already a database', str(ctx.exception))
            dbs.close()

    def test_open_already_open_error(self):
        """Test opening already open database raises error."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            with self.assertRaises(ValueError) as ctx:
                dbs.open_it()
            self.assertIn('Already open', str(ctx.exception))
            dbs.close()

    def test_close_already_closed_error(self):
        """Test closing already closed database raises error."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.close()
            with self.assertRaises(ValueError) as ctx:
                dbs.close()
            self.assertIn('Already closed', str(ctx.exception))

    def test_rollback(self):
        """Test rollback discards uncommitted changes."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'abc123')
            dbs.commit()

            # Make a change but don't commit
            dbs.source_set('us/next', 'def456')
            # Rollback should discard the change
            dbs.rollback()

            result = dbs.source_get('us/next')
            self.assertEqual(result, 'abc123')
            dbs.close()

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


class TestDatabaseComment(unittest.TestCase):
    """Tests for Database comment functions."""

    def setUp(self):
        """Set up test fixtures."""
        fd, self.db_path = tempfile.mkstemp(suffix='.db')
        os.close(fd)
        os.unlink(self.db_path)

    def tearDown(self):
        """Clean up test fixtures."""
        if os.path.exists(self.db_path):
            os.unlink(self.db_path)
        database.Database.instances.clear()

    def test_comment_mark_and_check_processed(self):
        """Test marking and checking processed comments"""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()

            # Comment should not be processed initially
            self.assertFalse(dbs.comment_is_processed(123, 456))

            # Mark as processed
            dbs.comment_mark_processed(123, 456)
            dbs.commit()

            # Now should be processed
            self.assertTrue(dbs.comment_is_processed(123, 456))

            # Different comment should not be processed
            self.assertFalse(dbs.comment_is_processed(123, 789))
            self.assertFalse(dbs.comment_is_processed(999, 456))

            dbs.close()

    def test_comment_get_processed(self):
        """Test getting all processed comments for an MR"""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()

            # Mark several comments as processed
            dbs.comment_mark_processed(100, 1)
            dbs.comment_mark_processed(100, 2)
            dbs.comment_mark_processed(100, 3)
            dbs.comment_mark_processed(200, 10)  # Different MR
            dbs.commit()

            # Get processed for MR 100
            processed = dbs.comment_get_processed(100)
            self.assertEqual(len(processed), 3)
            self.assertIn(1, processed)
            self.assertIn(2, processed)
            self.assertIn(3, processed)
            self.assertNotIn(10, processed)

            # Get processed for MR 200
            processed = dbs.comment_get_processed(200)
            self.assertEqual(len(processed), 1)
            self.assertIn(10, processed)

            dbs.close()

    def test_comment_mark_processed_idempotent(self):
        """Test that marking same comment twice doesn't fail"""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()

            # Mark same comment twice (should not raise)
            dbs.comment_mark_processed(123, 456)
            dbs.comment_mark_processed(123, 456)
            dbs.commit()

            # Should still be processed
            self.assertTrue(dbs.comment_is_processed(123, 456))

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

        # First-parent log (to find next merge on mainline)
        fp_log_output = (
            'aaa111|aaa111a|Author 1|First commit|abc123\n'
            'bbb222|bbb222b|Author 2|Second commit|aaa111\n'
            'ccc333|ccc333c|Author 3|Merge branch feature|bbb222 ddd444\n'
            'eee555|eee555e|Author 4|After merge|ccc333\n'
        )
        # Full log (to get all commits up to the merge)
        full_log_output = (
            'aaa111|aaa111a|Author 1|First commit|abc123\n'
            'bbb222|bbb222b|Author 2|Second commit|aaa111\n'
            'ccc333|ccc333c|Author 3|Merge branch feature|bbb222 ddd444\n'
        )

        def mock_git(pipe_list):
            cmd = pipe_list[0] if pipe_list else []
            if '--first-parent' in cmd:
                return command.CommandResult(stdout=fp_log_output)
            return command.CommandResult(stdout=full_log_output)

        command.TEST_RESULT = mock_git

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


class TestNextMerges(unittest.TestCase):
    """Tests for next-merges command."""

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

    def test_next_merges(self):
        """Test next-merges shows upcoming merges"""
        # Add source to database
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'abc123')
            dbs.commit()
            dbs.close()

        database.Database.instances.clear()

        # Mock git log with merge commits
        log_output = (
            'aaa111|aaa111a|Merge branch feature-1\n'
            'bbb222|bbb222b|Merge branch feature-2\n'
            'ccc333|ccc333c|Merge branch feature-3\n'
        )
        command.TEST_RESULT = command.CommandResult(stdout=log_output)

        args = argparse.Namespace(cmd='next-merges', source='us/next', count=10)
        with terminal.capture() as (stdout, _):
            ret = control.do_pickman(args)
        self.assertEqual(ret, 0)
        output = stdout.getvalue()
        self.assertIn('Next 3 merges from us/next:', output)
        self.assertIn('1. aaa111a Merge branch feature-1', output)
        self.assertIn('2. bbb222b Merge branch feature-2', output)
        self.assertIn('3. ccc333c Merge branch feature-3', output)

    def test_next_merges_with_count(self):
        """Test next-merges respects count parameter"""
        # Add source to database
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'abc123')
            dbs.commit()
            dbs.close()

        database.Database.instances.clear()

        # Mock git log with merge commits
        log_output = (
            'aaa111|aaa111a|Merge branch feature-1\n'
            'bbb222|bbb222b|Merge branch feature-2\n'
            'ccc333|ccc333c|Merge branch feature-3\n'
        )
        command.TEST_RESULT = command.CommandResult(stdout=log_output)

        args = argparse.Namespace(cmd='next-merges', source='us/next', count=2)
        with terminal.capture() as (stdout, _):
            ret = control.do_pickman(args)
        self.assertEqual(ret, 0)
        output = stdout.getvalue()
        self.assertIn('Next 2 merges from us/next:', output)
        self.assertIn('1. aaa111a', output)
        self.assertIn('2. bbb222b', output)
        self.assertNotIn('3. ccc333c', output)

    def test_next_merges_no_merges(self):
        """Test next-merges with no merges remaining"""
        # Add source to database
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'abc123')
            dbs.commit()
            dbs.close()

        database.Database.instances.clear()

        command.TEST_RESULT = command.CommandResult(stdout='')

        args = argparse.Namespace(cmd='next-merges', source='us/next', count=10)
        with terminal.capture() as (stdout, _):
            ret = control.do_pickman(args)
        self.assertEqual(ret, 0)
        self.assertIn('No merges remaining', stdout.getvalue())


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

            # First-parent log (to find next merge on mainline)
            fp_log_output = (
                'aaa111|aaa111a|Author 1|First commit|abc123\n'
                'bbb222|bbb222b|Author 2|Merge branch|aaa111 ccc333\n'
                'ddd444|ddd444d|Author 3|After merge|bbb222\n'
            )
            # Full log (to get all commits up to the merge)
            full_log_output = (
                'aaa111|aaa111a|Author 1|First commit|abc123\n'
                'bbb222|bbb222b|Author 2|Merge branch|aaa111 ccc333\n'
            )

            def mock_git(pipe_list):
                cmd = pipe_list[0] if pipe_list else []
                if '--first-parent' in cmd:
                    return command.CommandResult(stdout=fp_log_output)
                return command.CommandResult(stdout=full_log_output)

            command.TEST_RESULT = mock_git

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

        args = argparse.Namespace(cmd='apply', source='unknown', branch=None)
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

        args = argparse.Namespace(cmd='apply', source='us/next', branch=None)
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
            with terminal.capture():
                result = gitlab_api.check_available()
            self.assertFalse(result)

    def test_check_available_true(self):
        """Test check_available returns True when gitlab is installed."""
        with mock.patch.object(gitlab_api, 'AVAILABLE', True):
            with terminal.capture():
                result = gitlab_api.check_available()
            self.assertTrue(result)


class TestUpdateMrDescription(unittest.TestCase):
    """Tests for update_mr_description function."""

    @mock.patch.object(gitlab_api, 'get_remote_url')
    @mock.patch.object(gitlab_api, 'get_token')
    @mock.patch.object(gitlab_api, 'AVAILABLE', True)
    def test_update_mr_description_success(self, mock_token, mock_url):
        """Test successful MR description update."""
        mock_token.return_value = 'test-token'
        mock_url.return_value = 'git@gitlab.com:group/project.git'

        mock_mr = mock.MagicMock()
        mock_project = mock.MagicMock()
        mock_project.mergerequests.get.return_value = mock_mr

        with mock.patch('gitlab.Gitlab') as mock_gitlab:
            mock_gitlab.return_value.projects.get.return_value = mock_project

            result = gitlab_api.update_mr_description('origin', 123,
                                                      'New description')

            self.assertTrue(result)
            self.assertEqual(mock_mr.description, 'New description')
            mock_mr.save.assert_called_once()

    @mock.patch.object(gitlab_api, 'AVAILABLE', False)
    def test_update_mr_description_not_available(self):
        """Test update_mr_description when gitlab not available."""
        with terminal.capture():
            result = gitlab_api.update_mr_description('origin', 123, 'desc')
        self.assertFalse(result)

    @mock.patch.object(gitlab_api, 'get_token')
    @mock.patch.object(gitlab_api, 'AVAILABLE', True)
    def test_update_mr_description_no_token(self, mock_token):
        """Test update_mr_description when no token set."""
        mock_token.return_value = None
        with terminal.capture():
            result = gitlab_api.update_mr_description('origin', 123, 'desc')
        self.assertFalse(result)


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


class TestParseStep(unittest.TestCase):
    """Tests for step command argument parsing."""

    def test_parse_step_defaults(self):
        """Test parsing step command with defaults."""
        args = pickman.parse_args(['step', 'us/next'])
        self.assertEqual(args.cmd, 'step')
        self.assertEqual(args.source, 'us/next')
        self.assertEqual(args.remote, 'ci')
        self.assertEqual(args.target, 'master')

    def test_parse_step_with_options(self):
        """Test parsing step command with all options."""
        args = pickman.parse_args(['step', 'us/next', '-r', 'origin',
                                   '-t', 'main'])
        self.assertEqual(args.cmd, 'step')
        self.assertEqual(args.source, 'us/next')
        self.assertEqual(args.remote, 'origin')
        self.assertEqual(args.target, 'main')


class TestParseMrDescription(unittest.TestCase):
    """Tests for parse_mr_description function."""

    def test_parse_mr_description(self):
        """Test parsing a valid MR description."""
        description = """## 2025-01-15: us/next

Branch: cherry-abc123

Commits:
- abc123a First commit
- def456b Second commit
- caf789c Third commit

### Conversation log
Some log text"""
        source, last_hash = control.parse_mr_description(description)
        self.assertEqual(source, 'us/next')
        self.assertEqual(last_hash, 'caf789c')

    def test_parse_mr_description_single_commit(self):
        """Test parsing MR description with single commit."""
        description = """## 2025-01-15: feature/branch

Branch: cherry-xyz

Commits:
- abc1234 Only commit"""
        source, last_hash = control.parse_mr_description(description)
        self.assertEqual(source, 'feature/branch')
        self.assertEqual(last_hash, 'abc1234')

    def test_parse_mr_description_invalid(self):
        """Test parsing invalid MR description."""
        source, last_hash = control.parse_mr_description('invalid description')
        self.assertIsNone(source)
        self.assertIsNone(last_hash)

    def test_parse_mr_description_no_commits(self):
        """Test parsing MR description without commits."""
        description = """## 2025-01-15: us/next

Branch: cherry-abc"""
        source, last_hash = control.parse_mr_description(description)
        self.assertIsNone(source)
        self.assertIsNone(last_hash)

    def test_parse_mr_description_ignores_short_hashes(self):
        """Test that short numbers in conversation log are not matched."""
        description = """## 2025-01-15: us/next

Branch: cherry-abc123

Commits:
- abc123a First commit
- def456b Second commit

### Conversation log
- 1 board built (sandbox)
- 2 tests passed"""
        source, last_hash = control.parse_mr_description(description)
        self.assertEqual(source, 'us/next')
        # Should match def456b, not "1" or "2" from conversation log
        self.assertEqual(last_hash, 'def456b')


class TestStep(unittest.TestCase):
    """Tests for step command."""

    def test_step_with_pending_mr(self):
        """Test step does nothing when MR is pending."""
        mock_mr = gitlab_api.PickmanMr(
            iid=123,
            title='[pickman] Test MR',
            web_url='https://gitlab.com/mr/123',
            source_branch='cherry-test',
            description='Test',
        )
        with mock.patch.object(gitlab_api, 'get_merged_pickman_mrs',
                               return_value=[]):
            with mock.patch.object(gitlab_api, 'get_open_pickman_mrs',
                                   return_value=[mock_mr]):
                args = argparse.Namespace(cmd='step', source='us/next',
                                          remote='ci', target='master')
                with terminal.capture():
                    ret = control.do_step(args, None)

        self.assertEqual(ret, 0)

    def test_step_gitlab_error(self):
        """Test step when GitLab API returns error."""
        with mock.patch.object(gitlab_api, 'get_merged_pickman_mrs',
                               return_value=None):
            args = argparse.Namespace(cmd='step', source='us/next',
                                      remote='ci', target='master')
            with terminal.capture():
                ret = control.do_step(args, None)

        self.assertEqual(ret, 1)

    def test_step_open_mrs_error(self):
        """Test step when get_open_pickman_mrs returns error."""
        with mock.patch.object(gitlab_api, 'get_merged_pickman_mrs',
                               return_value=[]):
            with mock.patch.object(gitlab_api, 'get_open_pickman_mrs',
                                   return_value=None):
                args = argparse.Namespace(cmd='step', source='us/next',
                                          remote='ci', target='master')
                with terminal.capture():
                    ret = control.do_step(args, None)

        self.assertEqual(ret, 1)


class TestParseReview(unittest.TestCase):
    """Tests for review command argument parsing."""

    def test_parse_review_defaults(self):
        """Test parsing review command with defaults."""
        args = pickman.parse_args(['review'])
        self.assertEqual(args.cmd, 'review')
        self.assertEqual(args.remote, 'ci')

    def test_parse_review_with_remote(self):
        """Test parsing review command with custom remote."""
        args = pickman.parse_args(['review', '-r', 'origin'])
        self.assertEqual(args.cmd, 'review')
        self.assertEqual(args.remote, 'origin')


class TestReview(unittest.TestCase):
    """Tests for review command."""

    def test_review_no_mrs(self):
        """Test review when no open MRs found."""
        with mock.patch.object(gitlab_api, 'get_open_pickman_mrs',
                               return_value=[]):
            args = argparse.Namespace(cmd='review', remote='ci')
            with terminal.capture():
                ret = control.do_review(args, None)

        self.assertEqual(ret, 0)

    def test_review_gitlab_error(self):
        """Test review when GitLab API returns error."""
        with mock.patch.object(gitlab_api, 'get_open_pickman_mrs',
                               return_value=None):
            args = argparse.Namespace(cmd='review', remote='ci')
            with terminal.capture():
                ret = control.do_review(args, None)

        self.assertEqual(ret, 1)


class TestParsePoll(unittest.TestCase):
    """Tests for poll command argument parsing."""

    def test_parse_poll_defaults(self):
        """Test parsing poll command with defaults."""
        args = pickman.parse_args(['poll', 'us/next'])
        self.assertEqual(args.cmd, 'poll')
        self.assertEqual(args.source, 'us/next')
        self.assertEqual(args.interval, 300)
        self.assertEqual(args.remote, 'ci')
        self.assertEqual(args.target, 'master')

    def test_parse_poll_with_options(self):
        """Test parsing poll command with all options."""
        args = pickman.parse_args([
            'poll', 'us/next',
            '-i', '60', '-r', 'origin', '-t', 'main'
        ])
        self.assertEqual(args.cmd, 'poll')
        self.assertEqual(args.source, 'us/next')
        self.assertEqual(args.interval, 60)
        self.assertEqual(args.remote, 'origin')
        self.assertEqual(args.target, 'main')


class TestPoll(unittest.TestCase):
    """Tests for poll command."""

    def test_poll_stops_on_keyboard_interrupt(self):
        """Test poll stops gracefully on KeyboardInterrupt."""
        call_count = [0]

        def mock_step(_args, _dbs):
            call_count[0] += 1
            if call_count[0] >= 2:
                raise KeyboardInterrupt
            return 0

        with mock.patch.object(control, 'do_step', mock_step):
            with mock.patch('time.sleep'):
                args = argparse.Namespace(
                    cmd='poll', source='us/next', interval=1,
                    remote='ci', target='master'
                )
                with terminal.capture():
                    ret = control.do_poll(args, None)

        self.assertEqual(ret, 0)
        self.assertEqual(call_count[0], 2)

    def test_poll_continues_on_step_error(self):
        """Test poll continues when step returns non-zero."""
        call_count = [0]

        def mock_step(_args, _dbs):
            call_count[0] += 1
            if call_count[0] >= 2:
                raise KeyboardInterrupt
            return 1  # Return error

        with mock.patch.object(control, 'do_step', mock_step):
            with mock.patch('time.sleep'):
                args = argparse.Namespace(
                    cmd='poll', source='us/next', interval=1,
                    remote='ci', target='master'
                )
                with terminal.capture() as (_, stderr):
                    ret = control.do_poll(args, None)

        self.assertEqual(ret, 0)
        self.assertIn('returned 1', stderr.getvalue())


class TestFormatHistorySummary(unittest.TestCase):
    """Tests for format_history_summary function."""

    def test_format_history_summary(self):
        """Test formatting history summary."""
        commits = [
            control.CommitInfo('aaa111', 'aaa111a', 'First commit', 'Author 1'),
            control.CommitInfo('bbb222', 'bbb222b', 'Second commit', 'Author 2'),
        ]
        result = control.format_history_summary('us/next', commits, 'cherry-abc')

        self.assertIn('us/next', result)
        self.assertIn('Branch: cherry-abc', result)
        self.assertIn('- aaa111a First commit', result)
        self.assertIn('- bbb222b Second commit', result)


class TestGetHistory(unittest.TestCase):
    """Tests for get_history function."""

    def setUp(self):
        """Set up test fixtures."""
        fd, self.history_file = tempfile.mkstemp(suffix='.history')
        os.close(fd)
        os.unlink(self.history_file)  # Remove so we start fresh

    def tearDown(self):
        """Clean up test fixtures."""
        if os.path.exists(self.history_file):
            os.unlink(self.history_file)

    def test_get_history_empty(self):
        """Test get_history with no existing file."""
        commits = [
            control.CommitInfo('aaa111', 'aaa111a', 'First commit', 'Author 1'),
        ]
        content, commit_msg = control.get_history(
            self.history_file, 'us/next', commits, 'cherry-abc',
            'Conversation output')

        self.assertIn('us/next', content)
        self.assertIn('Branch: cherry-abc', content)
        self.assertIn('- aaa111a First commit', content)
        self.assertIn('### Conversation log', content)
        self.assertIn('Conversation output', content)
        self.assertIn('---', content)

        # Verify commit message
        self.assertIn('pickman: Record cherry-pick of 1 commits', commit_msg)
        self.assertIn('- aaa111a First commit', commit_msg)

        # Verify file was written
        with open(self.history_file, 'r', encoding='utf-8') as fhandle:
            file_content = fhandle.read()
        self.assertEqual(file_content, content)

    def test_get_history_with_existing(self):
        """Test get_history appends to existing content."""
        # Create existing file
        with open(self.history_file, 'w', encoding='utf-8') as fhandle:
            fhandle.write('Previous history content\n')

        commits = [
            control.CommitInfo('bbb222', 'bbb222b', 'New commit', 'Author 2'),
        ]
        content, commit_msg = control.get_history(
            self.history_file, 'us/next', commits, 'cherry-new',
            'New conversation')

        self.assertIn('Previous history content', content)
        self.assertIn('cherry-new', content)
        self.assertIn('New conversation', content)
        self.assertIn('- bbb222b New commit', commit_msg)

    def test_get_history_replaces_existing_branch(self):
        """Test get_history removes existing entry for same branch."""
        # Create existing file with an entry for cherry-abc
        existing = """## 2025-01-01: us/next

Branch: cherry-abc

Commits:
- aaa111a Old commit

### Conversation log
Old conversation

---

Other content
"""
        with open(self.history_file, 'w', encoding='utf-8') as fhandle:
            fhandle.write(existing)

        commits = [
            control.CommitInfo('ccc333', 'ccc333c', 'Updated commit', 'Author'),
        ]
        content, _ = control.get_history(self.history_file, 'us/next', commits,
                                         'cherry-abc', 'New conversation')

        # Old entry should be removed
        self.assertNotIn('Old commit', content)
        self.assertNotIn('Old conversation', content)
        # New entry should be present
        self.assertIn('Updated commit', content)
        self.assertIn('New conversation', content)
        # Other content should be preserved
        self.assertIn('Other content', content)

    def test_get_history_multiple_commits(self):
        """Test get_history with multiple commits."""
        commits = [
            control.CommitInfo('aaa111', 'aaa111a', 'First commit', 'Author 1'),
            control.CommitInfo('bbb222', 'bbb222b', 'Second commit', 'Author 2'),
            control.CommitInfo('ccc333', 'ccc333c', 'Third commit', 'Author 3'),
        ]
        content, commit_msg = control.get_history(
            self.history_file, 'us/next', commits, 'cherry-abc', 'Log')

        # Verify all commits in content
        self.assertIn('- aaa111a First commit', content)
        self.assertIn('- bbb222b Second commit', content)
        self.assertIn('- ccc333c Third commit', content)

        # Verify commit message
        self.assertIn('pickman: Record cherry-pick of 3 commits', commit_msg)
        self.assertIn('- aaa111a First commit', commit_msg)
        self.assertIn('- bbb222b Second commit', commit_msg)
        self.assertIn('- ccc333c Third commit', commit_msg)


class TestPrepareApply(unittest.TestCase):
    """Tests for prepare_apply function."""

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

    def test_prepare_apply_error(self):
        """Test prepare_apply returns error code 1 on source not found."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()

            info, ret = control.prepare_apply(dbs, 'unknown', None)

            self.assertIsNone(info)
            self.assertEqual(ret, 1)
            dbs.close()

    def test_prepare_apply_no_commits(self):
        """Test prepare_apply returns code 0 when no commits."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'abc123')
            dbs.commit()

            command.TEST_RESULT = command.CommandResult(stdout='')

            info, ret = control.prepare_apply(dbs, 'us/next', None)

            self.assertIsNone(info)
            self.assertEqual(ret, 0)
            dbs.close()

    def test_prepare_apply_with_commits(self):
        """Test prepare_apply returns ApplyInfo with commits."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'abc123')
            dbs.commit()

            log_output = 'aaa111|aaa111a|Author 1|First commit|abc123\n'

            def mock_git(pipe_list):
                cmd = pipe_list[0] if pipe_list else []
                if 'log' in cmd:
                    return command.CommandResult(stdout=log_output)
                if 'rev-parse' in cmd:
                    return command.CommandResult(stdout='master')
                return command.CommandResult(stdout='')

            command.TEST_RESULT = mock_git

            info, ret = control.prepare_apply(dbs, 'us/next', None)

            self.assertIsNotNone(info)
            self.assertEqual(ret, 0)
            self.assertEqual(len(info.commits), 1)
            self.assertEqual(info.branch_name, 'cherry-aaa111a')
            self.assertEqual(info.original_branch, 'master')
            dbs.close()

    def test_prepare_apply_custom_branch(self):
        """Test prepare_apply uses custom branch name."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'abc123')
            dbs.commit()

            log_output = 'aaa111|aaa111a|Author 1|First commit|abc123\n'

            def mock_git(pipe_list):
                cmd = pipe_list[0] if pipe_list else []
                if 'log' in cmd:
                    return command.CommandResult(stdout=log_output)
                if 'rev-parse' in cmd:
                    return command.CommandResult(stdout='master')
                return command.CommandResult(stdout='')

            command.TEST_RESULT = mock_git

            info, _ = control.prepare_apply(dbs, 'us/next', 'my-branch')

            self.assertIsNotNone(info)
            self.assertEqual(info.branch_name, 'my-branch')
            dbs.close()


class TestExecuteApply(unittest.TestCase):
    """Tests for execute_apply function."""

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

    def test_execute_apply_success(self):
        """Test execute_apply with successful cherry-pick."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'abc123')
            dbs.commit()

            commits = [control.CommitInfo('aaa111', 'aaa111a', 'Test commit',
                                          'Author')]
            args = argparse.Namespace(push=False)

            with mock.patch.object(control.agent, 'cherry_pick_commits',
                                   return_value=(True, 'conversation log')):
                ret, success, conv_log = control.execute_apply(
                    dbs, 'us/next', commits, 'cherry-branch', args)

            self.assertEqual(ret, 0)
            self.assertTrue(success)
            self.assertEqual(conv_log, 'conversation log')

            # Check commit was added to database
            commit_rec = dbs.commit_get('aaa111')
            self.assertIsNotNone(commit_rec)
            self.assertEqual(commit_rec[6], 'applied')  # status field
            dbs.close()

    def test_execute_apply_failure(self):
        """Test execute_apply with failed cherry-pick."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'abc123')
            dbs.commit()

            commits = [control.CommitInfo('bbb222', 'bbb222b', 'Test commit',
                                          'Author')]
            args = argparse.Namespace(push=False)

            with mock.patch.object(control.agent, 'cherry_pick_commits',
                                   return_value=(False, 'error log')):
                ret, success, _ = control.execute_apply(
                    dbs, 'us/next', commits, 'cherry-branch', args)

            self.assertEqual(ret, 1)
            self.assertFalse(success)

            # Check commit status is conflict
            commit_rec = dbs.commit_get('bbb222')
            self.assertEqual(commit_rec[6], 'conflict')
            dbs.close()

    def test_execute_apply_with_push(self):
        """Test execute_apply with push enabled."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'abc123')
            dbs.commit()

            commits = [control.CommitInfo('ccc333', 'ccc333c', 'Test commit',
                                          'Author')]
            args = argparse.Namespace(push=True, remote='origin',
                                      target='main')

            with mock.patch.object(control.agent, 'cherry_pick_commits',
                                   return_value=(True, 'log')):
                with mock.patch.object(gitlab_api, 'push_and_create_mr',
                                       return_value='https://mr/url'):
                    ret, success, _ = control.execute_apply(
                        dbs, 'us/next', commits, 'cherry-branch', args)

            self.assertEqual(ret, 0)
            self.assertTrue(success)
            dbs.close()

    def test_execute_apply_push_fails(self):
        """Test execute_apply when MR creation fails."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'abc123')
            dbs.commit()

            commits = [control.CommitInfo('ddd444', 'ddd444d', 'Test commit',
                                          'Author')]
            args = argparse.Namespace(push=True, remote='origin',
                                      target='main')

            with mock.patch.object(control.agent, 'cherry_pick_commits',
                                   return_value=(True, 'log')):
                with mock.patch.object(gitlab_api, 'push_and_create_mr',
                                       return_value=None):
                    ret, success, _ = control.execute_apply(
                        dbs, 'us/next', commits, 'cherry-branch', args)

            self.assertEqual(ret, 1)
            self.assertTrue(success)  # cherry-pick succeeded, MR failed
            dbs.close()


class TestGetNextCommitsEmptyLine(unittest.TestCase):
    """Tests for get_next_commits with empty lines."""

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

    def test_get_next_commits_with_empty_lines(self):
        """Test get_next_commits handles empty lines in output."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'abc123')
            dbs.commit()

            # Log output with empty lines
            log_output = (
                'aaa111|aaa111a|Author 1|First commit|abc123\n'
                '\n'  # Empty line
                'bbb222|bbb222b|Author 2|Second commit|aaa111\n'
                '\n'  # Another empty line
            )
            command.TEST_RESULT = command.CommandResult(stdout=log_output)

            commits, merge_found, error = control.get_next_commits(dbs,
                                                                   'us/next')
            self.assertIsNone(error)
            self.assertFalse(merge_found)
            self.assertEqual(len(commits), 2)
            dbs.close()


class TestDoCommitSourceResolveError(unittest.TestCase):
    """Tests for do_commit_source error handling."""

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

    def test_commit_source_resolve_error(self):
        """Test commit-source fails when commit can't be resolved."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'oldcommit12345')
            dbs.commit()

        database.Database.instances.clear()

        def mock_git_fail(**_kwargs):
            raise command.CommandExc('git error', command.CommandResult())

        command.TEST_RESULT = mock_git_fail

        args = argparse.Namespace(cmd='commit-source', source='us/next',
                                  commit='badcommit')
        with terminal.capture() as (_, stderr):
            ret = control.do_commit_source(args, None)
        self.assertEqual(ret, 1)
        self.assertIn('Could not resolve', stderr.getvalue())


class TestDoPickmanUnknownCommand(unittest.TestCase):
    """Tests for do_pickman with unknown command."""

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

    def test_unknown_command(self):
        """Test do_pickman returns 1 for unknown command."""
        args = argparse.Namespace(cmd='unknown-command')
        with terminal.capture():
            ret = control.do_pickman(args)
        self.assertEqual(ret, 1)


class TestDoReviewWithMrs(unittest.TestCase):
    """Tests for do_review with open MRs."""

    def test_review_with_mrs_no_comments(self):
        """Test review with open MRs but no comments."""
        tout.init(tout.INFO)

        mock_mr = gitlab_api.PickmanMr(
            iid=123,
            title='[pickman] Test MR',
            web_url='https://gitlab.com/mr/123',
            source_branch='cherry-test',
            description='Test',
        )
        with mock.patch.object(gitlab_api, 'get_open_pickman_mrs',
                               return_value=[mock_mr]):
            with mock.patch.object(gitlab_api, 'get_mr_comments',
                                   return_value=[]):
                args = argparse.Namespace(cmd='review', remote='ci')
                with terminal.capture() as (stdout, _):
                    ret = control.do_review(args, None)

        self.assertEqual(ret, 0)
        self.assertIn('Found 1 open pickman MR', stdout.getvalue())


class TestProcessMergedMrs(unittest.TestCase):
    """Tests for process_merged_mrs function."""

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
        command.TEST_RESULT = None

    def test_process_merged_mrs_updates_newer(self):
        """Test that newer commits update the database."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'aaa111aaa111aaa111aaa111aaa111aaa111aaa1')
            dbs.commit()

            merged_mrs = [gitlab_api.PickmanMr(
                iid=100,
                title='[pickman] Test MR',
                description='## 2025-01-01: us/next\n\n- bbb222b Subject',
                source_branch='cherry-test',
                web_url='https://gitlab.com/mr/100',
            )]

            def mock_git(args):
                if args[0] == 'rev-parse':
                    return 'bbb222bbb222bbb222bbb222bbb222bbb222bbb2'
                if args[0] == 'merge-base':
                    # current is ancestor of last_hash (newer)
                    return ''
                return ''

            with mock.patch.object(gitlab_api, 'get_merged_pickman_mrs',
                                   return_value=merged_mrs):
                with mock.patch.object(control, 'run_git', mock_git):
                    processed = control.process_merged_mrs('ci', 'us/next', dbs)

            self.assertEqual(processed, 1)
            new_commit = dbs.source_get('us/next')
            self.assertEqual(new_commit,
                             'bbb222bbb222bbb222bbb222bbb222bbb222bbb2')

            dbs.close()

    def test_process_merged_mrs_skips_older(self):
        """Test that older commits don't update the database."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'bbb222bbb222bbb222bbb222bbb222bbb222bbb2')
            dbs.commit()

            merged_mrs = [gitlab_api.PickmanMr(
                iid=100,
                title='[pickman] Test MR',
                description='## 2025-01-01: us/next\n\n- aaa111a Subject',
                source_branch='cherry-test',
                web_url='https://gitlab.com/mr/100',
            )]

            def mock_git(args):
                if args[0] == 'rev-parse':
                    return 'aaa111aaa111aaa111aaa111aaa111aaa111aaa1'
                if args[0] == 'merge-base':
                    # current is NOT ancestor of last_hash (older)
                    raise RuntimeError('Not an ancestor')
                return ''

            with mock.patch.object(gitlab_api, 'get_merged_pickman_mrs',
                                   return_value=merged_mrs):
                with mock.patch.object(control, 'run_git', mock_git):
                    processed = control.process_merged_mrs('ci', 'us/next', dbs)

            self.assertEqual(processed, 0)
            # Should remain unchanged
            current = dbs.source_get('us/next')
            self.assertEqual(current,
                             'bbb222bbb222bbb222bbb222bbb222bbb222bbb2')

            dbs.close()


if __name__ == '__main__':
    unittest.main()

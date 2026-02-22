# SPDX-License-Identifier: GPL-2.0+
#
# Copyright 2025 Canonical Ltd.
# Written by Simon Glass <simon.glass@canonical.com>
#
# pylint: disable=too-many-lines
"""Tests for pickman."""

import argparse
import os
import shutil
import subprocess
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
from pickman import agent
from pickman import control
from pickman import database
from pickman import gitlab_api as gitlab

# Test URL constants
TEST_OAUTH_URL = 'https://oauth2:test-token@gitlab.com/group/project.git'
TEST_HTTPS_URL = 'https://gitlab.com/group/project.git'
TEST_SSH_URL = 'git@gitlab.com:group/project.git'
TEST_MR_URL = 'https://gitlab.com/group/project/-/merge_requests/42'
TEST_MR_42_URL = 'https://gitlab.com/mr/42'
TEST_MR_1_URL = 'https://gitlab.com/mr/1'
TEST_SHORT_OAUTH_URL = 'https://oauth2:token@gitlab.com/g/p.git'


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
        self.assertEqual(commit.chash, 'abc123d')
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
            self.assertEqual(commit.chash, 'abc123d')
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
            self.assertEqual(commit.chash, 'def456a')
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
                            if not l.startswith(('Update database',
                                                'Creating'))]
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
                             TEST_MR_42_URL, '2025-01-15')
            dbs.commit()

            # Get the merge request
            result = dbs.mergereq_get(42)
            self.assertIsNotNone(result)
            self.assertEqual(result[1], source_id)  # source_id
            self.assertEqual(result[2], 'cherry-abc123')  # branch_name
            self.assertEqual(result[3], 42)  # mr_id
            self.assertEqual(result[4], 'open')  # status
            self.assertEqual(result[5], TEST_MR_42_URL)  # url
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
                             TEST_MR_1_URL, '2025-01-01')
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
                             TEST_MR_42_URL, '2025-01-15')
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
                             TEST_MR_42_URL, '2025-01-15')
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
                             TEST_MR_42_URL, '2025-01-15')
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
            if '--first-parent' in cmd and '--merges' in cmd:
                # detect_sub_merges: no sub-merges
                return command.CommandResult(stdout='')
            if '--first-parent' in cmd:
                return command.CommandResult(stdout=fp_log_output)
            if 'rev-parse' in cmd:
                # detect_sub_merges: return two parents (it's a merge)
                return command.CommandResult(stdout='bbb222\nddd444\n')
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

    def _make_simple_merge_mock(self, log_output):
        """Create a mock handler for merges with no sub-merges"""
        def mock_git(pipe_list):
            cmd = pipe_list[0] if pipe_list else []
            # Initial merge listing
            if '--reverse' in cmd and '--format=%H|%h|%s' in cmd:
                return command.CommandResult(stdout=log_output)
            # Sub-merge detection: no sub-merges
            if '--first-parent' in cmd and '--merges' in cmd:
                return command.CommandResult(stdout='')
            # Parent lookup for detect_sub_merges
            if 'rev-parse' in cmd:
                return command.CommandResult(
                    stdout='parent1\nparent2\n')
            return command.CommandResult(stdout='')
        return mock_git

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

        log_output = (
            'aaa111|aaa111a|Merge branch feature-1\n'
            'bbb222|bbb222b|Merge branch feature-2\n'
            'ccc333|ccc333c|Merge branch feature-3\n'
        )
        command.TEST_RESULT = self._make_simple_merge_mock(log_output)

        args = argparse.Namespace(cmd='next-merges', source='us/next', count=10)
        with terminal.capture() as (stdout, _):
            ret = control.do_pickman(args)
        self.assertEqual(ret, 0)
        output = stdout.getvalue()
        self.assertIn('3 from 3 first-parent', output)
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

        log_output = (
            'aaa111|aaa111a|Merge branch feature-1\n'
            'bbb222|bbb222b|Merge branch feature-2\n'
            'ccc333|ccc333c|Merge branch feature-3\n'
        )
        command.TEST_RESULT = self._make_simple_merge_mock(log_output)

        args = argparse.Namespace(cmd='next-merges', source='us/next', count=2)
        with terminal.capture() as (stdout, _):
            ret = control.do_pickman(args)
        self.assertEqual(ret, 0)
        output = stdout.getvalue()
        self.assertIn('2 from 2 first-parent', output)
        self.assertIn('1. aaa111a', output)
        self.assertIn('2. bbb222b', output)
        self.assertNotIn('3. ccc333c', output)

    def test_next_merges_expands_mega_merge(self):
        """Test next-merges expands mega-merges into sub-merges"""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'abc123')
            dbs.commit()
            dbs.close()

        database.Database.instances.clear()

        def mock_git(pipe_list):
            cmd = pipe_list[0] if pipe_list else []
            cmd_str = ' '.join(cmd)
            # Initial merge listing - one mega-merge
            if '--reverse' in cmd and '--format=%H|%h|%s' in cmd:
                return command.CommandResult(
                    stdout='mega111|mega111a|Merge branch next\n')
            # Parent lookup
            if 'rev-parse' in cmd and '^@' in cmd_str:
                return command.CommandResult(
                    stdout='first_parent\nsecond_parent\n')
            # Sub-merge detection on second parent chain
            if ('--first-parent' in cmd and '--merges' in cmd
                    and '--format=%H' in cmd):
                return command.CommandResult(
                    stdout='sub_aaa\nsub_bbb\n')
            # Sub-merge detail lookup
            if 'log' in cmd and '-1' in cmd and '--format=%h|%s' in cmd:
                if 'sub_aaa' in cmd_str:
                    return command.CommandResult(
                        stdout='sub_aaa1|Merge feature-A\n')
                if 'sub_bbb' in cmd_str:
                    return command.CommandResult(
                        stdout='sub_bbb1|Merge feature-B\n')
            return command.CommandResult(stdout='')

        command.TEST_RESULT = mock_git

        args = argparse.Namespace(cmd='next-merges', source='us/next', count=10)
        with terminal.capture() as (stdout, _):
            ret = control.do_pickman(args)
        self.assertEqual(ret, 0)
        output = stdout.getvalue()
        self.assertIn('2 from 1 first-parent', output)
        self.assertIn('mega111a Merge branch next', output)
        self.assertIn('2 sub-merges', output)
        self.assertIn('1. sub_aaa1 Merge feature-A', output)
        self.assertIn('2. sub_bbb1 Merge feature-B', output)

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


class TestCountMerges(unittest.TestCase):
    """Tests for count-merges command."""

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

    def test_count_merges(self):
        """Test count-merges shows total remaining"""
        # Add source to database
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'abc123')
            dbs.commit()
            dbs.close()

        database.Database.instances.clear()

        # Mock git log with merge commits (oneline format)
        log_output = (
            'aaa111a Merge branch feature-1\n'
            'bbb222b Merge branch feature-2\n'
            'ccc333c Merge branch feature-3\n'
        )
        command.TEST_RESULT = command.CommandResult(stdout=log_output)

        args = argparse.Namespace(cmd='count-merges', source='us/next')
        with terminal.capture() as (stdout, _):
            ret = control.do_pickman(args)
        self.assertEqual(ret, 0)
        self.assertIn('3 merges remaining from us/next', stdout.getvalue())

    def test_count_merges_none(self):
        """Test count-merges with no merges remaining"""
        # Add source to database
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'abc123')
            dbs.commit()
            dbs.close()

        database.Database.instances.clear()

        command.TEST_RESULT = command.CommandResult(stdout='')

        args = argparse.Namespace(cmd='count-merges', source='us/next')
        with terminal.capture() as (stdout, _):
            ret = control.do_pickman(args)
        self.assertEqual(ret, 0)
        self.assertIn('0 merges remaining', stdout.getvalue())

    def test_count_merges_source_not_found(self):
        """Test count-merges with unknown source"""
        # Create empty database
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.close()

        database.Database.instances.clear()

        args = argparse.Namespace(cmd='count-merges', source='unknown')
        with terminal.capture() as (_, stderr):
            ret = control.do_pickman(args)
        self.assertEqual(ret, 1)
        self.assertIn("Source 'unknown' not found", stderr.getvalue())


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
            info, err = control.get_next_commits(dbs, 'unknown')
            self.assertIsNone(info)
            self.assertIn('not found', err)
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
                if '--first-parent' in cmd and '--merges' in cmd:
                    # detect_sub_merges: no sub-merges
                    return command.CommandResult(stdout='')
                if '--first-parent' in cmd:
                    return command.CommandResult(stdout=fp_log_output)
                if 'rev-parse' in cmd:
                    # detect_sub_merges: return parents
                    return command.CommandResult(stdout='aaa111\nccc333\n')
                return command.CommandResult(stdout=full_log_output)

            command.TEST_RESULT = mock_git

            info, err = control.get_next_commits(dbs, 'us/next')
            self.assertIsNone(err)
            self.assertTrue(info.merge_found)
            self.assertEqual(len(info.commits), 2)
            self.assertEqual(info.commits[0].chash, 'aaa111a')
            self.assertEqual(info.commits[1].chash, 'bbb222b')
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
        host, path = gitlab.parse_url(
            'git@gitlab.com:group/project.git')
        self.assertEqual(host, 'gitlab.com')
        self.assertEqual(path, 'group/project')

    def test_parse_ssh_url_no_git_suffix(self):
        """Test parsing SSH URL without .git suffix."""
        host, path = gitlab.parse_url(
            'git@gitlab.com:group/project')
        self.assertEqual(host, 'gitlab.com')
        self.assertEqual(path, 'group/project')

    def test_parse_ssh_url_nested_group(self):
        """Test parsing SSH URL with nested group."""
        host, path = gitlab.parse_url(
            'git@gitlab.denx.de:u-boot/custodians/u-boot-dm.git')
        self.assertEqual(host, 'gitlab.denx.de')
        self.assertEqual(path, 'u-boot/custodians/u-boot-dm')

    def test_parse_https_url(self):
        """Test parsing HTTPS URL."""
        host, path = gitlab.parse_url(
            'https://gitlab.com/group/project.git')
        self.assertEqual(host, 'gitlab.com')
        self.assertEqual(path, 'group/project')

    def test_parse_https_url_no_git_suffix(self):
        """Test parsing HTTPS URL without .git suffix."""
        host, path = gitlab.parse_url(
            'https://gitlab.com/group/project')
        self.assertEqual(host, 'gitlab.com')
        self.assertEqual(path, 'group/project')

    def test_parse_http_url(self):
        """Test parsing HTTP URL."""
        host, path = gitlab.parse_url(
            'http://gitlab.example.com/group/project.git')
        self.assertEqual(host, 'gitlab.example.com')
        self.assertEqual(path, 'group/project')

    def test_parse_invalid_url(self):
        """Test parsing invalid URL."""
        host, path = gitlab.parse_url('not-a-valid-url')
        self.assertIsNone(host)
        self.assertIsNone(path)

    def test_parse_empty_url(self):
        """Test parsing empty URL."""
        host, path = gitlab.parse_url('')
        self.assertIsNone(host)
        self.assertIsNone(path)


class TestCheckAvailable(unittest.TestCase):
    """Tests for GitLab availability checks."""

    def test_check_available_false(self):
        """Test check_available returns False when gitlab not installed."""
        with mock.patch.object(gitlab, 'AVAILABLE', False):
            with terminal.capture():
                result = gitlab.check_available()
            self.assertFalse(result)

    def test_check_available_true(self):
        """Test check_available returns True when gitlab is installed."""
        with mock.patch.object(gitlab, 'AVAILABLE', True):
            with terminal.capture():
                result = gitlab.check_available()
            self.assertTrue(result)


class TestGetPushUrl(unittest.TestCase):
    """Tests for get_push_url function."""

    def test_get_push_url_success(self):
        """Test successful push URL generation."""
        with mock.patch.object(gitlab, 'get_token',
                               return_value='test-token'):
            with mock.patch.object(
                    gitlab, 'get_remote_url',
                    return_value=TEST_SSH_URL):
                url = gitlab.get_push_url('origin')
        self.assertEqual(url, TEST_OAUTH_URL)

    def test_get_push_url_no_token(self):
        """Test returns None when no token available."""
        with mock.patch.object(gitlab, 'get_token', return_value=None):
            url = gitlab.get_push_url('origin')
        self.assertIsNone(url)

    def test_get_push_url_invalid_remote(self):
        """Test returns None for invalid remote URL."""
        with mock.patch.object(gitlab, 'get_token',
                               return_value='test-token'):
            with mock.patch.object(gitlab, 'get_remote_url',
                                   return_value='not-a-valid-url'):
                url = gitlab.get_push_url('origin')
        self.assertIsNone(url)

    def test_get_push_url_https_remote(self):
        """Test with HTTPS remote URL."""
        with mock.patch.object(gitlab, 'get_token',
                               return_value='test-token'):
            with mock.patch.object(gitlab, 'get_remote_url',
                                   return_value=TEST_HTTPS_URL):
                url = gitlab.get_push_url('origin')
        self.assertEqual(url, TEST_OAUTH_URL)


class TestPushBranch(unittest.TestCase):
    """Tests for push_branch function."""

    def test_push_branch_force_with_remote_ref(self):
        """Test force push when remote branch exists uses --force-with-lease."""
        with mock.patch.object(gitlab, 'get_push_url',
                               return_value=TEST_SHORT_OAUTH_URL):
            with mock.patch.object(command, 'output') as mock_output:
                mock_output.side_effect = [
                    None,  # fetch succeeds
                    'abc123def\n',  # rev-parse returns OID
                    None,  # push succeeds
                ]
                result = gitlab.push_branch('ci', 'test-branch', force=True)

        self.assertTrue(result)
        # Should fetch, rev-parse, then push with --force-with-lease
        calls = mock_output.call_args_list
        self.assertEqual(len(calls), 3)
        self.assertEqual(calls[0], mock.call(
            'git', 'fetch', 'ci',
            '+refs/heads/test-branch:refs/remotes/ci/test-branch'))
        self.assertEqual(calls[1], mock.call(
            'git', 'rev-parse', 'refs/remotes/ci/test-branch'))
        push_args = calls[2][0]
        self.assertIn('--force-with-lease=test-branch:abc123def',
                      push_args)

    def test_push_branch_force_no_remote_ref(self):
        """Test force push when remote branch doesn't exist uses --force."""
        with mock.patch.object(gitlab, 'get_push_url',
                               return_value=TEST_SHORT_OAUTH_URL):
            with mock.patch.object(command, 'output') as mock_output:
                # Fetch fails (branch doesn't exist on remote)
                mock_output.side_effect = [
                    command.CommandExc('fetch failed',
                                       command.CommandResult()),  # fetch fails
                    None,  # push succeeds
                ]
                result = gitlab.push_branch('ci', 'new-branch', force=True)

        self.assertTrue(result)
        # Should try fetch, fail, then push with --force
        # (not --force-with-lease)
        calls = mock_output.call_args_list
        self.assertEqual(len(calls), 2)
        push_args = calls[1][0]
        self.assertIn('--force', push_args)
        self.assertNotIn('--force-with-lease', ' '.join(push_args))

    def test_push_branch_no_force(self):
        """Test regular push without force doesn't fetch or use force flags."""
        with mock.patch.object(gitlab, 'get_push_url',
                               return_value=TEST_SHORT_OAUTH_URL):
            with mock.patch.object(command, 'output') as mock_output:
                result = gitlab.push_branch('ci', 'test-branch', force=False)

        self.assertTrue(result)
        # Should only push, no fetch, no force flags
        calls = mock_output.call_args_list
        self.assertEqual(len(calls), 1)
        push_args = calls[0][0]
        self.assertNotIn('--force', ' '.join(push_args))
        self.assertNotIn('fetch', ' '.join(push_args))


class TestConfigFile(unittest.TestCase):
    """Tests for config file support."""

    def setUp(self):
        """Set up test fixtures."""
        self.config_dir = tempfile.mkdtemp()
        self.config_file = os.path.join(self.config_dir, 'pickman.conf')

    def tearDown(self):
        """Clean up test fixtures."""
        shutil.rmtree(self.config_dir)

    def test_get_token_from_config(self):
        """Test getting token from config file."""
        with open(self.config_file, 'w', encoding='utf-8') as fhandle:
            fhandle.write('[gitlab]\ntoken = test-config-token\n')

        with mock.patch.object(gitlab, 'CONFIG_FILE', self.config_file):
            token = gitlab.get_token()
        self.assertEqual(token, 'test-config-token')

    def test_get_token_fallback_to_env(self):
        """Test falling back to environment variable."""
        # Config file doesn't exist
        with mock.patch.object(gitlab, 'CONFIG_FILE', '/nonexistent/path'):
            with mock.patch.dict(os.environ, {'GITLAB_TOKEN': 'env-token'}):
                token = gitlab.get_token()
        self.assertEqual(token, 'env-token')

    def test_get_token_config_missing_section(self):
        """Test config file without gitlab section."""
        with open(self.config_file, 'w', encoding='utf-8') as fhandle:
            fhandle.write('[other]\nkey = value\n')

        with mock.patch.object(gitlab, 'CONFIG_FILE', self.config_file):
            with mock.patch.dict(os.environ, {'GITLAB_TOKEN': 'env-token'}):
                token = gitlab.get_token()
        self.assertEqual(token, 'env-token')

    def test_get_config_value(self):
        """Test get_config_value function."""
        with open(self.config_file, 'w', encoding='utf-8') as fhandle:
            fhandle.write('[section1]\nkey1 = value1\n')

        with mock.patch.object(gitlab, 'CONFIG_FILE', self.config_file):
            value = gitlab.get_config_value('section1', 'key1')
        self.assertEqual(value, 'value1')


class TestCheckPermissions(unittest.TestCase):
    """Tests for check_permissions function."""

    @mock.patch.object(gitlab, 'get_remote_url')
    @mock.patch.object(gitlab, 'get_token')
    @mock.patch.object(gitlab, 'AVAILABLE', True)
    def test_check_permissions_developer(self, mock_token, mock_url):
        """Test checking permissions for a developer."""
        mock_token.return_value = 'test-token'
        mock_url.return_value = 'git@gitlab.com:group/project.git'

        mock_user = mock.MagicMock()
        mock_user.username = 'testuser'
        mock_user.id = 123

        mock_member = mock.MagicMock()
        mock_member.access_level = 30  # Developer

        mock_project = mock.MagicMock()
        mock_project.members.get.return_value = mock_member

        mock_glab = mock.MagicMock()
        mock_glab.user = mock_user
        mock_glab.projects.get.return_value = mock_project

        with mock.patch('gitlab.Gitlab', return_value=mock_glab):
            perms = gitlab.check_permissions('origin')

        self.assertIsNotNone(perms)
        self.assertEqual(perms.user, 'testuser')
        self.assertEqual(perms.access_level, 30)
        self.assertEqual(perms.access_name, 'Developer')
        self.assertTrue(perms.can_push)
        self.assertTrue(perms.can_create_mr)
        self.assertFalse(perms.can_merge)

    @mock.patch.object(gitlab, 'AVAILABLE', False)
    def test_check_permissions_not_available(self):
        """Test check_permissions when gitlab not available."""
        with terminal.capture():
            perms = gitlab.check_permissions('origin')
        self.assertIsNone(perms)

    @mock.patch.object(gitlab, 'get_token')
    @mock.patch.object(gitlab, 'AVAILABLE', True)
    def test_check_permissions_no_token(self, mock_token):
        """Test check_permissions when no token set."""
        mock_token.return_value = None
        with terminal.capture():
            perms = gitlab.check_permissions('origin')
        self.assertIsNone(perms)


class TestUpdateMrDescription(unittest.TestCase):
    """Tests for update_mr_desc function."""

    @mock.patch.object(gitlab, 'get_remote_url')
    @mock.patch.object(gitlab, 'get_token')
    @mock.patch.object(gitlab, 'AVAILABLE', True)
    def test_update_mr_desc_success(self, mock_token, mock_url):
        """Test successful MR description update."""
        mock_token.return_value = 'test-token'
        mock_url.return_value = 'git@gitlab.com:group/project.git'

        mock_mr = mock.MagicMock()
        mock_project = mock.MagicMock()
        mock_project.mergerequests.get.return_value = mock_mr

        with mock.patch('gitlab.Gitlab') as mock_gitlab:
            mock_gitlab.return_value.projects.get.return_value = mock_project

            result = gitlab.update_mr_desc('origin', 123,
                                                      'New description')

            self.assertTrue(result)
            self.assertEqual(mock_mr.description, 'New description')
            mock_mr.save.assert_called_once()

    @mock.patch.object(gitlab, 'AVAILABLE', False)
    def test_update_mr_desc_not_available(self):
        """Test update_mr_desc when gitlab not available."""
        with terminal.capture():
            result = gitlab.update_mr_desc('origin', 123, 'desc')
        self.assertFalse(result)

    @mock.patch.object(gitlab, 'get_token')
    @mock.patch.object(gitlab, 'AVAILABLE', True)
    def test_update_mr_desc_no_token(self, mock_token):
        """Test update_mr_desc when no token set."""
        mock_token.return_value = None
        with terminal.capture():
            result = gitlab.update_mr_desc('origin', 123, 'desc')
        self.assertFalse(result)


class TestGetPickmanMrs(unittest.TestCase):
    """Tests for get_pickman_mrs function."""

    @mock.patch.object(gitlab, 'get_remote_url')
    @mock.patch.object(gitlab, 'get_token')
    @mock.patch.object(gitlab, 'AVAILABLE', True)
    def test_get_pickman_mrs_sorted_oldest_first(self, mock_token, mock_url):
        """Test that MRs are requested sorted by created_at ascending."""
        mock_token.return_value = 'test-token'
        mock_url.return_value = 'git@gitlab.com:group/project.git'

        # Create mock MRs with [pickman] in the title
        mock_mr1 = mock.MagicMock()
        mock_mr1.iid = 1
        mock_mr1.title = '[pickman] Older MR'
        mock_mr1.web_url = TEST_MR_1_URL
        mock_mr1.source_branch = 'cherry-1'
        mock_mr1.description = 'desc1'
        mock_mr1.has_conflicts = False
        mock_mr1.detailed_merge_status = 'mergeable'
        mock_mr1.diverged_commits_count = 0

        mock_mr2 = mock.MagicMock()
        mock_mr2.iid = 2
        mock_mr2.title = '[pickman] Newer MR'
        mock_mr2.web_url = 'https://gitlab.com/mr/2'
        mock_mr2.source_branch = 'cherry-2'
        mock_mr2.description = 'desc2'
        mock_mr2.has_conflicts = False
        mock_mr2.detailed_merge_status = 'mergeable'
        mock_mr2.diverged_commits_count = 0

        mock_project = mock.MagicMock()
        # Return MRs in the order they would come from GitLab (oldest first)
        mock_project.mergerequests.list.return_value = [mock_mr1, mock_mr2]
        mock_project.mergerequests.get.side_effect = [mock_mr1, mock_mr2]

        with mock.patch('gitlab.Gitlab') as mock_gitlab:
            mock_gitlab.return_value.projects.get.return_value = mock_project

            result = gitlab.get_pickman_mrs('origin', state='opened')

        # Verify the list call includes sorting parameters
        mock_project.mergerequests.list.assert_called_once_with(
            state='opened', order_by='created_at', sort='asc', get_all=True)

        # Verify we got both MRs in order
        self.assertEqual(len(result), 2)
        self.assertEqual(result[0].iid, 1)
        self.assertEqual(result[1].iid, 2)


class TestCreateMr(unittest.TestCase):
    """Tests for create_mr function."""

    @mock.patch.object(gitlab, 'get_token')
    @mock.patch.object(gitlab, 'AVAILABLE', True)
    def test_create_mr_409_returns_existing(self, mock_token):
        """Test that 409 error returns existing MR URL."""
        tout.init(tout.INFO)
        mock_token.return_value = 'test-token'

        # Create mock existing MR
        mock_existing_mr = mock.MagicMock()
        mock_existing_mr.web_url = TEST_MR_URL

        mock_project = mock.MagicMock()
        mock_project.mergerequests.list.return_value = [mock_existing_mr]

        # Simulate 409 Conflict error
        mock_project.mergerequests.create.side_effect = \
            gitlab.MrCreateError(response_code=409)

        with mock.patch('gitlab.Gitlab') as mock_gitlab:
            mock_gitlab.return_value.projects.get.return_value = mock_project

            with terminal.capture():
                result = gitlab.create_mr(
                    'gitlab.com', 'group/project',
                    'cherry-abc', 'master', 'Test MR')

        self.assertEqual(result, mock_existing_mr.web_url)
        mock_project.mergerequests.list.assert_called_once_with(
            source_branch='cherry-abc', state='opened')


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
        self.assertEqual(args.max_mrs, 5)
        self.assertEqual(args.remote, 'ci')
        self.assertEqual(args.target, 'master')

    def test_parse_step_with_options(self):
        """Test parsing step command with all options."""
        args = pickman.parse_args(['step', 'us/next', '-m', '3',
                                   '-r', 'origin', '-t', 'main'])
        self.assertEqual(args.cmd, 'step')
        self.assertEqual(args.source, 'us/next')
        self.assertEqual(args.max_mrs, 3)
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

    def test_parse_mr_description_ignores_chashes(self):
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
        mock_mr = gitlab.PickmanMr(
            iid=123,
            title='[pickman] Test MR',
            web_url='https://gitlab.com/mr/123',
            source_branch='cherry-test',
            description='Test',
        )
        with mock.patch.object(control, 'run_git'):
            with mock.patch.object(gitlab, 'get_merged_pickman_mrs',
                                   return_value=[]):
                with mock.patch.object(gitlab, 'get_open_pickman_mrs',
                                       return_value=[mock_mr]):
                    args = argparse.Namespace(cmd='step', source='us/next',
                                              remote='ci', target='master',
                                              max_mrs=1)
                    with terminal.capture():
                        ret = control.do_step(args, None)

        self.assertEqual(ret, 0)

    def test_step_gitlab_error(self):
        """Test step when GitLab API returns error."""
        with mock.patch.object(gitlab, 'get_merged_pickman_mrs',
                               return_value=None):
            args = argparse.Namespace(cmd='step', source='us/next',
                                      remote='ci', target='master',
                                      max_mrs=5)
            with terminal.capture():
                ret = control.do_step(args, None)

        self.assertEqual(ret, 1)

    def test_step_open_mrs_error(self):
        """Test step when get_open_pickman_mrs returns error."""
        with mock.patch.object(gitlab, 'get_merged_pickman_mrs',
                               return_value=[]):
            with mock.patch.object(gitlab, 'get_open_pickman_mrs',
                                   return_value=None):
                args = argparse.Namespace(cmd='step', source='us/next',
                                          remote='ci', target='master',
                                          max_mrs=5)
                with terminal.capture():
                    ret = control.do_step(args, None)

        self.assertEqual(ret, 1)

    def test_step_allows_below_max(self):
        """Test step allows new MR when count is below max_mrs."""
        mock_mr = gitlab.PickmanMr(
            iid=123,
            title='[pickman] Test MR',
            web_url='https://gitlab.com/mr/123',
            source_branch='cherry-test',
            description='Test',
        )
        with mock.patch.object(control, 'run_git'):
            with mock.patch.object(gitlab, 'get_merged_pickman_mrs',
                                   return_value=[]):
                with mock.patch.object(gitlab, 'get_open_pickman_mrs',
                                       return_value=[mock_mr]):
                    with mock.patch.object(control, 'do_apply',
                                           return_value=0) as mock_apply:
                        args = argparse.Namespace(cmd='step', source='us/next',
                                                  remote='ci', target='master',
                                                  max_mrs=5)
                        with terminal.capture():
                            ret = control.do_step(args, None)

        # With 1 open MR and max_mrs=5, it should try to create a new one
        self.assertEqual(ret, 0)
        mock_apply.assert_called_once()

    def test_step_blocks_at_max(self):
        """Test step blocks new MR when at max_mrs limit."""
        mock_mrs = [
            gitlab.PickmanMr(
                iid=i,
                title=f'[pickman] Test MR {i}',
                web_url=f'https://gitlab.com/mr/{i}',
                source_branch=f'cherry-test-{i}',
                description='Test',
            )
            for i in range(3)
        ]
        with mock.patch.object(control, 'run_git'):
            with mock.patch.object(gitlab, 'get_merged_pickman_mrs',
                                   return_value=[]):
                with mock.patch.object(gitlab, 'get_open_pickman_mrs',
                                       return_value=mock_mrs):
                    with mock.patch.object(control, 'do_apply') as mock_apply:
                        args = argparse.Namespace(cmd='step', source='us/next',
                                                  remote='ci', target='master',
                                                  max_mrs=3)
                        with terminal.capture():
                            ret = control.do_step(args, None)

        # With 3 open MRs and max_mrs=3, should not create new MR
        self.assertEqual(ret, 0)
        mock_apply.assert_not_called()


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
        with mock.patch.object(gitlab, 'get_open_pickman_mrs',
                               return_value=[]):
            args = argparse.Namespace(cmd='review', remote='ci')
            with terminal.capture():
                ret = control.do_review(args, None)

        self.assertEqual(ret, 0)

    def test_review_gitlab_error(self):
        """Test review when GitLab API returns error."""
        with mock.patch.object(gitlab, 'get_open_pickman_mrs',
                               return_value=None):
            args = argparse.Namespace(cmd='review', remote='ci')
            with terminal.capture():
                ret = control.do_review(args, None)

        self.assertEqual(ret, 1)


class TestUpdateHistoryWithReview(unittest.TestCase):
    """Tests for update_history function."""

    def setUp(self):
        """Set up test fixtures."""
        self.test_dir = tempfile.mkdtemp()
        self.orig_dir = os.getcwd()
        os.chdir(self.test_dir)

        # Initialize git repo
        subprocess.run(['git', 'init'], check=True, capture_output=True)
        subprocess.run(['git', 'config', 'user.email', 'test@test.com'],
                       check=True, capture_output=True)
        subprocess.run(['git', 'config', 'user.name', 'Test'],
                       check=True, capture_output=True)

    def tearDown(self):
        """Clean up test fixtures."""
        os.chdir(self.orig_dir)
        shutil.rmtree(self.test_dir)

    def test_update_history(self):
        """Test that review handling is appended to history."""
        comments = [
            gitlab.MrComment(id=1, author='reviewer1',
                                 body='Please fix the indentation here',
                                 created_at='2025-01-01', resolvable=True,
                                 resolved=False),
            gitlab.MrComment(id=2, author='reviewer2', body='Add a docstring',
                                 created_at='2025-01-01', resolvable=True,
                                 resolved=False),
        ]
        conversation_log = 'Fixed indentation and added docstring.'

        control.update_history('cherry-abc123', comments,
                                           conversation_log)

        # Check history file was created
        self.assertTrue(os.path.exists(control.HISTORY_FILE))

        with open(control.HISTORY_FILE, 'r', encoding='utf-8') as fhandle:
            content = fhandle.read()

        self.assertIn('### Review:', content)
        self.assertIn('Branch: cherry-abc123', content)
        self.assertIn('reviewer1', content)
        self.assertIn('reviewer2', content)
        self.assertIn('Fixed indentation', content)

    def test_update_history_appends(self):
        """Test that review handling appends to existing history."""
        # Create existing history
        with open(control.HISTORY_FILE, 'w', encoding='utf-8') as fhandle:
            fhandle.write('Existing history content\n')
        subprocess.run(['git', 'add', control.HISTORY_FILE],
                       check=True, capture_output=True)
        subprocess.run(['git', 'commit', '-m', 'Initial'],
                       check=True, capture_output=True)

        comms = [gitlab.MrComment(id=1, author='reviewer', body='Fix this',
                                      created_at='2025-01-01', resolvable=True,
                                      resolved=False)]
        control.update_history('cherry-xyz', comms, 'Fixed it')

        with open(control.HISTORY_FILE, 'r', encoding='utf-8') as fhandle:
            content = fhandle.read()

        self.assertIn('Existing history content', content)
        self.assertIn('### Review:', content)


class TestProcessMrReviewsCommentTracking(unittest.TestCase):
    """Tests for comment tracking in process_mr_reviews."""

    def setUp(self):
        """Set up test fixtures."""
        fd, self.db_path = tempfile.mkstemp(suffix='.db')
        os.close(fd)
        os.unlink(self.db_path)
        self.test_dir = tempfile.mkdtemp()
        self.orig_dir = os.getcwd()
        os.chdir(self.test_dir)

        # Initialize git repo
        subprocess.run(['git', 'init'], check=True, capture_output=True)
        subprocess.run(['git', 'config', 'user.email', 'test@test.com'],
                       check=True, capture_output=True)
        subprocess.run(['git', 'config', 'user.name', 'Test'],
                       check=True, capture_output=True)

    def tearDown(self):
        """Clean up test fixtures."""
        os.chdir(self.orig_dir)
        shutil.rmtree(self.test_dir)
        if os.path.exists(self.db_path):
            os.unlink(self.db_path)
        database.Database.instances.clear()

    def test_skips_processed_comments(self):
        """Test that already-processed comments are skipped."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()

            # Mark comment as processed
            dbs.comment_mark_processed(100, 1)
            dbs.commit()

            mrs = [gitlab.PickmanMr(
                iid=100,
                title='[pickman] Test MR',
                source_branch='cherry-test',
                description='Test',
                web_url='https://gitlab.com/mr/100',
            )]

            # Comment 1 is processed, comment 2 is new
            comments = [
                gitlab.MrComment(id=1, author='reviewer', body='Old comment',
                                     created_at='2025-01-01', resolvable=True,
                                     resolved=False),
                gitlab.MrComment(id=2, author='reviewer', body='New comment',
                                     created_at='2025-01-01', resolvable=True,
                                     resolved=False),
            ]

            with mock.patch.object(control, 'run_git'):
                with mock.patch.object(gitlab, 'get_mr_comments',
                                       return_value=comments):
                    with mock.patch.object(agent, 'handle_mr_comments',
                                           return_value=(True, 'Done')) as moc:
                        with mock.patch.object(gitlab, 'update_mr_desc'):
                            with mock.patch.object(control, 'update_history'):
                                control.process_mr_reviews('ci', mrs, dbs)

            # Agent should only receive the new comment
            call_args = moc.call_args
            passed_comments = call_args[0][2]
            self.assertEqual(len(passed_comments), 1)
            self.assertEqual(passed_comments[0].id, 2)

            dbs.close()

    def test_rebase_without_comments(self):
        """Test that MRs needing rebase trigger agent even without comments."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()

            # MR needs rebase but has no comments
            mrs = [gitlab.PickmanMr(
                iid=100,
                title='[pickman] Test MR',
                source_branch='cherry-test',
                description='Test',
                web_url='https://gitlab.com/mr/100',
                has_conflicts=False,
                needs_rebase=True,
            )]

            with mock.patch.object(control, 'run_git'):
                with mock.patch.object(gitlab, 'get_mr_comments',
                                       return_value=[]):
                    with mock.patch.object(agent, 'handle_mr_comments',
                                           return_value=(True, 'Rebased')) as m:
                        with mock.patch.object(gitlab, 'update_mr_desc'):
                            with mock.patch.object(control, 'update_history'):
                                control.process_mr_reviews('ci', mrs, dbs)

            # Agent should be called with needs_rebase=True
            m.assert_called_once()
            call_kwargs = m.call_args[1]
            self.assertTrue(call_kwargs.get('needs_rebase'))
            self.assertFalse(call_kwargs.get('has_conflicts'))

            dbs.close()

    def test_skips_mr_no_rebase_no_comments(self):
        """Test that MRs without rebase need or comments are skipped."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()

            # MR has no comments and doesn't need rebase
            mrs = [gitlab.PickmanMr(
                iid=100,
                title='[pickman] Test MR',
                source_branch='cherry-test',
                description='Test',
                web_url='https://gitlab.com/mr/100',
                has_conflicts=False,
                needs_rebase=False,
            )]

            with mock.patch.object(control, 'run_git'):
                with mock.patch.object(gitlab, 'get_mr_comments',
                                       return_value=[]):
                    with mock.patch.object(agent, 'handle_mr_comments',
                                           return_value=(True, 'Done')) as moc:
                        control.process_mr_reviews('ci', mrs, dbs)

            # Agent should NOT be called
            moc.assert_not_called()

            dbs.close()


class TestParsePoll(unittest.TestCase):
    """Tests for poll command argument parsing."""

    def test_parse_poll_defaults(self):
        """Test parsing poll command with defaults."""
        args = pickman.parse_args(['poll', 'us/next'])
        self.assertEqual(args.cmd, 'poll')
        self.assertEqual(args.source, 'us/next')
        self.assertEqual(args.interval, 300)
        self.assertEqual(args.max_mrs, 5)
        self.assertEqual(args.remote, 'ci')
        self.assertEqual(args.target, 'master')

    def test_parse_poll_with_options(self):
        """Test parsing poll command with all options."""
        args = pickman.parse_args([
            'poll', 'us/next',
            '-i', '60', '-m', '3', '-r', 'origin', '-t', 'main'
        ])
        self.assertEqual(args.cmd, 'poll')
        self.assertEqual(args.source, 'us/next')
        self.assertEqual(args.interval, 60)
        self.assertEqual(args.max_mrs, 3)
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


class TestParseInstruction(unittest.TestCase):
    """Tests for parse_instruction function."""

    def test_pickman_skip(self):
        """Test 'pickman skip' format."""
        self.assertEqual(control.parse_instruction('pickman skip'), 'skip')

    def test_pickman_colon_skip(self):
        """Test 'pickman: skip' format."""
        self.assertEqual(control.parse_instruction('pickman: skip'), 'skip')

    def test_at_pickman_skip(self):
        """Test '@pickman skip' format."""
        self.assertEqual(control.parse_instruction('@pickman skip'), 'skip')

    def test_at_pickman_colon_skip(self):
        """Test '@pickman: skip' format."""
        self.assertEqual(control.parse_instruction('@pickman: skip'), 'skip')

    def test_pickman_unskip(self):
        """Test 'pickman unskip' format."""
        self.assertEqual(control.parse_instruction('pickman unskip'), 'unskip')

    def test_at_pickman_unskip(self):
        """Test '@pickman unskip' format."""
        self.assertEqual(control.parse_instruction('@pickman unskip'), 'unskip')

    def test_case_insensitive(self):
        """Test case insensitivity."""
        self.assertEqual(control.parse_instruction('PICKMAN SKIP'), 'skip')
        self.assertEqual(control.parse_instruction('Pickman: Skip'), 'skip')

    def test_in_longer_text(self):
        """Test instruction embedded in longer comment."""
        body = 'Please pickman skip this MR, it does not apply'
        self.assertEqual(control.parse_instruction(body), 'skip')

    def test_no_instruction(self):
        """Test comment without pickman instruction."""
        self.assertIsNone(control.parse_instruction('Just a regular comment'))

    def test_pickman_without_command(self):
        """Test 'pickman' alone without a command."""
        self.assertIsNone(control.parse_instruction('pickman'))

    def test_has_instruction(self):
        """Test has_instruction helper."""
        self.assertTrue(control.has_instruction('pickman skip', 'skip'))
        self.assertTrue(control.has_instruction('@pickman: unskip', 'unskip'))
        self.assertFalse(control.has_instruction('pickman skip', 'unskip'))
        self.assertFalse(control.has_instruction('regular comment', 'skip'))


class TestFormatHistorySummary(unittest.TestCase):
    """Tests for format_history function."""

    def test_format_history(self):
        """Test formatting history summary."""
        commits = [
            control.CommitInfo('aaa111', 'aaa111a', 'First commit', 'Author 1'),
            control.CommitInfo('bbb222', 'bbb222b', 'Second one', 'Author 2'),
        ]
        result = control.format_history('us/next', commits, 'cherry-abc')

        self.assertIn('us/next', result)
        self.assertIn('Branch: cherry-abc', result)
        self.assertIn('- aaa111a First commit', result)
        self.assertIn('- bbb222b Second one', result)


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
            control.CommitInfo('bbb222', 'bbb222b', 'Second one', 'Author 2'),
            control.CommitInfo('ccc333', 'ccc333c', 'Third commit', 'Author 3'),
        ]
        content, commit_msg = control.get_history(
            self.history_file, 'us/next', commits, 'cherry-abc', 'Log')

        # Verify all commits in content
        self.assertIn('- aaa111a First commit', content)
        self.assertIn('- bbb222b Second one', content)
        self.assertIn('- ccc333c Third commit', content)

        # Verify commit message
        self.assertIn('pickman: Record cherry-pick of 3 commits', commit_msg)
        self.assertIn('- aaa111a First commit', commit_msg)
        self.assertIn('- bbb222b Second one', commit_msg)
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

    def test_prepare_apply_deletes_existing_branch(self):
        """Test prepare_apply deletes a branch that already exists."""
        git_cmds = []

        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'abc123')
            dbs.commit()

            log_output = 'aaa111|aaa111a|Author 1|First commit|abc123\n'

            def mock_git(pipe_list):
                cmd = pipe_list[0] if pipe_list else []
                git_cmds.append(cmd)
                if 'log' in cmd:
                    return command.CommandResult(stdout=log_output)
                if 'rev-parse' in cmd:
                    return command.CommandResult(stdout='master')
                if 'branch' in cmd and '--list' in cmd:
                    return command.CommandResult(stdout='cherry-aaa111a\n')
                return command.CommandResult(stdout='')

            command.TEST_RESULT = mock_git

            info, ret = control.prepare_apply(dbs, 'us/next', None)

            self.assertIsNotNone(info)
            self.assertEqual(ret, 0)
            self.assertTrue(
                any('branch' in c and '-D' in c for c in git_cmds))
            dbs.close()

    def test_prepare_apply_merge_found(self):
        """Test prepare_apply sets merge_found and advance_to."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'abc123')
            dbs.commit()

            merge_hash = 'ccc333ccc333ccc333'

            merge_info = control.NextCommitsInfo(
                commits=[
                    control.CommitInfo('aaa111', 'aaa111a', 'First commit',
                                       'Author 1'),
                    control.CommitInfo('bbb222', 'bbb222b', 'Second commit',
                                       'Author 2'),
                ],
                merge_found=True,
                advance_to=merge_hash,
            )

            def mock_git(pipe_list):
                cmd = pipe_list[0] if pipe_list else []
                if 'rev-parse' in cmd:
                    return command.CommandResult(stdout='master')
                return command.CommandResult(stdout='')

            with mock.patch.object(control, 'get_next_commits',
                                   return_value=(merge_info, None)):
                command.TEST_RESULT = mock_git
                info, ret = control.prepare_apply(dbs, 'us/next', None)

            self.assertIsNotNone(info)
            self.assertEqual(ret, 0)
            self.assertTrue(info.merge_found)
            self.assertEqual(info.advance_to, merge_hash)
            self.assertEqual(len(info.commits), 2)
            dbs.close()

    def test_prepare_apply_no_merge(self):
        """Test prepare_apply reports no merge found."""
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
            self.assertFalse(info.merge_found)
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
                with mock.patch.object(control, 'run_git',
                                       return_value='abc123'):
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
                with mock.patch.object(control, 'run_git',
                                       return_value='abc123'):
                    with mock.patch.object(gitlab, 'push_and_create_mr',
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
                with mock.patch.object(control, 'run_git',
                                       return_value='abc123'):
                    with mock.patch.object(gitlab, 'push_and_create_mr',
                                           return_value=None):
                        ret, success, _ = control.execute_apply(
                            dbs, 'us/next', commits, 'cherry-branch', args)

            self.assertEqual(ret, 1)
            self.assertTrue(success)  # cherry-pick succeeded, MR failed
            dbs.close()

    def test_execute_apply_agent_aborted(self):
        """Test execute_apply when agent aborts and branch doesn't exist."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'abc123')
            dbs.commit()

            commits = [control.CommitInfo('fff666', 'fff666f', 'Test commit',
                                          'Author')]
            args = argparse.Namespace(push=False)

            # Agent reports success but branch doesn't exist (agent aborted)
            with mock.patch.object(control.agent, 'cherry_pick_commits',
                                   return_value=(True, 'aborted log')):
                with mock.patch.object(control, 'run_git',
                                       side_effect=Exception('not found')):
                    ret, success, _ = control.execute_apply(
                        dbs, 'us/next', commits, 'cherry-branch', args)

            # Should detect failure since branch doesn't exist
            self.assertEqual(ret, 1)
            self.assertFalse(success)

            # Check commit status is conflict (not applied)
            commit_rec = dbs.commit_get('fff666')
            self.assertEqual(commit_rec[6], 'conflict')
            dbs.close()

    def test_execute_apply_already_applied(self):
        """Test execute_apply when agent detects commits already applied."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'abc123')
            dbs.commit()

            commits = [control.CommitInfo('ggg777', 'ggg777g', 'Test commit',
                                          'Author'),
                       control.CommitInfo('hhh888', 'hhh888h', 'Merge commit',
                                          'Author')]
            args = argparse.Namespace(push=False)

            # Agent returns success but leaves signal file
            with mock.patch.object(control.agent, 'cherry_pick_commits',
                                   return_value=(True, 'already applied log')):
                with mock.patch.object(control.agent, 'read_signal_file',
                                       return_value=(agent.SIGNAL_APPLIED,
                                                     'hhh888')):
                    ret, success, _ = control.execute_apply(
                        dbs, 'us/next', commits, 'cherry-branch', args)

            # Should return success (skip MR created), but success=False
            self.assertEqual(ret, 0)
            self.assertFalse(success)

            # Check commits are marked as skipped
            commit_rec = dbs.commit_get('ggg777')
            self.assertEqual(commit_rec[6], 'skipped')
            commit_rec = dbs.commit_get('hhh888')
            self.assertEqual(commit_rec[6], 'skipped')

            # Check source was updated
            source_commit = dbs.source_get('us/next')
            self.assertEqual(source_commit, 'hhh888')
            dbs.close()


class TestSignalFile(unittest.TestCase):
    """Tests for signal file handling."""

    def setUp(self):
        """Set up test fixtures."""
        self.test_dir = tempfile.mkdtemp()
        self.signal_path = os.path.join(self.test_dir, agent.SIGNAL_FILE)

    def tearDown(self):
        """Clean up test fixtures."""
        if os.path.exists(self.signal_path):
            os.unlink(self.signal_path)
        os.rmdir(self.test_dir)

    def test_read_signal_file_not_exists(self):
        """Test read_signal_file when file doesn't exist."""
        status, commit = agent.read_signal_file(self.test_dir)
        self.assertIsNone(status)
        self.assertIsNone(commit)

    def test_read_signal_file_already_applied(self):
        """Test read_signal_file with already_applied status."""
        with open(self.signal_path, 'w', encoding='utf-8') as fhandle:
            fhandle.write('already_applied\nabc123def456\n')

        status, commit = agent.read_signal_file(self.test_dir)
        self.assertEqual(status, 'already_applied')
        self.assertEqual(commit, 'abc123def456')

        # File should be removed after reading
        self.assertFalse(os.path.exists(self.signal_path))

    def test_read_signal_file_status_only(self):
        """Test read_signal_file with only status line."""
        with open(self.signal_path, 'w', encoding='utf-8') as fhandle:
            fhandle.write('conflict\n')

        status, commit = agent.read_signal_file(self.test_dir)
        self.assertEqual(status, 'conflict')
        self.assertIsNone(commit)

        self.assertFalse(os.path.exists(self.signal_path))


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

            info, err = control.get_next_commits(dbs, 'us/next')
            self.assertIsNone(err)
            self.assertFalse(info.merge_found)
            self.assertEqual(len(info.commits), 2)
            dbs.close()

    def test_get_next_commits_skips_db_commits(self):
        """Test get_next_commits skips commits already in database."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'abc123')

            # Add first commit to database (simulating pending MR)
            source_id = dbs.source_get_id('us/next')
            dbs.commit_add('aaa111', source_id, 'First commit', 'Author 1',
                           status='pending')
            dbs.commit()

            # Log output with two commits, first already in DB
            log_output = (
                'aaa111|aaa111a|Author 1|First commit|abc123\n'
                'bbb222|bbb222b|Author 2|Second commit|aaa111\n'
            )
            command.TEST_RESULT = command.CommandResult(stdout=log_output)

            info, err = control.get_next_commits(dbs, 'us/next')
            self.assertIsNone(err)
            self.assertFalse(info.merge_found)
            # Only second commit should be returned (first is in DB)
            self.assertEqual(len(info.commits), 1)
            self.assertEqual(info.commits[0].chash, 'bbb222b')
            dbs.close()

    def test_get_next_commits_all_in_db(self):
        """Test get_next_commits returns empty when all commits in database."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'abc123')

            # Add both commits to database
            source_id = dbs.source_get_id('us/next')
            dbs.commit_add('aaa111', source_id, 'First commit', 'Author 1',
                           status='pending')
            dbs.commit_add('bbb222', source_id, 'Second commit', 'Author 2',
                           status='pending')
            dbs.commit()

            # Log output with two commits, both in DB
            log_output = (
                'aaa111|aaa111a|Author 1|First commit|abc123\n'
                'bbb222|bbb222b|Author 2|Second commit|aaa111\n'
            )
            command.TEST_RESULT = command.CommandResult(stdout=log_output)

            info, err = control.get_next_commits(dbs, 'us/next')
            self.assertIsNone(err)
            self.assertFalse(info.merge_found)
            # No commits should be returned (all in DB)
            self.assertEqual(len(info.commits), 0)
            dbs.close()

    def test_get_next_commits_skips_processed_merge(self):
        """Test get_next_commits skips merge with all commits in database."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'abc123')

            # Add commits from first merge to database (simulating pending MR)
            source_id = dbs.source_get_id('us/next')
            dbs.commit_add('aaa111', source_id, 'First commit', 'Author 1',
                           status='pending')
            dbs.commit_add('merge1', source_id, 'Merge branch', 'Author 2',
                           status='pending')
            dbs.commit()

            # First-parent log shows two merges
            fp_log = (
                'aaa111|aaa111a|Author 1|First commit|abc123\n'
                'merge1|merge1m|Author 2|Merge branch|aaa111 side1\n'
                'ccc333|ccc333c|Author 3|Third commit|merge1\n'
                'merge2|merge2m|Author 4|Second merge|ccc333 side2\n'
            )

            # When asked for first merge's commits (all in DB)
            merge1_log = (
                'aaa111|aaa111a|Author 1|First commit|abc123\n'
                'merge1|merge1m|Author 2|Merge branch|aaa111 side1\n'
            )

            # When asked for second merge's commits (not in DB)
            merge2_log = (
                'ccc333|ccc333c|Author 3|Third commit|merge1\n'
                'merge2|merge2m|Author 4|Second merge|ccc333 side2\n'
            )

            def mock_git(pipe_list):
                cmd = pipe_list[0] if pipe_list else []
                if '--first-parent' in cmd and '--merges' in cmd:
                    # detect_sub_merges: no sub-merges
                    return command.CommandResult(stdout='')
                if '--first-parent' in cmd:
                    return command.CommandResult(stdout=fp_log)
                if 'rev-parse' in cmd:
                    # detect_sub_merges: return parents for merges
                    return command.CommandResult(stdout='aaa111\nside1\n')
                # Determine which merge range by checking the cmd args
                cmd_str = ' '.join(cmd)
                if 'merge1' in cmd_str and 'abc123' in cmd_str:
                    return command.CommandResult(stdout=merge1_log)
                if 'merge2' in cmd_str and 'merge1' in cmd_str:
                    return command.CommandResult(stdout=merge2_log)
                return command.CommandResult(stdout=merge2_log)

            command.TEST_RESULT = mock_git

            info, err = control.get_next_commits(dbs, 'us/next')
            self.assertIsNone(err)
            self.assertTrue(info.merge_found)
            # Should return commits from second merge (first was skipped)
            self.assertEqual(len(info.commits), 2)
            self.assertEqual(info.commits[0].chash, 'ccc333c')
            self.assertEqual(info.commits[1].chash, 'merge2m')
            dbs.close()


class TestDetectSubMerges(unittest.TestCase):
    """Tests for detect_sub_merges function."""

    def tearDown(self):
        """Clean up test fixtures."""
        command.TEST_RESULT = None

    def test_not_a_merge(self):
        """Test detect_sub_merges returns empty for non-merge commit."""
        # Single parent means not a merge
        command.TEST_RESULT = command.CommandResult(stdout='abc123\n')
        result = control.detect_sub_merges('abc123')
        self.assertEqual(result, [])

    def test_no_sub_merges(self):
        """Test detect_sub_merges returns empty when no sub-merges exist."""
        call_count = [0]

        def mock_git(pipe_list):  # pylint: disable=unused-argument
            call_count[0] += 1
            if call_count[0] == 1:
                # rev-parse ^@ returns two parents (it's a merge)
                return command.CommandResult(stdout='parent1\nparent2\n')
            # log --merges returns empty (no sub-merges)
            return command.CommandResult(stdout='')

        command.TEST_RESULT = mock_git
        result = control.detect_sub_merges('merge123')
        self.assertEqual(result, [])

    def test_found_sub_merges(self):
        """Test detect_sub_merges finds sub-merges."""
        call_count = [0]

        def mock_git(pipe_list):  # pylint: disable=unused-argument
            call_count[0] += 1
            if call_count[0] == 1:
                # rev-parse ^@ returns two parents
                return command.CommandResult(stdout='parent1\nparent2\n')
            # log --merges returns sub-merge hashes
            return command.CommandResult(
                stdout='sub_merge1\nsub_merge2\nsub_merge3\n')

        command.TEST_RESULT = mock_git
        result = control.detect_sub_merges('mega_merge')
        self.assertEqual(result, ['sub_merge1', 'sub_merge2', 'sub_merge3'])

    def test_error_handling(self):
        """Test detect_sub_merges returns empty on git error."""
        def mock_git_fail(**_kwargs):
            raise command.CommandExc('git error', command.CommandResult())

        command.TEST_RESULT = mock_git_fail
        result = control.detect_sub_merges('bad_hash')
        self.assertEqual(result, [])


class TestDecomposeMegaMerge(unittest.TestCase):
    """Tests for decompose_mega_merge function."""

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

    def test_first_batch_mainline(self):
        """Test decompose returns mainline commits first."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'base')
            dbs.commit()

            call_count = [0]

            def mock_git(pipe_list):  # pylint: disable=unused-argument
                call_count[0] += 1
                if call_count[0] == 1:
                    # rev-parse ^@ for mega-merge parents
                    return command.CommandResult(
                        stdout='first_parent\nsecond_parent\n')
                if call_count[0] == 2:
                    # log -1 for mega-merge subject/author (pre-add)
                    return command.CommandResult(
                        stdout='Mega merge subject|Author\n')
                if call_count[0] == 3:
                    # Mainline commits (prev..first_parent)
                    return command.CommandResult(
                        stdout='aaa|aaa1|A|Mainline commit|base\n')
                return command.CommandResult(stdout='')

            command.TEST_RESULT = mock_git

            commits, advance_to = control.decompose_mega_merge(
                dbs, 'base', 'mega_hash', ['sub1', 'sub2'])

            self.assertEqual(len(commits), 1)
            self.assertEqual(commits[0].chash, 'aaa1')
            self.assertEqual(advance_to, 'first_parent')
            dbs.close()

    def test_sub_merge_batch(self):
        """Test decompose returns sub-merge batch when mainline is done."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'base')
            dbs.commit()

            call_count = [0]

            def mock_git(pipe_list):  # pylint: disable=unused-argument
                call_count[0] += 1
                if call_count[0] == 1:
                    # rev-parse ^@ for mega-merge parents
                    return command.CommandResult(
                        stdout='first_parent\nsecond_parent\n')
                if call_count[0] == 2:
                    # log -1 for mega-merge subject/author
                    return command.CommandResult(
                        stdout='Mega merge|Author\n')
                if call_count[0] == 3:
                    # Mainline commits - empty (already processed)
                    return command.CommandResult(stdout='')
                if call_count[0] == 4:
                    # Sub-merge 1 commits
                    return command.CommandResult(
                        stdout='bbb|bbb1|B|Sub commit 1|first_parent\n'
                               'ccc|ccc1|C|Sub commit 2|bbb\n')
                return command.CommandResult(stdout='')

            command.TEST_RESULT = mock_git

            commits, advance_to = control.decompose_mega_merge(
                dbs, 'base', 'mega_hash', ['sub1', 'sub2'])

            self.assertEqual(len(commits), 2)
            self.assertEqual(commits[0].chash, 'bbb1')
            self.assertIsNone(advance_to)
            dbs.close()

    def test_skips_processed_sub_merge(self):
        """Test decompose skips sub-merges already in database."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'base')
            dbs.commit()

            # Add sub-merge 1 commits to database
            source_id = dbs.source_get_id('us/next')
            dbs.commit_add('bbb', source_id, 'Sub commit 1', 'B',
                           status='applied')
            dbs.commit()

            call_count = [0]

            def mock_git(pipe_list):  # pylint: disable=unused-argument
                call_count[0] += 1
                if call_count[0] == 1:
                    return command.CommandResult(
                        stdout='first_parent\nsecond_parent\n')
                if call_count[0] == 2:
                    return command.CommandResult(
                        stdout='Mega merge|Author\n')
                if call_count[0] == 3:
                    # Mainline - empty
                    return command.CommandResult(stdout='')
                if call_count[0] == 4:
                    # Sub-merge 1 commits (already in DB)
                    return command.CommandResult(
                        stdout='bbb|bbb1|B|Sub commit 1|first_parent\n')
                if call_count[0] == 5:
                    # Sub-merge 2 commits (not in DB)
                    return command.CommandResult(
                        stdout='ddd|ddd1|D|Sub commit 3|sub1\n')
                return command.CommandResult(stdout='')

            command.TEST_RESULT = mock_git

            commits, advance_to = control.decompose_mega_merge(
                dbs, 'base', 'mega_hash', ['sub1', 'sub2'])

            self.assertEqual(len(commits), 1)
            self.assertEqual(commits[0].chash, 'ddd1')
            self.assertIsNone(advance_to)
            dbs.close()

    def test_all_done(self):
        """Test decompose returns empty when all sub-merges are processed."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'base')
            dbs.commit()

            # Add all commits to database
            source_id = dbs.source_get_id('us/next')
            dbs.commit_add('bbb', source_id, 'Sub commit 1', 'B',
                           status='applied')
            dbs.commit_add('ddd', source_id, 'Sub commit 2', 'D',
                           status='applied')
            dbs.commit()

            call_count = [0]

            def mock_git(pipe_list):  # pylint: disable=too-many-return-statements,unused-argument
                call_count[0] += 1
                if call_count[0] == 1:
                    return command.CommandResult(
                        stdout='first_parent\nsecond_parent\n')
                if call_count[0] == 2:
                    return command.CommandResult(
                        stdout='Mega merge|Author\n')
                if call_count[0] == 3:
                    # Mainline - empty
                    return command.CommandResult(stdout='')
                if call_count[0] == 4:
                    # Sub-merge 1
                    return command.CommandResult(
                        stdout='bbb|bbb1|B|Sub commit 1|first_parent\n')
                if call_count[0] == 5:
                    # Sub-merge 2
                    return command.CommandResult(
                        stdout='ddd|ddd1|D|Sub commit 2|sub1\n')
                if call_count[0] == 6:
                    # Remainder - empty
                    return command.CommandResult(stdout='')
                return command.CommandResult(stdout='')

            command.TEST_RESULT = mock_git

            commits, advance_to = control.decompose_mega_merge(
                dbs, 'base', 'mega_hash', ['sub1', 'sub2'])

            self.assertEqual(len(commits), 0)
            self.assertIsNone(advance_to)
            dbs.close()


class TestGetNextCommitsMegaMerge(unittest.TestCase):
    """Tests for get_next_commits with mega-merges."""

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

    def test_returns_sub_batch(self):
        """Test get_next_commits returns sub-merge batch for mega-merge."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'base')
            dbs.commit()

            call_count = [0]

            def mock_git(pipe_list):  # pylint: disable=too-many-return-statements,unused-argument
                call_count[0] += 1
                if call_count[0] == 1:
                    # First-parent log shows one mega-merge
                    return command.CommandResult(
                        stdout='mega|mega1|A|Merge branch next|'
                               'base second_parent\n')
                if call_count[0] == 2:
                    # detect_sub_merges: rev-parse ^@
                    return command.CommandResult(
                        stdout='base\nsecond_parent\n')
                if call_count[0] == 3:
                    # detect_sub_merges: log --merges (found sub-merges)
                    return command.CommandResult(stdout='sub1\n')
                if call_count[0] == 4:
                    # decompose: rev-parse ^@ for mega-merge
                    return command.CommandResult(
                        stdout='base\nsecond_parent\n')
                if call_count[0] == 5:
                    # decompose: log -1 for mega-merge info
                    return command.CommandResult(
                        stdout='Mega merge|Author\n')
                if call_count[0] == 6:
                    # decompose: mainline commits (empty)
                    return command.CommandResult(stdout='')
                if call_count[0] == 7:
                    # decompose: sub-merge 1 commits
                    return command.CommandResult(
                        stdout='aaa|aaa1|A|Sub commit|base\n')
                return command.CommandResult(stdout='')

            command.TEST_RESULT = mock_git

            info, err = control.get_next_commits(dbs, 'us/next')

            self.assertIsNone(err)
            self.assertTrue(info.merge_found)
            self.assertEqual(len(info.commits), 1)
            self.assertEqual(info.commits[0].chash, 'aaa1')
            # Sub-merge batch: advance_to should be None
            self.assertIsNone(info.advance_to)
            dbs.close()

    def test_all_done_advances_past(self):
        """Test get_next_commits advances past fully-processed mega-merge."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'base')
            dbs.commit()

            # Add all sub-merge commits to database
            source_id = dbs.source_get_id('us/next')
            dbs.commit_add('aaa', source_id, 'Sub commit', 'A',
                           status='applied')
            dbs.commit()

            call_count = [0]

            def mock_git(pipe_list):  # pylint: disable=too-many-return-statements,unused-argument
                call_count[0] += 1
                if call_count[0] == 1:
                    # First-parent log shows mega-merge
                    return command.CommandResult(
                        stdout='mega|mega1|A|Merge branch next|'
                               'base second_parent\n')
                if call_count[0] == 2:
                    # detect_sub_merges: rev-parse ^@
                    return command.CommandResult(
                        stdout='base\nsecond_parent\n')
                if call_count[0] == 3:
                    # detect_sub_merges: log --merges
                    return command.CommandResult(stdout='sub1\n')
                if call_count[0] == 4:
                    # decompose: rev-parse ^@
                    return command.CommandResult(
                        stdout='base\nsecond_parent\n')
                if call_count[0] == 5:
                    # decompose: log -1 for mega-merge info
                    return command.CommandResult(
                        stdout='Mega merge|Author\n')
                if call_count[0] == 6:
                    # decompose: mainline (empty)
                    return command.CommandResult(stdout='')
                if call_count[0] == 7:
                    # decompose: sub-merge 1 (in DB)
                    return command.CommandResult(
                        stdout='aaa|aaa1|A|Sub commit|base\n')
                if call_count[0] == 8:
                    # decompose: remainder (empty)
                    return command.CommandResult(stdout='')
                if call_count[0] == 9:
                    # Remaining commits after mega-merge (empty)
                    return command.CommandResult(stdout='')
                return command.CommandResult(stdout='')

            command.TEST_RESULT = mock_git

            info, err = control.get_next_commits(dbs, 'us/next')

            self.assertIsNone(err)
            self.assertFalse(info.merge_found)
            self.assertEqual(len(info.commits), 0)
            # Should advance past the mega-merge
            self.assertEqual(info.advance_to, 'mega')
            dbs.close()

    def test_normal_merge_returns_advance_to(self):
        """Test get_next_commits returns advance_to for normal merges."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'base')
            dbs.commit()

            call_count = [0]

            def mock_git(pipe_list):  # pylint: disable=unused-argument
                call_count[0] += 1
                if call_count[0] == 1:
                    # First-parent log shows a normal merge
                    return command.CommandResult(
                        stdout='merge1|m1|A|Merge branch feat|'
                               'base side1\n')
                if call_count[0] == 2:
                    # detect_sub_merges: rev-parse ^@
                    return command.CommandResult(
                        stdout='base\nside1\n')
                if call_count[0] == 3:
                    # detect_sub_merges: log --merges (no sub-merges)
                    return command.CommandResult(stdout='')
                if call_count[0] == 4:
                    # Commits for this merge
                    return command.CommandResult(
                        stdout='aaa|aaa1|A|Commit 1|base\n'
                               'merge1|m1|A|Merge branch feat|'
                               'base side1\n')
                return command.CommandResult(stdout='')

            command.TEST_RESULT = mock_git

            info, err = control.get_next_commits(dbs, 'us/next')

            self.assertIsNone(err)
            self.assertTrue(info.merge_found)
            self.assertEqual(len(info.commits), 2)
            # Normal merge: advance_to is the merge hash
            self.assertEqual(info.advance_to, 'merge1')
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


class TestRewind(unittest.TestCase):
    """Tests for rewind command."""

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

    def test_rewind_source_not_found(self):
        """Test rewind with unknown source."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.close()

        database.Database.instances.clear()

        args = argparse.Namespace(cmd='rewind', source='unknown', count=1,
                                  force=True, remote='ci')
        with terminal.capture() as (_, stderr):
            ret = control.do_pickman(args)
        self.assertEqual(ret, 1)
        self.assertIn("Source 'unknown' not found", stderr.getvalue())

    def test_rewind_dry_run(self):
        """Test rewind dry run shows what would happen without executing."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'current_hash')
            source_id = dbs.source_get_id('us/next')
            dbs.commit_add('commit_a', source_id, 'Commit A', 'Author')
            dbs.commit_add('commit_b', source_id, 'Commit B', 'Author')
            dbs.commit()
            dbs.close()

        database.Database.instances.clear()

        def mock_git(pipe_list):
            cmd = pipe_list[0] if pipe_list else []
            if '--merges' in cmd:
                return command.CommandResult(
                    stdout='current_hash|current1|Current merge\n'
                           'prev_hash|prev1234|Previous merge\n')
            if 'rev-list' in cmd:
                return command.CommandResult(
                    stdout='commit_a\ncommit_b\n')
            if 'branch' in cmd and '--list' in cmd:
                return command.CommandResult(stdout='')
            return command.CommandResult(stdout='')

        command.TEST_RESULT = mock_git

        args = argparse.Namespace(cmd='rewind', source='us/next', count=1,
                                  force=False, remote='ci')
        with terminal.capture() as (stdout, _):
            ret = control.do_pickman(args)
        self.assertEqual(ret, 0)
        output = stdout.getvalue()
        self.assertIn('dry run', output)
        self.assertIn('prev1234', output)
        self.assertIn('2', output)  # 2 commits to delete
        self.assertIn('--force', output)

        # Verify database was NOT modified
        database.Database.instances.clear()
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            self.assertEqual(dbs.source_get('us/next'), 'current_hash')
            self.assertIsNotNone(dbs.commit_get('commit_a'))
            self.assertIsNotNone(dbs.commit_get('commit_b'))
            dbs.close()

    def test_rewind_one_merge(self):
        """Test rewinding by one merge with --force."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'current_hash')
            source_id = dbs.source_get_id('us/next')
            # Add some commits that should be deleted
            dbs.commit_add('commit_a', source_id, 'Commit A', 'Author')
            dbs.commit_add('commit_b', source_id, 'Commit B', 'Author')
            dbs.commit()
            dbs.close()

        database.Database.instances.clear()

        def mock_git(pipe_list):
            cmd = pipe_list[0] if pipe_list else []
            if '--merges' in cmd:
                return command.CommandResult(
                    stdout='current_hash|current1|Current merge\n'
                           'prev_hash|prev1234|Previous merge\n')
            if 'rev-list' in cmd:
                return command.CommandResult(
                    stdout='commit_a\ncommit_b\n')
            if 'branch' in cmd and '--list' in cmd:
                return command.CommandResult(stdout='')
            return command.CommandResult(stdout='')

        command.TEST_RESULT = mock_git

        args = argparse.Namespace(cmd='rewind', source='us/next', count=1,
                                  force=True, remote='ci')
        with terminal.capture() as (stdout, _):
            ret = control.do_pickman(args)
        self.assertEqual(ret, 0)
        output = stdout.getvalue()
        self.assertIn('prev1234', output)
        self.assertIn('Deleted 2 commit(s)', output)

        # Verify source was updated
        database.Database.instances.clear()
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            self.assertEqual(dbs.source_get('us/next'), 'prev_hash')
            # Commits should be deleted
            self.assertIsNone(dbs.commit_get('commit_a'))
            self.assertIsNone(dbs.commit_get('commit_b'))
            dbs.close()

    def test_rewind_shows_mr_details(self):
        """Test rewind shows MR numbers, titles and URLs."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'current_hash')
            dbs.commit()
            dbs.close()

        database.Database.instances.clear()

        def mock_git(pipe_list):
            cmd = pipe_list[0] if pipe_list else []
            if '--merges' in cmd:
                return command.CommandResult(
                    stdout='current_hash|current1|Current merge\n'
                           'prev_hash|prev1234|Previous merge\n')
            if 'rev-list' in cmd:
                return command.CommandResult(
                    stdout='abc1234ffffff\ndef5678aaaaaa\n')
            if 'branch' in cmd and '--list' in cmd:
                return command.CommandResult(
                    stdout='  ci/cherry-abc1234f\n'
                           '  ci/cherry-other99\n')
            return command.CommandResult(stdout='')

        command.TEST_RESULT = mock_git

        mock_mrs = [
            gitlab.PickmanMr(
                iid=541, title='[pickman] Some cherry-pick',
                web_url='https://gitlab.com/proj/-/merge_requests/541',
                source_branch='cherry-abc1234f',
                description='desc'),
            gitlab.PickmanMr(
                iid=540, title='[pickman] Unrelated MR',
                web_url='https://gitlab.com/proj/-/merge_requests/540',
                source_branch='cherry-zzz9999',
                description='desc'),
        ]

        args = argparse.Namespace(cmd='rewind', source='us/next', count=1,
                                  force=False, remote='ci')
        with mock.patch.object(gitlab, 'get_open_pickman_mrs',
                               return_value=mock_mrs):
            with terminal.capture() as (stdout, _):
                ret = control.do_pickman(args)
        self.assertEqual(ret, 0)
        output = stdout.getvalue()
        self.assertIn('!541', output)
        self.assertIn('[pickman] Some cherry-pick', output)
        self.assertIn('merge_requests/541', output)
        # Unrelated MR should not appear
        self.assertNotIn('!540', output)
        self.assertIn('MRs to delete', output)

    def test_rewind_shows_branches_when_api_unavailable(self):
        """Test rewind falls back to branch names when GitLab unavailable."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'current_hash')
            dbs.commit()
            dbs.close()

        database.Database.instances.clear()

        def mock_git(pipe_list):
            cmd = pipe_list[0] if pipe_list else []
            if '--merges' in cmd:
                return command.CommandResult(
                    stdout='current_hash|current1|Current merge\n'
                           'prev_hash|prev1234|Previous merge\n')
            if 'rev-list' in cmd:
                return command.CommandResult(
                    stdout='abc1234ffffff\n')
            if 'branch' in cmd and '--list' in cmd:
                return command.CommandResult(
                    stdout='  ci/cherry-abc1234f\n')
            return command.CommandResult(stdout='')

        command.TEST_RESULT = mock_git

        args = argparse.Namespace(cmd='rewind', source='us/next', count=1,
                                  force=False, remote='ci')
        # GitLab API returns None (unavailable)
        with mock.patch.object(gitlab, 'get_open_pickman_mrs',
                               return_value=None):
            with terminal.capture() as (stdout, _):
                ret = control.do_pickman(args)
        self.assertEqual(ret, 0)
        output = stdout.getvalue()
        self.assertIn('cherry-abc1234f', output)
        self.assertIn('Branches to check', output)

    def test_rewind_two_merges(self):
        """Test rewinding by two merges with --force."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'current_hash')
            dbs.commit()
            dbs.close()

        database.Database.instances.clear()

        def mock_git(pipe_list):
            cmd = pipe_list[0] if pipe_list else []
            if '--merges' in cmd:
                return command.CommandResult(
                    stdout='current_hash|current1|Current merge\n'
                           'mid_hash|mid12345|Middle merge\n'
                           'old_hash|old12345|Old merge\n')
            if 'rev-list' in cmd:
                return command.CommandResult(stdout='')
            if 'branch' in cmd and '--list' in cmd:
                return command.CommandResult(stdout='')
            return command.CommandResult(stdout='')

        command.TEST_RESULT = mock_git

        args = argparse.Namespace(cmd='rewind', source='us/next', count=2,
                                  force=True, remote='ci')
        with terminal.capture() as (stdout, _):
            ret = control.do_pickman(args)
        self.assertEqual(ret, 0)
        self.assertIn('old12345', stdout.getvalue())

        # Verify source was updated to old_hash
        database.Database.instances.clear()
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            self.assertEqual(dbs.source_get('us/next'), 'old_hash')
            dbs.close()

    def test_rewind_not_enough_merges(self):
        """Test rewind fails when not enough merges in history."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()
            dbs.source_set('us/next', 'current_hash')
            dbs.commit()
            dbs.close()

        database.Database.instances.clear()

        def mock_git(pipe_list):
            cmd = pipe_list[0] if pipe_list else []
            if '--merges' in cmd:
                # Only one merge (the current position)
                return command.CommandResult(
                    stdout='current_hash|current1|Current merge\n')
            return command.CommandResult(stdout='')

        command.TEST_RESULT = mock_git

        args = argparse.Namespace(cmd='rewind', source='us/next', count=1,
                                  force=True, remote='ci')
        with terminal.capture() as (_, stderr):
            ret = control.do_pickman(args)
        self.assertEqual(ret, 1)
        self.assertIn('Not enough merges', stderr.getvalue())

    def test_parse_rewind(self):
        """Test parsing rewind command."""
        args = pickman.parse_args(['rewind', 'us/next'])
        self.assertEqual(args.cmd, 'rewind')
        self.assertEqual(args.source, 'us/next')
        self.assertEqual(args.count, 1)
        self.assertFalse(args.force)
        self.assertEqual(args.remote, 'ci')

    def test_parse_rewind_with_count(self):
        """Test parsing rewind command with count."""
        args = pickman.parse_args(['rewind', 'us/next', '-c', '3'])
        self.assertEqual(args.cmd, 'rewind')
        self.assertEqual(args.source, 'us/next')
        self.assertEqual(args.count, 3)

    def test_parse_rewind_with_force(self):
        """Test parsing rewind command with --force."""
        args = pickman.parse_args(['rewind', 'us/next', '-c', '2', '-f'])
        self.assertEqual(args.cmd, 'rewind')
        self.assertEqual(args.count, 2)
        self.assertTrue(args.force)


class TestDoPushBranch(unittest.TestCase):
    """Tests for do_push_branch command."""

    def test_push_branch_success(self):
        """Test successful push."""
        tout.init(tout.INFO)
        args = argparse.Namespace(cmd='push-branch', branch='test-branch',
                                  remote='ci', force=False, run_ci=False)
        with mock.patch.object(gitlab, 'push_branch',
                               return_value=True) as mock_push:
            with terminal.capture():
                ret = control.do_push_branch(args, None)
        self.assertEqual(ret, 0)
        mock_push.assert_called_once_with('ci', 'test-branch', False,
                                          skip_ci=True)

    def test_push_branch_force(self):
        """Test force push."""
        tout.init(tout.INFO)
        args = argparse.Namespace(cmd='push-branch', branch='test-branch',
                                  remote='origin', force=True, run_ci=False)
        with mock.patch.object(gitlab, 'push_branch',
                               return_value=True) as mock_push:
            with terminal.capture():
                ret = control.do_push_branch(args, None)
        self.assertEqual(ret, 0)
        mock_push.assert_called_once_with('origin', 'test-branch', True,
                                          skip_ci=True)

    def test_push_branch_failure(self):
        """Test push failure."""
        tout.init(tout.INFO)
        args = argparse.Namespace(cmd='push-branch', branch='test-branch',
                                  remote='ci', force=False, run_ci=False)
        with mock.patch.object(gitlab, 'push_branch',
                               return_value=False):
            with terminal.capture():
                ret = control.do_push_branch(args, None)
        self.assertEqual(ret, 1)


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

        mock_mr = gitlab.PickmanMr(
            iid=123,
            title='[pickman] Test MR',
            web_url='https://gitlab.com/mr/123',
            source_branch='cherry-test',
            description='Test',
        )
        with mock.patch.object(control, 'run_git'):
            with mock.patch.object(gitlab, 'get_open_pickman_mrs',
                                   return_value=[mock_mr]):
                with mock.patch.object(gitlab, 'get_mr_comments',
                                       return_value=[]):
                    args = argparse.Namespace(cmd='review', remote='ci',
                                              target='master')
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
            dbs.source_set('us/next', 'aaa111aaa111aaa111aaa111aaa111aaa111aaa')
            dbs.commit()

            merged_mrs = [gitlab.PickmanMr(
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

            with mock.patch.object(gitlab, 'get_merged_pickman_mrs',
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
            dbs.source_set('us/next',
                              'bbb222bbb222bbb222bbb222bbb222bbb222bbb2')
            dbs.commit()

            merged_mrs = [gitlab.PickmanMr(
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

            with mock.patch.object(gitlab, 'get_merged_pickman_mrs',
                                   return_value=merged_mrs):
                with mock.patch.object(control, 'run_git', mock_git):
                    processed = control.process_merged_mrs('ci', 'us/next', dbs)

            self.assertEqual(processed, 0)
            # Should remain unchanged
            current = dbs.source_get('us/next')
            self.assertEqual(current,
                             'bbb222bbb222bbb222bbb222bbb222bbb222bbb2')

            dbs.close()


class TestCheck(unittest.TestCase):
    """Tests for check command."""

    def setUp(self):
        """Set up test fixtures."""
        self.old_branch = 'old-branch'

    def test_parse_git_stat_output(self):
        """Test parsing git show --stat output."""
        stat_output = """commit abc123def456
Author: Test Author <test@example.com>
Date:   Mon Jan 15 10:30:00 2024 -0600

    Test commit message

 file1.c | 15 +++++++++++++++
 file2.h |  3 +--
 2 files changed, 16 insertions(+), 2 deletions(-)"""

        result = control.parse_git_stat_output(stat_output)
        files, insertions, deletions, file_set = result
        self.assertEqual(files, 2)
        self.assertEqual(insertions, 16)
        self.assertEqual(deletions, 2)
        self.assertEqual(file_set, {'file1.c', 'file2.h'})

    def test_parse_git_stat_output_empty(self):
        """Test parsing empty git show --stat output."""
        stat_output = """commit abc123def456
Author: Test Author <test@example.com>
Date:   Mon Jan 15 10:30:00 2024 -0600

    Empty commit message

 0 files changed"""

        result = control.parse_git_stat_output(stat_output)
        files, insertions, deletions, file_set = result
        self.assertEqual(files, 0)
        self.assertEqual(insertions, 0)
        self.assertEqual(deletions, 0)
        self.assertEqual(file_set, set())

    def test_calc_ratio_identical(self):
        """Test delta ratio calculation for identical commits."""
        orig_stats = control.GitStat(2, 15, 3, {'file1.c', 'file2.h'})
        cherry_stats = control.GitStat(2, 15, 3, {'file1.c', 'file2.h'})

        ratio = control.calc_ratio(orig_stats, cherry_stats)
        self.assertEqual(ratio, 0.0)

    def test_calc_ratio_different_files(self):
        """Test delta ratio calculation for different files."""
        orig_stats = control.GitStat(2, 15, 3, {'file1.c', 'file2.h'})
        cherry_stats = control.GitStat(
            3, 15, 3, {'file1.c', 'file2.h', 'file3.c'})

        ratio = control.calc_ratio(orig_stats, cherry_stats)
        self.assertGreater(ratio, 0.0)
        self.assertLessEqual(ratio, 1.0)

    def test_calc_ratio_different_lines(self):
        """Test delta ratio calculation for different line counts."""
        orig_stats = control.GitStat(2, 15, 3, {'file1.c', 'file2.h'})
        cherry_stats = control.GitStat(2, 30, 6, {'file1.c', 'file2.h'})

        ratio = control.calc_ratio(orig_stats, cherry_stats)
        self.assertGreater(ratio, 0.0)
        self.assertLessEqual(ratio, 1.0)

    def test_get_orig_commit(self):
        """Test finding original commit from cherry-pick message."""
        with mock.patch('pickman.control.run_git') as mock_run_git:
            commit_msg = """Test commit subject

This is the commit body.

(cherry picked from commit abc123def456789)
"""
            mock_run_git.return_value = commit_msg

            orig = control.get_orig_commit('def456abc123')
            self.assertEqual(orig, 'abc123def456789')

    def test_get_orig_commit_not_cherry_pick(self):
        """Test finding original commit for non-cherry-pick."""
        with mock.patch('pickman.control.run_git') as mock_run_git:
            commit_msg = """Test commit subject

This is a normal commit without cherry-pick info.
"""
            mock_run_git.return_value = commit_msg

            orig = control.get_orig_commit('def456abc123')
            self.assertIsNone(orig)

    def test_check_commits_no_commits(self):
        """Test check_commits with empty commit list."""
        commits = []
        results = list(control.check_commits(commits, 10))
        self.assertEqual(len(results), 0)

    def test_check_commits_large_delta(self):
        """Test check_commits finding commits with large deltas."""
        commits = [('abc123', 'abc123d', 'Test commit subject')]

        with mock.patch('pickman.control.run_git') as mock_run_git:
            with mock.patch('pickman.control.get_orig_commit') as \
                    mock_find_orig:
                with mock.patch('pickman.control.parse_git_stat_output') as \
                        mock_parse:
                    with mock.patch('pickman.control.calc_ratio') as mock_calc:
                        # Mock responses
                        mock_run_git.side_effect = [
                            ['def456'],  # parents (single parent)
                            'orig_stat_output',  # original commit stats
                            'cherry_stat_output'  # cherry-pick commit stats
                        ]
                        mock_find_orig.return_value = 'def456original'
                        mock_parse.side_effect = [
                            control.GitStat(2, 15, 3, {'file1.c', 'file2.h'}),
                            control.GitStat(3, 30, 6,
                                            {'file1.c', 'file2.h', 'file3.c'})
                        ]
                        mock_calc.return_value = 0.5  # 50% delta

                        results = list(control.check_commits(commits, 10))
                        self.assertEqual(len(results), 1)

                        result = results[0]
                        self.assertEqual(result.chash, 'abc123')
                        self.assertEqual(result.orig_hash, 'def456original')
                        self.assertEqual(result.delta_ratio, 0.5)
                        self.assertIsNone(result.reason)

    def test_check_commits_normal_commit(self):
        """Test check_commits processing a normal commit."""
        commits = [('abc123', 'abc123d', 'Test commit subject')]

        with mock.patch('pickman.control.run_git') as mock_run_git:
            with mock.patch('pickman.control.get_orig_commit') as \
                    mock_find_orig:
                with mock.patch('pickman.control.parse_git_stat_output') as \
                        mock_parse:
                    with mock.patch('pickman.control.calc_ratio') as mock_calc:
                        # Mock responses
                        mock_run_git.side_effect = [
                            ['def456'],  # parents (single parent)
                            'orig_stat_output',  # original commit stats
                            'cherry_stat_output'  # cherry-pick commit stats
                        ]
                        mock_find_orig.return_value = 'def456original'
                        mock_parse.side_effect = [
                            control.GitStat(2, 15, 3, {'file1.c', 'file2.h'}),
                            control.GitStat(3, 30, 6,
                                            {'file1.c', 'file2.h', 'file3.c'})
                        ]
                        mock_calc.return_value = 0.1  # 10% delta (low)

                        results = list(control.check_commits(commits, 10))
                        self.assertEqual(len(results), 1)

                        result = results[0]
                        self.assertEqual(result.chash, 'abc123')
                        self.assertEqual(result.orig_hash, 'def456original')
                        self.assertEqual(result.subject, 'Test commit subject')
                        self.assertEqual(result.delta_ratio, 0.1)
                        self.assertIsNone(result.reason)

    def test_check_commits_merge_skip(self):
        """Test check_commits skips merge commits."""
        commits = [('abc123', 'abc123d', 'Merge branch feature')]

        with mock.patch('pickman.control.run_git') as mock_run_git:
            # Mock multiple parents (merge commit)
            mock_run_git.return_value = ['parent1', 'parent2']

            results = list(control.check_commits(commits, 10))
            self.assertEqual(len(results), 1)

            result = results[0]
            self.assertEqual(result.reason, 'merge_commit')

    def test_check_commits_small_commit_skip(self):
        """Test check_commits skips small commits."""
        commits = [('abc123', 'abc123d', 'Fix typo')]

        with mock.patch('pickman.control.run_git') as mock_run_git:
            with mock.patch('pickman.control.get_orig_commit') as \
                    mock_find_orig:
                with mock.patch('pickman.control.parse_git_stat_output') as \
                        mock_parse:
                    # Mock responses for small commit
                    mock_run_git.side_effect = [
                        ['def456'],  # single parent
                        'orig_stat_output',
                        'cherry_stat_output'
                    ]
                    mock_find_orig.return_value = 'def456original'
                    mock_parse.side_effect = [
                        # 3 total lines (< 10)
                        control.GitStat(1, 2, 1, {'file1.c'}),
                        control.GitStat(1, 2, 1, {'file1.c'})
                    ]

                    results = list(control.check_commits(commits, 10))
                    self.assertEqual(len(results), 1)

                    result = results[0]
                    self.assertEqual(result.reason, 'small_commit_3_lines')

    @mock.patch('pickman.control.command')
    @mock.patch('pickman.control.run_git')
    @mock.patch('tempfile.NamedTemporaryFile')
    @mock.patch('os.unlink')
    def test_show_commit_diff_with_colour(self, unused_unlink, mock_temp,
                                         mock_run_git, mock_command):
        """Test show_commit_diff with colour enabled."""
        # Mock temporary files
        mock_temp.side_effect = [
            mock.mock_open()(),  # orig file
            mock.mock_open()()   # cherry file
        ]
        mock_temp.return_value.__enter__.return_value.name = '/tmp/test.patch'

        # Mock git show outputs
        mock_run_git.side_effect = [
            'orig patch content',
            'cherry patch content'
        ]

        # Mock diff command output
        mock_command.output.return_value = 'diff output'

        # Test data
        res = control.CheckResult(
            chash='abc123',
            orig_hash='def456',
            subject='Test',
            delta_ratio=0.5,
            orig_stats=None,
            cherry_stats=None,
            reason=None
        )

        with terminal.capture():
            control.show_commit_diff(res, no_colour=False)

        # Verify git show was called for both commits
        expected_calls = [
            mock.call(['show', '--no-ext-diff', 'def456']),
            mock.call(['show', '--no-ext-diff', 'abc123'])
        ]
        mock_run_git.assert_has_calls(expected_calls)

        # Verify diff was called with colour
        mock_command.output.assert_called_once()
        args, kwargs = mock_command.output.call_args
        self.assertIn('--color=always', args)
        self.assertEqual(kwargs.get('raise_on_error'), False)

    @mock.patch('pickman.control.command')
    @mock.patch('pickman.control.run_git')
    @mock.patch('tempfile.NamedTemporaryFile')
    @mock.patch('os.unlink')
    def test_show_commit_diff_no_colour(self, unused_unlink, mock_temp,
                                       mock_run_git, mock_command):
        """Test show_commit_diff with colour disabled."""
        # Mock temporary files
        mock_temp.side_effect = [
            mock.mock_open()(),  # orig file
            mock.mock_open()()   # cherry file
        ]
        mock_temp.return_value.__enter__.return_value.name = '/tmp/test.patch'

        # Mock git show outputs
        mock_run_git.side_effect = [
            'orig patch content',
            'cherry patch content'
        ]

        # Mock diff command output
        mock_command.output.return_value = 'diff output'

        # Test data
        res = control.CheckResult(
            chash='abc123',
            orig_hash='def456',
            subject='Test',
            delta_ratio=0.5,
            orig_stats=None,
            cherry_stats=None,
            reason=None
        )

        with terminal.capture():
            control.show_commit_diff(res, no_colour=True)

        # Verify diff was called without colour
        mock_command.output.assert_called_once()
        args, kwargs = mock_command.output.call_args
        self.assertNotIn('--color=always', args)
        self.assertEqual(kwargs.get('raise_on_error'), False)


class TestCheckAlreadyApplied(unittest.TestCase):
    """Tests for the check_already_applied function."""

    def setUp(self):
        """Set up test data."""
        self.commits = [
            control.CommitInfo('abc123def456', 'abc123d', 'Add new feature',
                               'Author <email>'),
            control.CommitInfo('def456abc123', 'def456a', 'Fix bug',
                               'Author <email>')
        ]
        self.quoted_commit = [
            control.CommitInfo('abc123def456', 'abc123d',
                               'Add "quoted" feature', 'Author <email>')
        ]
        self.single_commit = [
            control.CommitInfo('abc123def456', 'abc123d', 'Add new feature',
                               'Author <email>')
        ]

    @mock.patch('pickman.control.run_git')
    @mock.patch('pickman.control.tout')
    def test_check_already_applied_none_applied(self, mock_tout, mock_run_git):
        """Test check_already_applied when no commits are already applied."""
        # Mock git log returning empty (no matches)
        mock_run_git.return_value = ''

        new_commits, applied = control.check_already_applied(self.commits)

        self.assertEqual(len(new_commits), 2)
        self.assertEqual(len(applied), 0)
        self.assertEqual(new_commits, self.commits)
        mock_tout.info.assert_not_called()

    @mock.patch('pickman.control.run_git')
    @mock.patch('pickman.control.tout')
    def test_check_already_applied_some_applied(self, mock_tout, mock_run_git):
        """Test check_already_applied when some commits are already applied."""
        # First commit returns a match, second doesn't
        mock_run_git.side_effect = ['xyz789 Add new feature', '']

        new_commits, applied = control.check_already_applied(self.commits)

        self.assertEqual(len(new_commits), 1)
        self.assertEqual(len(applied), 1)
        self.assertEqual(new_commits[0].hash, 'def456abc123')
        self.assertEqual(applied[0].hash, 'abc123def456')
        mock_tout.info.assert_called_once()

    @mock.patch('pickman.control.run_git')
    @mock.patch('pickman.control.tout')
    def test_check_already_applied_all_applied(self, mock_tout, mock_run_git):
        """Test check_already_applied when all commits are already applied."""
        # Both commits return matches
        mock_run_git.side_effect = ['xyz789 Add new feature', 'uvw123 Fix bug']

        new_commits, applied = control.check_already_applied(self.commits)

        self.assertEqual(len(new_commits), 0)
        self.assertEqual(len(applied), 2)
        self.assertEqual(applied, self.commits)
        self.assertEqual(mock_tout.info.call_count, 2)

    @mock.patch('pickman.control.run_git')
    @mock.patch('pickman.control.tout')
    def test_check_already_applied_with_quotes_in_subject(
            self, unused_mock_tout, mock_run_git):
        """Test check_already_applied handles quotes in commit subjects."""
        mock_run_git.return_value = ''

        new_commits, applied = control.check_already_applied(self.quoted_commit)

        # Verify git was called with escaped quotes
        mock_run_git.assert_called_once_with([
            'log', '--oneline', 'ci/master',
            '--grep=Add \\"quoted\\" feature', '-1'
        ])
        self.assertEqual(len(new_commits), 1)
        self.assertEqual(len(applied), 0)

    @mock.patch('pickman.control.run_git')
    @mock.patch('pickman.control.tout')
    def test_check_already_applied_git_error(self, unused_mock_tout,
                                             mock_run_git):
        """Test check_already_applied handles git errors gracefully."""
        # Mock git command raising an exception
        mock_run_git.side_effect = Exception('Git error')

        new_commits, applied = control.check_already_applied(self.single_commit)

        # Should treat as not applied when git fails
        self.assertEqual(len(new_commits), 1)
        self.assertEqual(len(applied), 0)
        self.assertEqual(new_commits, self.single_commit)


class TestGetCommitsForPick(unittest.TestCase):
    """Tests for get_commits_for_pick function."""

    @mock.patch('pickman.control.run_git')
    def test_commit_range(self, mock_run_git):
        """Test parsing a commit range."""
        mock_run_git.return_value = (
            'aaa111|aaa111a|Author1|First commit\n'
            'bbb222|bbb222b|Author2|Second commit'
        )

        commits, err = control.get_commits_for_pick('abc123..def456')

        self.assertIsNone(err)
        self.assertEqual(len(commits), 2)
        self.assertEqual(commits[0].hash, 'aaa111')
        self.assertEqual(commits[0].chash, 'aaa111a')
        self.assertEqual(commits[0].subject, 'First commit')
        self.assertEqual(commits[1].hash, 'bbb222')
        mock_run_git.assert_called_with([
            'log', '--reverse', '--format=%H|%h|%an|%s', 'abc123..def456'
        ])

    @mock.patch('pickman.control.run_git')
    def test_commit_range_empty(self, mock_run_git):
        """Test empty commit range returns error."""
        mock_run_git.return_value = ''

        commits, err = control.get_commits_for_pick('abc123..abc123')

        self.assertEqual(commits, [])
        self.assertIn('No commits found', err)

    @mock.patch('pickman.control.run_git')
    def test_commit_range_invalid(self, mock_run_git):
        """Test invalid commit range returns error."""
        mock_run_git.side_effect = Exception('bad revision')

        commits, err = control.get_commits_for_pick('invalid..range')

        self.assertIsNone(commits)
        self.assertIn('Invalid commit range', err)

    @mock.patch('pickman.control.run_git')
    def test_single_commit_non_merge(self, mock_run_git):
        """Test single non-merge commit returns just that commit."""
        def git_handler(args):
            if 'rev-parse' in args:
                return 'parent123'  # Single parent = not a merge
            return 'abc123full|abc123|Author|Single commit'

        mock_run_git.side_effect = git_handler

        commits, err = control.get_commits_for_pick('abc123')

        self.assertIsNone(err)
        self.assertEqual(len(commits), 1)
        self.assertEqual(commits[0].hash, 'abc123full')
        self.assertEqual(commits[0].subject, 'Single commit')

    @mock.patch('pickman.control.run_git')
    def test_merge_commit(self, mock_run_git):
        """Test merge commit returns all child commits."""
        def git_handler(args):
            if 'rev-parse' in args:
                # Two parents = merge commit
                return 'parent1\nparent2'
            if '^parent1' in args:
                return (
                    'ccc333|ccc333c|Author1|Child commit 1\n'
                    'ddd444|ddd444d|Author2|Child commit 2'
                )
            return ''

        mock_run_git.side_effect = git_handler

        commits, err = control.get_commits_for_pick('merge123')

        self.assertIsNone(err)
        self.assertEqual(len(commits), 2)
        self.assertEqual(commits[0].hash, 'ccc333')
        self.assertEqual(commits[1].hash, 'ddd444')

    @mock.patch('pickman.control.run_git')
    def test_merge_commit_empty(self, mock_run_git):
        """Test merge commit with no children returns error."""
        def git_handler(args):
            if 'rev-parse' in args:
                return 'parent1\nparent2'
            return ''

        mock_run_git.side_effect = git_handler

        commits, err = control.get_commits_for_pick('merge123')

        self.assertEqual(commits, [])
        self.assertIn('No commits found in merge', err)

    @mock.patch('pickman.control.run_git')
    def test_invalid_single_commit(self, mock_run_git):
        """Test invalid single commit returns error."""
        mock_run_git.side_effect = Exception('unknown revision')

        commits, err = control.get_commits_for_pick('badcommit')

        self.assertIsNone(commits)
        self.assertIn('Invalid commit', err)

    @mock.patch('pickman.control.run_git')
    def test_subject_with_separator(self, mock_run_git):
        """Test commit subject containing pipe character."""
        mock_run_git.return_value = 'aaa111|aaa111a|Author|Subject|with|pipes'

        commits, err = control.get_commits_for_pick('abc..def')

        self.assertIsNone(err)
        self.assertEqual(commits[0].subject, 'Subject|with|pipes')


class TestParsePick(unittest.TestCase):
    """Tests for parsing pick command arguments."""

    def test_parse_pick_basic(self):
        """Test parsing basic pick command."""
        args = pickman.parse_args(['pick', 'abc123..def456'])
        self.assertEqual(args.cmd, 'pick')
        self.assertEqual(args.commits, 'abc123..def456')
        self.assertIsNone(args.branch)
        self.assertFalse(args.push)

    def test_parse_pick_with_branch(self):
        """Test parsing pick command with branch."""
        args = pickman.parse_args(['pick', 'abc123', '-b', 'my-branch'])
        self.assertEqual(args.commits, 'abc123')
        self.assertEqual(args.branch, 'my-branch')

    def test_parse_pick_with_push(self):
        """Test parsing pick command with push options."""
        args = pickman.parse_args([
            'pick', 'abc123..def456', '-p', '-r', 'origin', '-t', 'main'
        ])
        self.assertTrue(args.push)
        self.assertEqual(args.remote, 'origin')
        self.assertEqual(args.target, 'main')


class TestDoPick(unittest.TestCase):
    """Tests for do_pick function."""

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

    def test_pick_error(self):
        """Test do_pick with invalid commit spec."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()

            args = argparse.Namespace(commits='invalid..range', branch=None,
                                      push=False)

            with mock.patch.object(control, 'get_commits_for_pick',
                                   return_value=(None, 'Invalid commit')):
                ret = control.do_pick(args, dbs)

            self.assertEqual(ret, 1)
            dbs.close()

    def test_pick_no_commits(self):
        """Test do_pick with empty commit range."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()

            args = argparse.Namespace(commits='abc..abc', branch=None,
                                      push=False)

            with mock.patch.object(control, 'get_commits_for_pick',
                                   return_value=([], None)):
                ret = control.do_pick(args, dbs)

            self.assertEqual(ret, 0)
            dbs.close()

    def test_pick_success(self):
        """Test do_pick with successful cherry-pick."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()

            commits = [control.CommitInfo('aaa111', 'aaa111a', 'Test commit',
                                          'Author')]
            args = argparse.Namespace(commits='abc..def', branch=None,
                                      push=False)

            with mock.patch.object(control, 'get_commits_for_pick',
                                   return_value=(commits, None)):
                with mock.patch.object(control, 'run_git',
                                       return_value='main'):
                    with mock.patch.object(control.agent, 'cherry_pick_commits',
                                           return_value=(True, 'log')):
                        ret = control.do_pick(args, dbs)

            self.assertEqual(ret, 0)
            dbs.close()

    def test_pick_with_custom_branch(self):
        """Test do_pick with custom branch name."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()

            commits = [control.CommitInfo('bbb222', 'bbb222b', 'Test commit',
                                          'Author')]
            args = argparse.Namespace(commits='abc..def', branch='my-branch',
                                      push=False)

            with mock.patch.object(control, 'get_commits_for_pick',
                                   return_value=(commits, None)):
                with mock.patch.object(control, 'run_git',
                                       return_value='main'):
                    with mock.patch.object(
                            control.agent, 'cherry_pick_commits',
                            return_value=(True, 'log')) as mock_agent:
                        ret = control.do_pick(args, dbs)

            # Verify agent was called with correct branch name
            call_args = mock_agent.call_args
            self.assertEqual(call_args[0][2], 'my-branch')
            self.assertEqual(ret, 0)
            dbs.close()

    def test_pick_with_push(self):
        """Test do_pick with push enabled."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()

            commits = [control.CommitInfo('ccc333', 'ccc333c', 'Test commit',
                                          'Author')]
            args = argparse.Namespace(commits='abc..def', branch=None,
                                      push=True, remote='origin', target='main')

            with mock.patch.object(control, 'get_commits_for_pick',
                                   return_value=(commits, None)):
                with mock.patch.object(control, 'run_git',
                                       return_value='main'):
                    with mock.patch.object(control.agent, 'cherry_pick_commits',
                                           return_value=(True, 'log')):
                        with mock.patch.object(gitlab, 'push_and_create_mr',
                                               return_value='https://mr/url'):
                            ret = control.do_pick(args, dbs)

            self.assertEqual(ret, 0)
            dbs.close()

    def test_pick_agent_fails(self):
        """Test do_pick when agent fails."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()

            commits = [control.CommitInfo('ddd444', 'ddd444d', 'Test commit',
                                          'Author')]
            args = argparse.Namespace(commits='abc..def', branch=None,
                                      push=False)

            with mock.patch.object(control, 'get_commits_for_pick',
                                   return_value=(commits, None)):
                with mock.patch.object(control, 'run_git',
                                       return_value='main'):
                    with mock.patch.object(control.agent, 'cherry_pick_commits',
                                           return_value=(False, 'error log')):
                        ret = control.do_pick(args, dbs)

            self.assertEqual(ret, 1)
            dbs.close()

    def test_pick_agent_aborts(self):
        """Test do_pick when agent aborts and branch doesn't exist."""
        with terminal.capture():
            dbs = database.Database(self.db_path)
            dbs.start()

            commits = [control.CommitInfo('eee555', 'eee555e', 'Test commit',
                                          'Author')]
            args = argparse.Namespace(commits='abc..def', branch=None,
                                      push=False)

            def run_git_handler(args):
                if 'branch' in args and '--list' in args:
                    return ''  # Branch doesn't exist
                return 'main'

            with mock.patch.object(control, 'get_commits_for_pick',
                                   return_value=(commits, None)):
                with mock.patch.object(control, 'run_git',
                                       side_effect=run_git_handler):
                    with mock.patch.object(control.agent, 'cherry_pick_commits',
                                           return_value=(True, 'aborted')):
                        ret = control.do_pick(args, dbs)

            self.assertEqual(ret, 1)
            dbs.close()


if __name__ == '__main__':
    unittest.main()

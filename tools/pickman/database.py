# SPDX-License-Identifier: GPL-2.0+
#
# Copyright 2025 Canonical Ltd.
# Written by Simon Glass <simon.glass@canonical.com>
#
"""Database for pickman - tracks cherry-pick state.

This uses sqlite3 with a local file (.pickman.db).

To adjust the schema, increment LATEST, create a _migrate_to_v<x>() function
and add code in migrate_to() to call it.
"""

import os
import sqlite3

from u_boot_pylib import tools
from u_boot_pylib import tout

# Schema version (version 0 means there is no database yet)
LATEST = 1

# Default database filename
DB_FNAME = '.pickman.db'


class Database:
    """Database of cherry-pick state used by pickman"""

    # dict of databases:
    #   key: filename
    #   value: Database object
    instances = {}

    def __init__(self, db_path):
        """Set up a new database object

        Args:
            db_path (str): Path to the database
        """
        if db_path in Database.instances:
            raise ValueError(f"There is already a database for '{db_path}'")
        self.con = None
        self.cur = None
        self.db_path = db_path
        self.is_open = False
        Database.instances[db_path] = self

    @staticmethod
    def get_instance(db_path):
        """Get the database instance for a path

        Args:
            db_path (str): Path to the database

        Return:
            tuple:
                Database: Database instance, created if necessary
                bool: True if newly created
        """
        dbs = Database.instances.get(db_path)
        if dbs:
            return dbs, False
        return Database(db_path), True

    def start(self):
        """Open the database ready for use, migrate to latest schema"""
        self.open_it()
        self.migrate_to(LATEST)

    def open_it(self):
        """Open the database, creating it if necessary"""
        if self.is_open:
            raise ValueError('Already open')
        if not os.path.exists(self.db_path):
            tout.warning(f'Creating new database {self.db_path}')
        self.con = sqlite3.connect(self.db_path)
        self.cur = self.con.cursor()
        self.is_open = True
        Database.instances[self.db_path] = self

    def close(self):
        """Close the database"""
        if not self.is_open:
            raise ValueError('Already closed')
        self.con.close()
        self.cur = None
        self.con = None
        self.is_open = False
        Database.instances.pop(self.db_path, None)

    def _create_v1(self):
        """Create a database with the v1 schema"""
        # Table for tracking source branches and their last cherry-picked commit
        self.cur.execute(
            'CREATE TABLE source ('
            'id INTEGER PRIMARY KEY AUTOINCREMENT, '
            'name TEXT UNIQUE, '
            'last_commit TEXT)')

        # Schema version table
        self.cur.execute('CREATE TABLE schema_version (version INTEGER)')

    def migrate_to(self, dest_version):
        """Migrate the database to the selected version

        Args:
            dest_version (int): Version to migrate to
        """
        while True:
            version = self.get_schema_version()
            if version >= dest_version:
                break

            self.close()
            tools.write_file(f'{self.db_path}old.v{version}',
                             tools.read_file(self.db_path))

            version += 1
            tout.info(f'Update database to v{version}')
            self.open_it()
            if version == 1:
                self._create_v1()

            self.cur.execute('DELETE FROM schema_version')
            self.cur.execute(
                'INSERT INTO schema_version (version) VALUES (?)',
                (version,))
            self.commit()

    def get_schema_version(self):
        """Get the version of the database's schema

        Return:
            int: Database version, 0 means there is no data
        """
        try:
            self.cur.execute('SELECT version FROM schema_version')
            return self.cur.fetchone()[0]
        except sqlite3.OperationalError:
            return 0

    def execute(self, query, parameters=()):
        """Execute a database query

        Args:
            query (str): Query string
            parameters (tuple): Parameters to pass

        Return:
            Cursor result
        """
        return self.cur.execute(query, parameters)

    def commit(self):
        """Commit changes to the database"""
        self.con.commit()

    def rollback(self):
        """Roll back changes to the database"""
        self.con.rollback()

    # source functions

    def source_get(self, name):
        """Get the last cherry-picked commit for a source branch

        Args:
            name (str): Source branch name

        Return:
            str: Commit hash, or None if not found
        """
        res = self.execute(
            'SELECT last_commit FROM source WHERE name = ?', (name,))
        rec = res.fetchone()
        if rec:
            return rec[0]
        return None

    def source_set(self, name, commit):
        """Set the last cherry-picked commit for a source branch

        Args:
            name (str): Source branch name
            commit (str): Commit hash
        """
        self.execute(
            'UPDATE source SET last_commit = ? WHERE name = ?', (commit, name))
        if self.cur.rowcount == 0:
            self.execute(
                'INSERT INTO source (name, last_commit) VALUES (?, ?)',
                (name, commit))

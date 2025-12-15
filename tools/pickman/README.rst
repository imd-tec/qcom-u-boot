.. SPDX-License-Identifier: GPL-2.0+
..
.. Copyright 2025 Canonical Ltd.
.. Written by Simon Glass <simon.glass@canonical.com>

Pickman - Cherry-pick Manager
=============================

Pickman is a tool to help manage cherry-picking commits between branches.

Usage
-----

To add a source branch to track::

    ./tools/pickman/pickman add-source us/next

This finds the merge-base commit between the master branch (ci/master) and the
source branch, and stores it in the database as the starting point for
cherry-picking.

To compare branches and show commits that need to be cherry-picked::

    ./tools/pickman/pickman compare

This shows:

- The number of commits in the source branch (us/next) that are not in the
  master branch (ci/master)
- The last common commit between the two branches

Database
--------

Pickman uses a sqlite3 database (``.pickman.db``) to track state:

- **source table**: Tracks source branches and the last commit that was
  cherry-picked into master

Configuration
-------------

The branches to compare are configured as constants in control.py:

- ``BRANCH_MASTER``: The main branch to compare against (default: ci/master)
- ``BRANCH_SOURCE``: The source branch with commits to cherry-pick
  (default: us/next)

Testing
-------

To run the functional tests::

    ./tools/pickman/pickman test

Or using pytest::

    python3 -m pytest tools/pickman/ftest.py -v

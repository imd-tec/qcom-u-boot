.. SPDX-License-Identifier: GPL-2.0+
..
.. Copyright 2025 Canonical Ltd.
.. Written by Simon Glass <simon.glass@canonical.com>

Pickman - Cherry-pick Manager
=============================

Pickman is a tool to help manage cherry-picking commits between branches.

Usage
-----

To compare branches and show commits that need to be cherry-picked::

    ./tools/pickman/pickman

This shows:

- The number of commits in the source branch (us/next) that are not in the
  master branch (ci/master)
- The last common commit between the two branches

Configuration
-------------

The branches to compare are configured as constants at the top of the script:

- ``BRANCH_MASTER``: The main branch to compare against (default: ci/master)
- ``BRANCH_SOURCE``: The source branch with commits to cherry-pick
  (default: us/next)

Testing
-------

To run the functional tests::

    cd tools/pickman
    python3 -m pytest ftest.py -v

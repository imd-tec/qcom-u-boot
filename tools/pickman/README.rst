.. SPDX-License-Identifier: GPL-2.0+
..
.. Copyright 2025 Canonical Ltd.
.. Written by Simon Glass <simon.glass@canonical.com>

Pickman - Cherry-pick Manager
=============================

Pickman is a tool to help manage cherry-picking commits between branches.

Workflow
--------

The typical workflow for using pickman is:

1. **Setup**: Add a source branch to track with ``add-source``. This records the
   starting point (merge-base) for cherry-picking.

2. **Cherry-pick**: Run ``apply -p`` to cherry-pick the next set of commits (up
   to the next merge commit) and create a GitLab MR. A Claude agent handles the
   cherry-picks automatically, resolving simple conflicts.

3. **Review**: Once the MR is reviewed and merged, run ``commit-source`` to
   update the database with the last processed commit.

4. **Repeat**: Go back to step 2 until all commits are cherry-picked.

For fully automated workflows, use ``poll`` which runs ``step`` in a loop. The
``step`` command handles the complete cycle automatically:

- Detects merged MRs and updates the database (no manual ``commit-source``)
- Processes review comments on open MRs using Claude agent
- Creates new MRs when none are pending

This allows hands-off operation: just run ``poll`` and approve/merge MRs in
GitLab as they come in.

Usage
-----

To add a source branch to track::

    ./tools/pickman/pickman add-source us/next

This finds the merge-base commit between the master branch (ci/master) and the
source branch, and stores it in the database as the starting point for
cherry-picking.

To list all tracked source branches::

    ./tools/pickman/pickman list-sources

To compare branches and show commits that need to be cherry-picked::

    ./tools/pickman/pickman compare

This shows:

- The number of commits in the source branch (us/next) that are not in the
  master branch (ci/master)
- The last common commit between the two branches

To show the next set of commits to cherry-pick from a source branch::

    ./tools/pickman/pickman next-set us/next

This finds commits between the last cherry-picked commit and the next merge
commit in the source branch. It stops at the merge commit since that typically
represents a logical grouping of commits (e.g., a pull request).

To show the next N merges that will be applied::

    ./tools/pickman/pickman next-merges us/next

This shows the upcoming merge commits on the first-parent chain, useful for
seeing what's coming up. Use ``-c`` to specify the count (default 10).

To apply the next set of commits using a Claude agent::

    ./tools/pickman/pickman apply us/next

This uses the Claude Agent SDK to automate the cherry-pick process. The agent
will:

- Run git status to check the repository state
- Cherry-pick each commit in order
- Handle simple conflicts automatically
- Report status after completion

To push the branch and create a GitLab merge request::

    ./tools/pickman/pickman apply us/next -p

Options for the apply command:

- ``-b, --branch``: Branch name to create (default: cherry-<hash>)
- ``-p, --push``: Push branch and create GitLab MR
- ``-r, --remote``: Git remote for push (default: ci)
- ``-t, --target``: Target branch for MR (default: master)

On successful cherry-pick, an entry is appended to ``.pickman-history`` with:

- Date and source branch
- Branch name and list of commits
- The agent's conversation log

This file is committed automatically and included in the MR description when
using ``-p``.

After successfully applying commits, update the database to record progress::

    ./tools/pickman/pickman commit-source us/next <commit-hash>

This updates the last cherry-picked commit for the source branch, so subsequent
``next-set`` and ``apply`` commands will start from the new position.

To check open MRs for comments and address them::

    ./tools/pickman/pickman review

This lists open pickman MRs (those with ``[pickman]`` in the title), checks each
for unresolved comments, and uses a Claude agent to address them. The agent will
make code changes based on the feedback and push an updated branch.

Options for the review command:

- ``-r, --remote``: Git remote (default: ci)

To automatically create an MR if none is pending::

    ./tools/pickman/pickman step us/next

This command performs the following:

1. Checks for merged pickman MRs and updates the database with the last
   cherry-picked commit from each merged MR
2. Checks for open pickman MRs (those with ``[pickman]`` in the title)
3. If open MRs exist, processes any review comments using Claude agent
4. If no open MRs exist, runs ``apply`` with ``--push`` to create a new one

This is useful for automated workflows where only one MR should be active at a
time. The automatic database update on merge means you don't need to manually
run ``commit-source`` after each MR is merged, and review comments are handled
automatically.

Options for the step command:

- ``-r, --remote``: Git remote for push (default: ci)
- ``-t, --target``: Target branch for MR (default: master)

To run step continuously in a polling loop::

    ./tools/pickman/pickman poll us/next

This runs the ``step`` command repeatedly with a configurable interval,
creating new MRs as previous ones are merged. Press Ctrl+C to stop.

Options for the poll command:

- ``-i, --interval``: Interval between steps in seconds (default: 300)
- ``-r, --remote``: Git remote for push (default: ci)
- ``-t, --target``: Target branch for MR (default: master)

Requirements
------------

To use the ``apply`` command, install the Claude Agent SDK::

    pip install claude-agent-sdk

You will also need an Anthropic API key set in the ``ANTHROPIC_API_KEY``
environment variable.

To use the ``-p`` (push) option for GitLab integration, install python-gitlab::

    pip install python-gitlab

You will also need a GitLab API token set in the ``GITLAB_TOKEN`` environment
variable. See `GitLab Personal Access Tokens`_ for instructions on creating one.
The token needs ``api`` scope.

.. _GitLab Personal Access Tokens:
   https://docs.gitlab.com/ee/user/profile/personal_access_tokens.html

Database
--------

Pickman uses a sqlite3 database (``.pickman.db``) to track state. The schema
version is stored in the ``schema_version`` table and migrations are applied
automatically when the database is opened.

Tables
~~~~~~

**source**
    Tracks source branches and their cherry-pick progress.

    - ``id``: Primary key
    - ``name``: Branch name (e.g., 'us/next')
    - ``last_commit``: Hash of the last commit cherry-picked from this branch

**pcommit**
    Tracks individual commits being cherry-picked.

    - ``id``: Primary key
    - ``chash``: Original commit hash
    - ``source_id``: Foreign key to source table
    - ``mergereq_id``: Foreign key to mergereq table (optional)
    - ``subject``: Commit subject line
    - ``author``: Commit author
    - ``status``: One of 'pending', 'applied', 'skipped', 'conflict'
    - ``cherry_hash``: Hash of the cherry-picked commit (if applied)

**mergereq**
    Tracks merge requests created for cherry-picked commits.

    - ``id``: Primary key
    - ``source_id``: Foreign key to source table
    - ``branch_name``: Git branch name for this MR
    - ``mr_id``: GitLab merge request ID
    - ``status``: One of 'open', 'merged', 'closed'
    - ``url``: URL to the merge request
    - ``created_at``: Timestamp when the MR was created

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

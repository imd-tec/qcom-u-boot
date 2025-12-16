# SPDX-License-Identifier: GPL-2.0+
#
# Copyright 2025 Canonical Ltd.
# Written by Simon Glass <simon.glass@canonical.com>
#
"""GitLab integration for pickman - push branches and create merge requests."""

import os
import re
import sys

# Allow 'from pickman import xxx' to work via symlink
our_path = os.path.dirname(os.path.realpath(__file__))
sys.path.insert(0, os.path.join(our_path, '..'))

# pylint: disable=wrong-import-position,import-error
from u_boot_pylib import command
from u_boot_pylib import tout

# Check if gitlab module is available
try:
    import gitlab
    AVAILABLE = True
except ImportError:
    AVAILABLE = False


def check_available():
    """Check if the python-gitlab module is available

    Returns:
        bool: True if available, False otherwise
    """
    if not AVAILABLE:
        tout.error('python-gitlab module not available')
        tout.error('Install with: pip install python-gitlab')
        return False
    return True


def get_token():
    """Get GitLab API token from environment

    Returns:
        str: Token or None if not set
    """
    return os.environ.get('GITLAB_TOKEN') or os.environ.get('GITLAB_API_TOKEN')


def get_remote_url(remote):
    """Get the URL for a git remote

    Args:
        remote (str): Remote name

    Returns:
        str: Remote URL
    """
    return command.output('git', 'remote', 'get-url', remote).strip()


def parse_url(url):
    """Parse a GitLab URL to extract host and project path

    Args:
        url (str): Git remote URL (ssh or https)

    Returns:
        tuple: (host, proj_path) or (None, None) if not parseable

    Examples:
        - git@gitlab.com:group/project.git -> ('gitlab.com', 'group/project')
        - https://gitlab.com/group/project.git -> ('gitlab.com', 'group/project')
    """
    # SSH format: git@gitlab.com:group/project.git
    ssh_match = re.match(r'git@([^:]+):(.+?)(?:\.git)?$', url)
    if ssh_match:
        return ssh_match.group(1), ssh_match.group(2)

    # HTTPS format: https://gitlab.com/group/project.git
    https_match = re.match(r'https?://([^/]+)/(.+?)(?:\.git)?$', url)
    if https_match:
        return https_match.group(1), https_match.group(2)

    return None, None


def push_branch(remote, branch, force=False):
    """Push a branch to a remote

    Args:
        remote (str): Remote name
        branch (str): Branch name
        force (bool): Force push (overwrite remote branch)

    Returns:
        bool: True on success
    """
    try:
        # Use ci.skip to avoid duplicate pipeline (MR pipeline will still run)
        # Set SJG_LAB=1 CI variable for the MR pipeline
        args = ['git', 'push', '-u', '-o', 'ci.skip',
                '-o', 'ci.variable=SJG_LAB=1']
        if force:
            args.append('--force-with-lease')
        args.extend([remote, branch])
        command.output(*args)
        return True
    except command.CommandExc as exc:
        tout.error(f'Failed to push branch: {exc}')
        return False


# pylint: disable=too-many-arguments
def create_mr(host, proj_path, source, target, title, desc=''):
    """Create a merge request via GitLab API

    Args:
        host (str): GitLab host
        proj_path (str): Project path (e.g., 'group/project')
        source (str): Source branch name
        target (str): Target branch name
        title (str): MR title
        desc (str): MR description

    Returns:
        str: MR URL on success, None on failure
    """
    if not check_available():
        return None

    token = get_token()
    if not token:
        tout.error('GITLAB_TOKEN environment variable not set')
        return None

    try:
        glab = gitlab.Gitlab(f'https://{host}', private_token=token)
        project = glab.projects.get(proj_path)

        merge_req = project.mergerequests.create({
            'source_branch': source,
            'target_branch': target,
            'title': title,
            'description': desc,
            'remove_source_branch': False,
        })

        return merge_req.web_url
    except gitlab.exceptions.GitlabError as exc:
        tout.error(f'GitLab API error: {exc}')
        return None


def push_and_create_mr(remote, branch, target, title, desc=''):
    """Push a branch and create a merge request

    Args:
        remote (str): Remote name
        branch (str): Branch to push
        target (str): Target branch for MR
        title (str): MR title
        desc (str): MR description

    Returns:
        str: MR URL on success, None on failure
    """
    # Get remote URL and parse it
    remote_url = get_remote_url(remote)
    host, proj_path = parse_url(remote_url)

    if not host or not proj_path:
        tout.error(f"Could not parse GitLab URL from remote '{remote}': "
                   f'{remote_url}')
        return None

    tout.info(f'Pushing {branch} to {remote}...')
    if not push_branch(remote, branch, force=True):
        return None

    tout.info(f'Creating merge request to {target}...')
    mr_url = create_mr(host, proj_path, branch, target, title, desc)

    if mr_url:
        tout.info(f'Merge request created: {mr_url}')

    return mr_url

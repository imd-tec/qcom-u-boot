# SPDX-License-Identifier: GPL-2.0+
#
# Copyright 2025 Canonical Ltd.
# Written by Simon Glass <simon.glass@canonical.com>
#
"""GitLab integration for pickman - push branches and create merge requests."""

from collections import namedtuple
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


# Merge request info returned by get_pickman_mrs()
PickmanMr = namedtuple('PickmanMr', [
    'iid', 'title', 'web_url', 'source_branch', 'description'
])

# Comment info returned by get_mr_comments()
MrComment = namedtuple('MrComment', [
    'id', 'author', 'body', 'created_at', 'resolvable', 'resolved'
])


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


def get_pickman_mrs(remote, state='opened'):
    """Get merge requests created by pickman

    Args:
        remote (str): Remote name
        state (str): MR state ('opened', 'merged', 'closed', 'all')

    Returns:
        list: List of PickmanMr tuples, or None on failure
    """
    if not check_available():
        return None

    token = get_token()
    if not token:
        tout.error('GITLAB_TOKEN environment variable not set')
        return None

    remote_url = get_remote_url(remote)
    host, proj_path = parse_url(remote_url)

    if not host or not proj_path:
        tout.error(f"Could not parse GitLab URL from remote '{remote}'")
        return None

    try:
        glab = gitlab.Gitlab(f'https://{host}', private_token=token)
        project = glab.projects.get(proj_path)

        mrs = project.mergerequests.list(state=state, get_all=True)
        pickman_mrs = []
        for merge_req in mrs:
            if '[pickman]' in merge_req.title:
                pickman_mrs.append(PickmanMr(
                    iid=merge_req.iid,
                    title=merge_req.title,
                    web_url=merge_req.web_url,
                    source_branch=merge_req.source_branch,
                    description=merge_req.description or '',
                ))
        return pickman_mrs
    except gitlab.exceptions.GitlabError as exc:
        tout.error(f'GitLab API error: {exc}')
        return None


def get_open_pickman_mrs(remote):
    """Get open merge requests created by pickman

    Args:
        remote (str): Remote name

    Returns:
        list: List of dicts with 'iid', 'title', 'web_url', 'source_branch' keys,
              or None on failure
    """
    return get_pickman_mrs(remote, state='opened')


def get_merged_pickman_mrs(remote):
    """Get merged merge requests created by pickman

    Args:
        remote (str): Remote name

    Returns:
        list: List of dicts with 'iid', 'title', 'web_url', 'source_branch',
              'description' keys, or None on failure
    """
    return get_pickman_mrs(remote, state='merged')


def get_mr_comments(remote, mr_iid):
    """Get human comments on a merge request (excluding bot/system notes)

    Args:
        remote (str): Remote name
        mr_iid (int): Merge request IID

    Returns:
        list: List of MrComment tuples, or None on failure
    """
    if not check_available():
        return None

    token = get_token()
    if not token:
        tout.error('GITLAB_TOKEN environment variable not set')
        return None

    remote_url = get_remote_url(remote)
    host, proj_path = parse_url(remote_url)

    if not host or not proj_path:
        return None

    try:
        glab = gitlab.Gitlab(f'https://{host}', private_token=token)
        project = glab.projects.get(proj_path)
        merge_req = project.mergerequests.get(mr_iid)

        comments = []
        for note in merge_req.notes.list(get_all=True):
            # Skip system notes (merge status, etc.)
            if note.system:
                continue
            comments.append(MrComment(
                id=note.id,
                author=note.author['username'],
                body=note.body,
                created_at=note.created_at,
                resolvable=getattr(note, 'resolvable', False),
                resolved=getattr(note, 'resolved', False),
            ))
        return comments
    except gitlab.exceptions.GitlabError as exc:
        tout.error(f'GitLab API error: {exc}')
        return None


def reply_to_mr(remote, mr_iid, message):
    """Post a reply to a merge request

    Args:
        remote (str): Remote name
        mr_iid (int): Merge request IID
        message (str): Reply message

    Returns:
        bool: True on success
    """
    if not check_available():
        return False

    token = get_token()
    if not token:
        tout.error('GITLAB_TOKEN environment variable not set')
        return False

    remote_url = get_remote_url(remote)
    host, proj_path = parse_url(remote_url)

    if not host or not proj_path:
        return False

    try:
        glab = gitlab.Gitlab(f'https://{host}', private_token=token)
        project = glab.projects.get(proj_path)
        merge_req = project.mergerequests.get(mr_iid)
        merge_req.notes.create({'body': message})
        return True
    except gitlab.exceptions.GitlabError as exc:
        tout.error(f'GitLab API error: {exc}')
        return False


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

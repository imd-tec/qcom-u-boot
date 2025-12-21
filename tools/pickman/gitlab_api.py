# SPDX-License-Identifier: GPL-2.0+
#
# Copyright 2025 Canonical Ltd.
# Written by Simon Glass <simon.glass@canonical.com>
#
"""GitLab integration for pickman - push branches and create merge requests."""

from collections import namedtuple
import configparser
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


class MrCreateError(Exception):
    """Exception for MR creation failures, used for testing

    This mirrors gitlab.exceptions.GitlabCreateError so tests don't need
    to import the gitlab module.
    """
    def __init__(self, response_code=None, message=''):
        self.response_code = response_code
        super().__init__(message)


# Merge request info returned by get_pickman_mrs()
# Use defaults for new fields so existing code doesn't break
PickmanMr = namedtuple('PickmanMr', [
    'iid', 'title', 'web_url', 'source_branch', 'description',
    'has_conflicts', 'needs_rebase'
], defaults=[False, False])

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


CONFIG_FILE = os.path.expanduser('~/.config/pickman.conf')


def get_config_value(section, key):
    """Get a value from the pickman config file

    Args:
        section (str): Config section name
        key (str): Config key name

    Returns:
        str: Value or None if not found
    """
    if not os.path.exists(CONFIG_FILE):
        return None

    config = configparser.ConfigParser()
    config.read(CONFIG_FILE)

    try:
        return config.get(section, key)
    except (configparser.NoSectionError, configparser.NoOptionError):
        return None


def get_token():
    """Get GitLab API token from config file or environment

    Checks in order:
    1. Config file (~/.config/pickman.conf) [gitlab] token
    2. GITLAB_TOKEN environment variable
    3. GITLAB_API_TOKEN environment variable

    Returns:
        str: Token or None if not set
    """
    # Try config file first
    token = get_config_value('gitlab', 'token')
    if token:
        return token

    # Fall back to environment variables
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


def get_push_url(remote):
    """Get a push URL using the GitLab API token for authentication

    This allows pushing as the token owner (e.g., a bot account) rather than
    using the user's configured git credentials.

    Args:
        remote (str): Remote name

    Returns:
        str: HTTPS URL with embedded token, or None if not available
    """
    token = get_token()
    if not token:
        return None

    url = get_remote_url(remote)
    host, proj_path = parse_url(url)
    if not host or not proj_path:
        return None

    return f'https://oauth2:{token}@{host}/{proj_path}.git'


def push_branch(remote, branch, force=False, skip_ci=True):
    """Push a branch to a remote

    Uses the GitLab API token for authentication if available, so the push
    comes from the token owner (e.g., a bot account) rather than the user's
    configured git credentials.

    Args:
        remote (str): Remote name
        branch (str): Branch name
        force (bool): Force push (overwrite remote branch)
        skip_ci (bool): Skip CI pipeline (default True for new MRs where
            MR pipeline runs automatically; set False for updates that
            need pipeline verification)

    Returns:
        bool: True on success
    """
    try:
        # Use token-authenticated URL if available
        push_url = get_push_url(remote)
        push_target = push_url if push_url else remote

        # When using --force-with-lease with an HTTPS URL (not remote name),
        # git can't find tracking refs automatically. Try to fetch first to
        # update the tracking ref. If fetch fails (branch doesn't exist on
        # remote yet), use regular --force instead of --force-with-lease.
        have_remote_ref = False
        if force and push_url:
            try:
                command.output('git', 'fetch', remote, branch)
                have_remote_ref = True
            except command.CommandExc:
                pass  # Branch doesn't exist on remote, will use --force

        args = ['git', 'push', '-u']
        if skip_ci:
            args.extend(['-o', 'ci.skip'])
        if force:
            if have_remote_ref:
                args.append(f'--force-with-lease=refs/remotes/{remote}/{branch}')
            else:
                args.append('--force')
        args.extend([push_target, f'HEAD:{branch}'])
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
    except (gitlab.exceptions.GitlabCreateError, MrCreateError) as exc:
        # 409 means MR already exists for this source branch
        if exc.response_code == 409:
            mrs = project.mergerequests.list(
                source_branch=source, state='opened')
            if mrs:
                tout.info(f'MR already exists: {mrs[0].web_url}')
                return mrs[0].web_url
        tout.error(f'GitLab API error: {exc}')
        return None
    except gitlab.exceptions.GitlabError as exc:
        tout.error(f'GitLab API error: {exc}')
        return None


# pylint: disable=too-many-locals
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

        # Sort by created_at ascending so oldest MRs are processed first
        mrs = project.mergerequests.list(state=state, order_by='created_at',
                                         sort='asc', get_all=True)
        pickman_mrs = []
        for merge_req in mrs:
            if '[pickman]' in merge_req.title:
                needs_rebase = False
                has_conflicts = False

                # For open MRs, fetch full details since list() doesn't
                # include accurate merge status fields
                if state == 'opened':
                    full_mr = project.mergerequests.get(merge_req.iid)
                    has_conflicts = getattr(full_mr, 'has_conflicts', False)

                    # Check merge status - detailed_merge_status is newer API
                    detailed_status = getattr(full_mr,
                                              'detailed_merge_status', '')
                    needs_rebase = detailed_status == 'need_rebase'
                    # Also check diverged_commits_count as fallback
                    if not needs_rebase:
                        diverged = getattr(full_mr, 'diverged_commits_count', 0)
                        needs_rebase = diverged and diverged > 0

                pickman_mrs.append(PickmanMr(
                    iid=merge_req.iid,
                    title=merge_req.title,
                    web_url=merge_req.web_url,
                    source_branch=merge_req.source_branch,
                    description=merge_req.description or '',
                    has_conflicts=has_conflicts,
                    needs_rebase=needs_rebase,
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


def update_mr_description(remote, mr_iid, desc):
    """Update a merge request's description

    Args:
        remote (str): Remote name
        mr_iid (int): Merge request IID
        desc (str): New description

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
        merge_req.description = desc
        merge_req.save()
        return True
    except gitlab.exceptions.GitlabError as exc:
        tout.error(f'GitLab API error: {exc}')
        return False


def update_mr_title(remote, mr_iid, title):
    """Update a merge request's title

    Args:
        remote (str): Remote name
        mr_iid (int): Merge request IID
        title (str): New title

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
        merge_req.title = title
        merge_req.save()
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


# Access level constants from GitLab
ACCESS_LEVELS = {
    0: 'No access',
    5: 'Minimal access',
    10: 'Guest',
    20: 'Reporter',
    30: 'Developer',
    40: 'Maintainer',
    50: 'Owner',
}

# Permission info returned by check_permissions()
PermissionInfo = namedtuple('PermissionInfo', [
    'user', 'user_id', 'access_level', 'access_name',
    'can_push', 'can_create_mr', 'can_merge', 'project', 'host'
])


def check_permissions(remote):  # pylint: disable=too-many-return-statements
    """Check GitLab permissions for the current token

    Args:
        remote (str): Remote name

    Returns:
        PermissionInfo: Permission info, or None on failure
    """
    if not check_available():
        return None

    token = get_token()
    if not token:
        tout.error('No GitLab token configured')
        tout.error('Set token in ~/.config/pickman.conf or GITLAB_TOKEN env var')
        return None

    remote_url = get_remote_url(remote)
    host, proj_path = parse_url(remote_url)

    if not host or not proj_path:
        tout.error(f"Could not parse GitLab URL from remote '{remote}'")
        return None

    try:
        glab = gitlab.Gitlab(f'https://{host}', private_token=token)
        glab.auth()
        user = glab.user

        project = glab.projects.get(proj_path)

        # Get user's access level in this project
        access_level = 0
        try:
            # Try to get the member directly
            member = project.members.get(user.id)
            access_level = member.access_level
        except gitlab.exceptions.GitlabGetError:
            # User might have inherited access from a group
            try:
                member = project.members_all.get(user.id)
                access_level = member.access_level
            except gitlab.exceptions.GitlabGetError:
                pass

        access_name = ACCESS_LEVELS.get(access_level, f'Unknown ({access_level})')

        return PermissionInfo(
            user=user.username,
            user_id=user.id,
            access_level=access_level,
            access_name=access_name,
            can_push=access_level >= 30,  # Developer or higher
            can_create_mr=access_level >= 30,  # Developer or higher
            can_merge=access_level >= 40,  # Maintainer or higher
            project=proj_path,
            host=host,
        )
    except gitlab.exceptions.GitlabAuthenticationError as exc:
        tout.error(f'Authentication failed: {exc}')
        return None
    except gitlab.exceptions.GitlabGetError as exc:
        tout.error(f'Could not access project: {exc}')
        return None
    except gitlab.exceptions.GitlabError as exc:
        tout.error(f'GitLab API error: {exc}')
        return None

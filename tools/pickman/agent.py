# SPDX-License-Identifier: GPL-2.0+
#
# Copyright 2025 Canonical Ltd.
# Written by Simon Glass <simon.glass@canonical.com>
#
"""Agent module for pickman - uses Claude to automate cherry-picking."""

import asyncio
import os
import sys

# Allow 'from pickman import xxx' to work via symlink
our_path = os.path.dirname(os.path.realpath(__file__))
sys.path.insert(0, os.path.join(our_path, '..'))

# pylint: disable=wrong-import-position,import-error
from u_boot_pylib import tout

# Signal file for agent to communicate status back to pickman
SIGNAL_FILE = '.pickman-signal'

# Signal status codes
SIGNAL_SUCCESS = 'success'
SIGNAL_APPLIED = 'already_applied'
SIGNAL_CONFLICT = 'conflict'

# Check if claude_agent_sdk is available
try:
    from claude_agent_sdk import query, ClaudeAgentOptions
    AGENT_AVAILABLE = True
except ImportError:
    AGENT_AVAILABLE = False


def check_available():
    """Check if the Claude Agent SDK is available

    Returns:
        bool: True if available, False otherwise
    """
    if not AGENT_AVAILABLE:
        tout.error('Claude Agent SDK not available')
        tout.error('Install with: pip install claude-agent-sdk')
        return False
    return True


async def run(commits, source, branch_name, repo_path=None):
    """Run the Claude agent to cherry-pick commits

    Args:
        commits (list): list of (hash, short_hash, subject) tuples
        source (str): source branch name
        branch_name (str): name for the new branch to create
        repo_path (str): path to repository (defaults to current directory)

    Returns:
        bool: True on success, False on failure
    """
    if not check_available():
        return False

    if repo_path is None:
        repo_path = os.getcwd()

    # Remove any stale signal file from previous runs
    signal_path = os.path.join(repo_path, SIGNAL_FILE)
    if os.path.exists(signal_path):
        os.remove(signal_path)

    # Build commit list for the prompt
    commit_list = '\n'.join(
        f'  - {short_hash}: {subject}'
        for _, short_hash, subject in commits
    )

    # Get full hash of last commit for signal file
    last_commit_hash = commits[-1][0]

    prompt = f"""Cherry-pick the following commits from {source} branch:

{commit_list}

Steps to follow:
1. First run 'git status' to check the repository state is clean
2. Create and checkout a new branch based on ci/master:
   git checkout -b {branch_name} ci/master
3. Cherry-pick each commit in order:
   - For regular commits: git cherry-pick -x <hash>
   - For merge commits (identified by "Merge" in subject): git cherry-pick -x -m 1 --allow-empty <hash>
   Cherry-pick one commit at a time to handle each appropriately.
   IMPORTANT: Always include merge commits even if they result in empty commits.
   The merge commit message is important for tracking history.
4. If there are conflicts:
   - Show the conflicting files
   - Try to resolve simple conflicts automatically
   - For complex conflicts, describe what needs manual resolution and abort
   - When fix-ups are needed, amend the commit to add a one-line note at the
     end of the commit message describing the changes made
5. After ALL cherry-picks complete, verify with
   'git log --oneline -n {len(commits) + 2}'
   Ensure all {len(commits)} commits are present.
6. Run 'buildman -L --board sandbox -w -o /tmp/pickman' to verify the build
7. Report the final status including:
   - Build result (ok or list of warnings/errors)
   - Any fix-ups that were made

The cherry-pick branch will be left ready for pushing. Do NOT merge it back to any other branch.

Important:
- Stop immediately if there's a conflict that cannot be auto-resolved
- Do not force push or modify history
- If cherry-pick fails, run 'git cherry-pick --abort'
- NEVER skip merge commits - always use --allow-empty to preserve them

CRITICAL - Detecting Already-Applied Commits:
If the FIRST cherry-pick fails with conflicts, BEFORE aborting, check if the commits
are already present in ci/master with different hashes. Do this by searching for
commit subjects in ci/master:
   git log --oneline ci/master --grep="<subject>" -1
If ALL commits are already in ci/master (same subjects, different hashes), this means
the series was already applied via a different path. In this case:
1. Abort the cherry-pick: git cherry-pick --abort
2. Delete the branch: git branch -D {branch_name}
3. Write a signal file to indicate this status:
   echo "already_applied" > {SIGNAL_FILE}
   echo "{last_commit_hash}" >> {SIGNAL_FILE}
4. Report that all {len(commits)} commits are already present in ci/master
"""

    options = ClaudeAgentOptions(
        allowed_tools=['Bash', 'Read', 'Grep'],
        cwd=repo_path,
    )

    tout.info(f'Starting Claude agent to cherry-pick {len(commits)} commits...')
    tout.info('')

    conversation_log = []
    try:
        async for message in query(prompt=prompt, options=options):
            # Print agent output and capture it
            if hasattr(message, 'content'):
                for block in message.content:
                    if hasattr(block, 'text'):
                        print(block.text)
                        conversation_log.append(block.text)
        return True, '\n\n'.join(conversation_log)
    except (RuntimeError, ValueError, OSError) as exc:
        tout.error(f'Agent failed: {exc}')
        return False, '\n\n'.join(conversation_log)


def read_signal_file(repo_path=None):
    """Read and remove the signal file if it exists

    Args:
        repo_path (str): path to repository (defaults to current directory)

    Returns:
        tuple: (status, last_commit) where status is the signal status code
            (e.g., 'already_applied') and last_commit is the commit hash,
            or (None, None) if no signal file exists
    """
    if repo_path is None:
        repo_path = os.getcwd()

    signal_path = os.path.join(repo_path, SIGNAL_FILE)
    if not os.path.exists(signal_path):
        return None, None

    try:
        with open(signal_path, 'r', encoding='utf-8') as fhandle:
            lines = fhandle.read().strip().split('\n')
        status = lines[0] if lines else None
        last_commit = lines[1] if len(lines) > 1 else None

        # Remove the signal file after reading
        os.remove(signal_path)

        return status, last_commit
    except (IOError, OSError) as exc:
        tout.warning(f'Failed to read signal file: {exc}')
        return None, None


def cherry_pick_commits(commits, source, branch_name, repo_path=None):
    """Synchronous wrapper for running the cherry-pick agent

    Args:
        commits (list): list of (hash, short_hash, subject) tuples
        source (str): source branch name
        branch_name (str): name for the new branch to create
        repo_path (str): path to repository (defaults to current directory)

    Returns:
        tuple: (success, conversation_log) where success is bool and
            conversation_log is the agent's output text
    """
    return asyncio.run(run(commits, source, branch_name, repo_path))


def build_review_context(comments, mr_description, needs_rebase, remote,
                         target):
    """Build context sections for the review agent prompt

    Args:
        comments (list): List of comment dicts with 'author', 'body' keys
        mr_description (str): MR description with context from previous work
        needs_rebase (bool): Whether the MR needs rebasing
        remote (str): Git remote name
        target (str): Target branch for rebase operations

    Returns:
        tuple: (context_section, comment_section, rebase_section)
    """
    # Include MR description for context from previous work
    context_section = ''
    if mr_description:
        context_section = f'''
Context from MR description (includes previous work done on this MR):

{mr_description}

Use this context to understand what was done previously and respond appropriately.
'''

    # Format comments for the prompt (if any)
    comment_section = ''
    if comments:
        comment_text = '\n'.join(f'- [{c.author}]: {c.body}' for c in comments)
        comment_section = f'''
Review comments to address:

{comment_text}
'''

    # Build rebase instructions
    rebase_section = ''
    if needs_rebase:
        rebase_section = f'''
Rebase instructions:
- The MR is behind the target branch and needs rebasing
- Use: git rebase --keep-empty {remote}/{target}
- This preserves empty merge commits which are important for tracking
- If there are conflicts, try to resolve them automatically
- For complex conflicts that cannot be resolved, describe them and abort
'''

    return context_section, comment_section, rebase_section


# pylint: disable=too-many-arguments
def build_review_prompt(mr_iid, branch_name, task_desc, context_section,
                        comment_section, rebase_section, comments,
                        needs_rebase, remote, target):
    """Build the main prompt for the review agent

    Args:
        mr_iid (int): Merge request IID
        branch_name (str): Source branch name
        task_desc (str): Description of tasks to perform
        context_section (str): Context from MR description
        comment_section (str): Formatted review comments
        rebase_section (str): Rebase instructions
        comments (list): List of comments (to check if non-empty)
        needs_rebase (bool): Whether the MR needs rebasing
        remote (str): Git remote name
        target (str): Target branch for rebase operations

    Returns:
        str: The complete prompt for the agent
    """
    # Build step2 instruction based on whether rebase is needed
    if needs_rebase:
        step2 = f'Rebase onto {remote}/{target} first'
    else:
        step2 = 'Read and understand each comment'

    if needs_rebase and comments:
        step3 = 'After rebase, address any review comments'
    elif comments:
        step3 = 'For each actionable comment:'
    else:
        step3 = 'Verify the rebase completed successfully'

    comment_steps = ''
    if comments:
        comment_steps = """   - Make the requested changes to the code
   - Amend the relevant commit or create a fixup commit"""

    return f"""Task for merge request !{mr_iid} (branch: {branch_name}):
{task_desc}
{context_section}{comment_section}{rebase_section}
Steps to follow:
1. Checkout the branch: git checkout {branch_name}
2. {step2}
3. {step3}
{comment_steps}
4. Run 'buildman -L --board sandbox -w -o /tmp/pickman' to verify the build
5. Create a local branch with suffix '-v2' (or increment: -v3, -v4, etc.)
6. Force push to the ORIGINAL remote branch to update the MR:
   ./tools/pickman/pickman push-branch {branch_name} -r {remote} -f
   (GitLab automatically triggers an MR pipeline when the branch is updated)
7. Report what was done and what reply should be posted to the MR

Important:
- Keep changes minimal and focused
- If a comment is unclear or cannot be addressed, note this in your report
- Local branch: {branch_name}-v2 (or -v3, -v4 etc.)
- Remote push: always to '{branch_name}' to update the existing MR
- Do NOT update the MR title - it should remain as originally set
"""


# pylint: disable=too-many-arguments
def build_full_review_prompt(mr_iid, branch_name, comments, remote, target,
                             needs_rebase, has_conflicts, mr_description):
    """Build complete prompt and task description for the review agent

    Args:
        mr_iid (int): Merge request IID
        branch_name (str): Source branch name
        comments (list): List of comment dicts with 'author', 'body' keys
        remote (str): Git remote name
        target (str): Target branch for rebase operations
        needs_rebase (bool): Whether the MR needs rebasing
        has_conflicts (bool): Whether the MR has merge conflicts
        mr_description (str): MR description with context from previous work

    Returns:
        tuple: (prompt, task_desc) where prompt is the full agent prompt and
            task_desc is a short description of the tasks
    """
    # Build the task description
    tasks = []
    if needs_rebase:
        if has_conflicts:
            tasks.append('rebase and resolve merge conflicts')
        else:
            tasks.append('rebase onto latest target branch')
    if comments:
        tasks.append(f'address {len(comments)} review comment(s)')
    task_desc = ' and '.join(tasks)

    # Build context sections
    context_section, comment_section, rebase_section = build_review_context(
        comments, mr_description, needs_rebase, remote, target)

    # Build the prompt
    prompt = build_review_prompt(
        mr_iid, branch_name, task_desc, context_section, comment_section,
        rebase_section, comments, needs_rebase, remote, target)

    return prompt, task_desc


# pylint: disable=too-many-arguments,too-many-locals
async def run_review_agent(mr_iid, branch_name, comments, remote,
                           target='master', needs_rebase=False,
                           has_conflicts=False, mr_description='',
                           repo_path=None):
    """Run the Claude agent to handle MR comments and/or rebase

    Args:
        mr_iid (int): Merge request IID
        branch_name (str): Source branch name
        comments (list): List of comment dicts with 'author', 'body' keys
        remote (str): Git remote name
        target (str): Target branch for rebase operations
        needs_rebase (bool): Whether the MR needs rebasing
        has_conflicts (bool): Whether the MR has merge conflicts
        mr_description (str): MR description with context from previous work
        repo_path (str): Path to repository (defaults to current directory)

    Returns:
        bool: True on success, False on failure
    """
    if not check_available():
        return False

    if repo_path is None:
        repo_path = os.getcwd()

    prompt, task_desc = build_full_review_prompt(
        mr_iid, branch_name, comments, remote, target, needs_rebase,
        has_conflicts, mr_description)

    options = ClaudeAgentOptions(
        allowed_tools=['Bash', 'Read', 'Grep', 'Edit', 'Write'],
        cwd=repo_path,
    )

    tout.info(f'Starting Claude agent to {task_desc}...')
    tout.info('')

    conversation_log = []
    try:
        async for message in query(prompt=prompt, options=options):
            if hasattr(message, 'content'):
                for block in message.content:
                    if hasattr(block, 'text'):
                        print(block.text)
                        conversation_log.append(block.text)
        return True, '\n\n'.join(conversation_log)
    except (RuntimeError, ValueError, OSError) as exc:
        tout.error(f'Agent failed: {exc}')
        return False, '\n\n'.join(conversation_log)


# pylint: disable=too-many-arguments
def handle_mr_comments(mr_iid, branch_name, comments, remote, target='master',
                       needs_rebase=False, has_conflicts=False,
                       mr_description='', repo_path=None):
    """Synchronous wrapper for running the review agent

    Args:
        mr_iid (int): Merge request IID
        branch_name (str): Source branch name
        comments (list): List of comment dicts
        remote (str): Git remote name
        target (str): Target branch for rebase operations
        needs_rebase (bool): Whether the MR needs rebasing
        has_conflicts (bool): Whether the MR has merge conflicts
        mr_description (str): MR description with context from previous work
        repo_path (str): Path to repository (defaults to current directory)

    Returns:
        tuple: (success, conversation_log) where success is bool and
            conversation_log is the agent's output text
    """
    return asyncio.run(run_review_agent(mr_iid, branch_name, comments, remote,
                                        target, needs_rebase, has_conflicts,
                                        mr_description, repo_path))

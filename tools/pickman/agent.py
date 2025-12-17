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

    # Build commit list for the prompt
    commit_list = '\n'.join(
        f'  - {short_hash}: {subject}'
        for _, short_hash, subject in commits
    )

    prompt = f"""Cherry-pick the following commits from {source} branch:

{commit_list}

Steps to follow:
1. First run 'git status' to check the repository state is clean
2. Create and checkout a new branch based on ci/master: git checkout -b {branch_name} ci/master
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
   - When fix-ups are needed, amend the commit to add a one-line note at the end
     of the commit message describing the changes made
5. After ALL cherry-picks complete, verify with 'git log --oneline -n {len(commits) + 2}'
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
    return asyncio.run(run(commits, source, branch_name,
                                             repo_path))


async def run_review_agent(mr_iid, branch_name, comments, remote, repo_path=None):
    """Run the Claude agent to handle MR comments

    Args:
        mr_iid (int): Merge request IID
        branch_name (str): Source branch name
        comments (list): List of comment dicts with 'author', 'body' keys
        remote (str): Git remote name
        repo_path (str): Path to repository (defaults to current directory)

    Returns:
        bool: True on success, False on failure
    """
    if not check_available():
        return False

    if repo_path is None:
        repo_path = os.getcwd()

    # Format comments for the prompt
    comment_text = '\n'.join(
        f'- [{c.author}]: {c.body}'
        for c in comments
    )

    prompt = f"""Review comments on merge request !{mr_iid} (branch: {branch_name}):

{comment_text}

Steps to follow:
1. Checkout the branch: git checkout {branch_name}
2. Read and understand each comment
3. For each actionable comment:
   - Make the requested changes to the code
   - Amend the relevant commit or create a fixup commit
4. Run 'crosfw sandbox -L' to verify the build
5. Create a local branch with suffix '-v2' (or increment: -v3, -v4, etc.)
6. Force push to the ORIGINAL remote branch to update the MR:
   git push --force-with-lease {remote} HEAD:{branch_name}
7. Report what changes were made and what reply should be posted to the MR

Important:
- Keep changes minimal and focused on addressing the comments
- If a comment is unclear or cannot be addressed, note this in your report
- Local branch: {branch_name}-v2 (or -v3, -v4 etc.)
- Remote push: always to '{branch_name}' to update the existing MR
- If rebasing is requested, use: git rebase --keep-empty <base>
  This preserves empty merge commits which are important for tracking
"""

    options = ClaudeAgentOptions(
        allowed_tools=['Bash', 'Read', 'Grep', 'Edit', 'Write'],
        cwd=repo_path,
    )

    tout.info(f'Starting Claude agent to handle {len(comments)} comment(s)...')
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


def handle_mr_comments(mr_iid, branch_name, comments, remote, repo_path=None):
    """Synchronous wrapper for running the review agent

    Args:
        mr_iid (int): Merge request IID
        branch_name (str): Source branch name
        comments (list): List of comment dicts
        remote (str): Git remote name
        repo_path (str): Path to repository (defaults to current directory)

    Returns:
        tuple: (success, conversation_log) where success is bool and
            conversation_log is the agent's output text
    """
    return asyncio.run(run_review_agent(mr_iid, branch_name, comments, remote,
                                        repo_path))

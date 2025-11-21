#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0+
#
# Copyright 2025 Canonical Ltd
#
"""Test script for LSP client with clangd"""

import json
import os
import sys
import tempfile
import time

# Add parent directory to path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from lsp_client import LspClient  # pylint: disable=wrong-import-position


def test_clangd():
    """Test basic clangd functionality"""
    # Create a temporary directory with a simple C file
    with tempfile.TemporaryDirectory() as tmpdir:
        # Create a C file with CONFIG-style inactive code
        test_file = os.path.join(tmpdir, 'test.c')
        with open(test_file, 'w', encoding='utf-8') as f:
            f.write('''#include <stdio.h>

// Simulate U-Boot style CONFIG options
#define CONFIG_FEATURE_A 1

void always_compiled(void)
{
    printf("Always here\\n");
}

#ifdef CONFIG_FEATURE_A
void feature_a_code(void)
{
    printf("Feature A enabled\\n");
}
#endif

#ifdef CONFIG_FEATURE_B
void feature_b_code(void)
{
    printf("Feature B enabled (THIS SHOULD BE INACTIVE)\\n");
}
#endif

#if 0
void disabled_debug_code(void)
{
    printf("Debug code (INACTIVE)\\n");
}
#endif
''')

        # Create compile_commands.json
        compile_commands = [
            {
                'directory': tmpdir,
                'command': f'gcc -c {test_file}',
                'file': test_file
            }
        ]
        compile_db = os.path.join(tmpdir, 'compile_commands.json')
        with open(compile_db, 'w', encoding='utf-8') as f:
            json.dump(compile_commands, f)

        # Create .clangd config to enable inactive regions
        clangd_config = os.path.join(tmpdir, '.clangd')
        with open(clangd_config, 'w', encoding='utf-8') as f:
            f.write('''InactiveRegions:
  Opacity: 0.55
''')

        print(f'Created test file: {test_file}')
        print(f'Created compile DB: {compile_db}')
        print(f'Created clangd config: {clangd_config}')

        # Start clangd
        print('\\nStarting clangd...')
        with LspClient(['clangd', '--log=error',
                        f'--compile-commands-dir={tmpdir}']) as client:
            print('Initialising...')
            result = client.init(f'file://{tmpdir}')
            print(f'Server capabilities: {result.get("capabilities", {}).keys()}')

            # Open the document
            print(f'\\nOpening document: {test_file}')
            with open(test_file, 'r', encoding='utf-8') as f:
                content = f.read()

            client.notify('textDocument/didOpen', {
                'textDocument': {
                    'uri': f'file://{test_file}',
                    'languageId': 'c',
                    'version': 1,
                    'text': content
                }
            })

            # Wait for clangd to index the file
            print('\\nWaiting for clangd to index file...')
            time.sleep(3)

            # Check for inactive regions notification
            print('\\nChecking for inactive regions notification...')
            with client.lock:
                notifications = list(client.notifications)

            print(f'Received {len(notifications)} notifications:')
            inactive_regions = None
            for notif in notifications:
                method = notif.get('method', 'unknown')
                print(f'  - {method}')

                # Look for the clangd inactive regions extension
                if method == 'textDocument/clangd.inactiveRegions':
                    params = notif.get('params', {})
                    inactive_regions = params.get('inactiveRegions', [])
                    print(f'    Found {len(inactive_regions)} inactive regions!')

            if inactive_regions:
                print('\\nInactive regions:')
                for region in inactive_regions:
                    start = region['start']
                    end = region['end']
                    start_line = start['line'] + 1  # LSP is 0-indexed
                    end_line = end['line'] + 1
                    print(f'  Lines {start_line}-{end_line}')
            else:
                print('\\nNo inactive regions received (feature may not be enabled)')

            # Also show the file with line numbers for reference
            print('\\nFile contents:')
            for i, line in enumerate(content.split('\\n'), 1):
                print(f'{i:3}: {line}')

            print('\\nTest completed!')

            # Check clangd stderr for any errors
            print('\\n=== Clangd stderr output ===')
            stderr_output = client.process.stderr.read()
            if stderr_output:
                print(stderr_output[:1000])
            else:
                print('(no stderr output)')


if __name__ == '__main__':
    test_clangd()

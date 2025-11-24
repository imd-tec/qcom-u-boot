# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Canonical Ltd
#
"""Minimal LSP (Language Server Protocol) client for clangd.

This module provides a simple JSON-RPC 2.0 client for communicating with
LSP servers like clangd. It focuses on the specific functionality needed
for analyzing inactive preprocessor regions.
"""

import json
import subprocess
import threading
from typing import Any, Dict, Optional


class LspClient:
    """Minimal LSP client for JSON-RPC 2.0 communication.

    This client handles the basic LSP protocol communication over
    stdin/stdout with a language server process.

    Attributes:
        process: The language server subprocess
        next_id: Counter for JSON-RPC request IDs
        responses: Dict mapping request IDs to response data
        lock: Thread lock for response dictionary
        reader_thread: Background thread reading server responses
    """

    def __init__(self, server_command):
        """Init the LSP client and start the server.

        Args:
            server_command (list): Command to start the LSP server
                (e.g., ['clangd', '--log=error'])
        """
        self.process = subprocess.Popen(
            server_command,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=0
        )
        self.next_id = 1
        self.responses = {}
        self.notifications = []
        self.lock = threading.Lock()
        self.running = True

        # Start background thread to read responses
        self.reader_thread = threading.Thread(target=self._read_responses)
        self.reader_thread.daemon = True
        self.reader_thread.start()

    def _read_responses(self):
        """Background thread to read responses from the server"""
        while self.running and self.process.poll() is None:
            try:
                # Read headers
                headers = {}
                while True:
                    line = self.process.stdout.readline()
                    if not line or line == '\r\n' or line == '\n':
                        break
                    if ':' in line:
                        key, value = line.split(':', 1)
                        headers[key.strip()] = value.strip()

                if 'Content-Length' not in headers:
                    continue

                # Read content
                content_length = int(headers['Content-Length'])
                content = self.process.stdout.read(content_length)

                if not content:
                    break

                # Parse JSON
                message = json.loads(content)

                # Store response or notification
                with self.lock:
                    if 'id' in message:
                        # Response to a request
                        self.responses[message['id']] = message
                    else:
                        # Notification from server
                        self.notifications.append(message)

            except (json.JSONDecodeError, ValueError):
                continue
            except Exception:
                break

    def _send_message(self, message: Dict[str, Any]):
        """Send a JSON-RPC message to the server.

        Args:
            message: JSON-RPC message dictionary
        """
        content = json.dumps(message)
        headers = f'Content-Length: {len(content)}\r\n\r\n'
        self.process.stdin.write(headers + content)
        self.process.stdin.flush()

    def request(self, method: str, params: Optional[Dict] = None,
                timeout: int = 30) -> Optional[Dict]:
        """Send a JSON-RPC request and wait for response.

        Args:
            method: LSP method name (e.g., 'initialize')
            params: Method parameters dictionary
            timeout: Timeout in seconds (default: 30)

        Returns:
            Response dictionary, or None on timeout/error
        """
        request_id = self.next_id
        self.next_id += 1

        message = {
            'jsonrpc': '2.0',
            'id': request_id,
            'method': method,
        }
        if params:
            message['params'] = params

        self._send_message(message)

        # Wait for response
        import time
        start_time = time.time()
        while time.time() - start_time < timeout:
            with self.lock:
                if request_id in self.responses:
                    response = self.responses.pop(request_id)
                    if 'result' in response:
                        return response['result']
                    if 'error' in response:
                        raise RuntimeError(
                            f"LSP error: {response['error']}")
                    return response
            time.sleep(0.01)

        return None

    def notify(self, method: str, params: Optional[Dict] = None):
        """Send a JSON-RPC notification (no response expected).

        Args:
            method: LSP method name
            params: Method parameters dictionary
        """
        message = {
            'jsonrpc': '2.0',
            'method': method,
        }
        if params:
            message['params'] = params

        self._send_message(message)

    def init(self, root_uri: str, capabilities: Optional[Dict] = None) -> Dict:
        """Send initialize request to the server.

        Args:
            root_uri: Workspace root URI (e.g., 'file:///path/to/workspace')
            capabilities: Client capabilities dict

        Returns:
            Server capabilities from initialize response
        """
        if capabilities is None:
            capabilities = {
                'textDocument': {
                    'semanticTokens': {
                        'requests': {
                            'full': True
                        }
                    },
                    'publishDiagnostics': {},
                    'inactiveRegions': {
                        'refreshSupport': False
                    }
                }
            }

        result = self.request('initialize', {
            'processId': None,
            'rootUri': root_uri,
            'capabilities': capabilities
        })

        # Send initialized notification
        self.notify('initialized', {})

        return result

    def shutdown(self):
        """Shutdown the language server"""
        self.request('shutdown')
        self.notify('exit')
        self.running = False
        if self.process:
            self.process.wait(timeout=5)
            # Close file descriptors to avoid ResourceWarnings
            if self.process.stdin:
                self.process.stdin.close()
            if self.process.stdout:
                self.process.stdout.close()
            if self.process.stderr:
                self.process.stderr.close()

    def __enter__(self):
        """Context manager entry"""
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit - ensure cleanup"""
        self.shutdown()

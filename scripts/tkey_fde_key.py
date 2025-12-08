#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0+
# Copyright (C) 2025 Canonical Ltd

"""TKey Full Disk Encryption Key Generator

This script uses tkey-sign to generate encryption keys for full-disk encryption.
It prompts the user for a passphrase and uses the TKey's hardware-based key
derivation to create a consistent encryption key.

USAGE OVERVIEW:
==============

This tool provides three main functions:
1. Generate hardware-backed encryption keys using TKey
2. Encrypt disk images with LUKS using the derived keys
3. Open LUKS encrypted disks using the derived keys

BASIC USAGE:
-----------

# Generate a key interactively (prompts for password)
tkey-fde-key.py

# Generate key and save to file
tkey-fde-key.py -o /tmp/my-key.bin

# Read password from file instead of interactive prompt
tkey-fde-key.py -p /path/to/passfile

# Read password from stdin
echo 'mypassword' | tkey-fde-key.py -p -

DISK ENCRYPTION:
---------------

# Encrypt a disk image with LUKS (automatically resizes image for LUKS header)
tkey-fde-key.py -e /path/to/disk.img -p /path/to/passfile

# Encrypt disk with password from stdin
echo 'mypassword' | tkey-fde-key.py -e /path/to/disk.img -p -

# Encrypt disk and save backup key
tkey-fde-key.py -e /path/to/disk.img -p /path/to/passfile -o /tmp/backup.key

# Interactive encryption (will prompt for password)
tkey-fde-key.py -e /path/to/disk.img

DISK OPENING:
------------

# Open an encrypted disk (creates /dev/mapper/tkey-disk)
tkey-fde-key.py -O /path/to/encrypted.img -p /path/to/passfile

# Open with password from stdin
echo 'mypassword' | tkey-fde-key.py -O /path/to/encrypted.img -p -

# Open with custom mapper name
tkey-fde-key.py -O /path/to/encrypted.img -m my-disk -p /path/to/passfile

# Interactive opening (will prompt for password)
tkey-fde-key.py -O /path/to/encrypted.img

# After opening, mount the filesystem:
sudo mount /dev/mapper/tkey-disk /mnt

# When done, unmount and close:
sudo umount /mnt
sudo cryptsetup close tkey-disk

IMPORTANT NOTES:
===============

- The same password must be used to derive the same key
- TKey must be in firmware mode or will prompt for reinsertion
- Disk operations may require root privileges
- LUKS encryption uses AES-XTS-256 with SHA256
- Device mapper names must be unique system-wide
- Always backup important data before encryption

SECURITY:
========

- Keys are derived using TKey's hardware security module
- Temporary keyfiles are created with restrictive permissions (600)
- All temporary files are automatically cleaned up
- Password confirmation required for interactive input
- Empty passwords are not allowed

DEPENDENCIES:
============

- tkey-sign (TKey development tools)
- cryptsetup (for LUKS operations)
- truncate (for disk resizing)
- dmsetup (for device mapper operations)

For more examples, see the --help output.
"""

import argparse
import base64
import getpass
import glob
import hashlib
import os
import subprocess
import sys
import tempfile
import time
from types import SimpleNamespace

import serial

# Add the tools directory to the path for u_boot_pylib
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'tools'))

# pylint: disable=wrong-import-position,import-error
from u_boot_pylib import tools
from u_boot_pylib import tout

# TKey frame constants (from U-Boot tkey-uclass.c)
TKEY_FRAME_ID_CMD_V1 = 0
TKEY_ENDPOINT_FIRMWARE = 2
TKEY_STATUS_OK = 0
TKEY_LENGTH_1_BYTE = 0
TKEY_FW_CMD_NAME_VERSION = 0x01


def parse_args():
    """Parse command line arguments

    Returns:
        argparse.Namespace: Parsed command line arguments
    """
    parser = argparse.ArgumentParser(
        description='Generate full-disk encryption keys using TKey hardware',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
  # Generate key interactively
  tkey-fde-key.py

  # Generate key and save to file
  tkey-fde-key.py --output /tmp/disk.key

  # Read password from file
  tkey-fde-key.py --password-file /path/to/passfile

  # Read password from stdin
  echo 'mypassword' | tkey-fde-key.py --password-file -

  # Output binary format instead of hex
  tkey-fde-key.py --binary

  # Encrypt a disk image with LUKS
  tkey-fde-key.py -e /path/to/disk.img

  # Encrypt specific partition (will prompt for selection)
  tkey-fde-key.py -e /path/to/disk.img -P 2

  # Open an encrypted disk image
  tkey-fde-key.py -O /path/to/encrypted.img

  # Open disk with custom mapper name
  tkey-fde-key.py -O /path/to/encrypted.img -m my-disk
'''
    )

    parser.add_argument(
        '--device',
        help='TKey serial device (auto-detected if not specified)'
    )
    parser.add_argument(
        '--output', '-o',
        help='Output file for the key (prints to stdout if not specified)'
    )
    parser.add_argument(
        '--binary',
        action='store_true',
        help='Output key in binary format (default is hex)'
    )
    parser.add_argument(
        '--force', '-f',
        action='store_true',
        help='Overwrite existing output file'
    )
    parser.add_argument(
        '--verbose', '-v',
        action='store_true',
        help='Enable verbose output'
    )
    parser.add_argument(
        '--debug', '-d',
        action='store_true',
        help='Enable debug mode (passes --verbose to tkey-sign)'
    )
    parser.add_argument(
        '--password-file', '-p',
        help="Read password from file (use '-' for stdin)"
    )
    parser.add_argument(
        '--encrypt-disk', '-e',
        help='Disk image file to encrypt with LUKS using the derived key'
    )
    parser.add_argument(
        '--open-disk', '-O',
        help='LUKS encrypted disk image to open using the derived key'
    )
    parser.add_argument(
        '--mapper-name', '-m',
        default='tkey-disk',
        help='Device mapper name for opened disk (default: tkey-disk)'
    )
    parser.add_argument(
        '--partition', '-P',
        type=int,
        help='Partition number to encrypt (if not specified, will prompt or encrypt whole disk)'
    )

    return parser.parse_args()

def run_sudo(cmd, inp=None, timeout=60, capture=True):
    """Run a command with sudo, handling password prompts properly

    Args:
        cmd (list): Command and arguments to run with sudo
        inp (str, optional): Data to pass to stdin. Defaults to None.
        timeout (int, optional): Command timeout in seconds. Defaults to 60.
        capture (bool, optional): Capture stdout/stderr. Defaults to True.

    Returns:
        subprocess.CompletedProcess: Result of the subprocess run
    """
    # Check if we're already running as root
    if not os.geteuid():
        # Already root, run command directly
        sudo_cmd = cmd
    else:
        # Not root, use sudo with environment preservation
        # Preserve PATH and other important environment variables
        env_vars = []
        for var in ['PATH', 'HOME', 'USER']:
            if var in os.environ:
                env_vars.extend([f'{var}={os.environ[var]}'])

        if env_vars:
            sudo_cmd = ['sudo', 'env'] + env_vars + cmd
        else:
            sudo_cmd = ['sudo'] + cmd

    cmd_str = ' '.join(sudo_cmd)
    # Mask environment variables in verbose output for cleanliness
    if 'env' in cmd_str:
        cmd_str = cmd_str.replace('env PATH=', 'env PATH=...')
    tout.detail(f'Running: {cmd_str}')

    if capture:
        return subprocess.run(
            sudo_cmd,
            input=inp,
            capture_output=True,
            text=True,
            timeout=timeout,
            check=True
        )
    # Allow interactive sudo (don't capture output)
    return subprocess.run(
        sudo_cmd,
        input=inp,
        text=True,
        timeout=timeout,
        check=False
    )

def find_tkey_sign():
    """Find the full path to tkey-sign command

    Returns:
        str or None: Full path to tkey-sign or None if not found
    """
    # Check if tkey-sign is in current PATH
    try:
        result = subprocess.run(['which', 'tkey-sign'], capture_output=True,
                                text=True, timeout=5, check=False)
        if not result.returncode:
            return result.stdout.strip()
    except (subprocess.TimeoutExpired, OSError):
        pass

    # If running as root, check the original user's paths
    original_user = os.environ.get('SUDO_USER')
    if original_user and not os.geteuid():
        import pwd  # pylint: disable=import-outside-toplevel
        try:
            user_info = pwd.getpwnam(original_user)
            user_home = user_info.pw_dir
            user_paths = [
                f'{user_home}/bin/tkey-sign',
                f'{user_home}/.local/bin/tkey-sign',
            ]
            for path in user_paths:
                if os.path.exists(path) and os.access(path, os.X_OK):
                    return path
        except KeyError:
            pass

    # Common system locations
    common_paths = [
        '/usr/local/bin/tkey-sign',
        '/usr/bin/tkey-sign',
        os.path.expanduser('~/bin/tkey-sign'),
        os.path.expanduser('~/.local/bin/tkey-sign'),
    ]

    for path in common_paths:
        if os.path.exists(path) and os.access(path, os.X_OK):
            return path

    return None

def run_tkey_sign(args, inp=None):
    """Run tkey-sign command with given arguments

    Args:
        args (list): Command line arguments for tkey-sign
        inp (str, optional): Data to pass to stdin. Defaults to None.

    Returns:
        subprocess.CompletedProcess: Result of the subprocess run
    """
    # Find tkey-sign path
    fname = find_tkey_sign()
    if not fname:
        tout.warning('tkey-sign not found, using PATH search')
        fname = 'tkey-sign'

    cmd = [fname] + args

    # Show the command being run, but mask USS input for security
    cmd_str = ' '.join(cmd)
    if '--uss-file' in args:
        tout.detail(f'Running: {cmd_str} (with USS file)')
    elif inp and '--uss' in args:
        tout.detail(f'Running: {cmd_str} (with USS input)')
    else:
        tout.detail(f'Running: {cmd_str}')

    result = subprocess.run(
        cmd,
        input=inp,
        capture_output=True,
        text=True,
        timeout=30,
        check=True
    )

    tout.detail(f'Command exit code: {result.returncode}')
    if result.stdout:
        tout.detail(f'Command stdout: {result.stdout}')
    if result.stderr:
        tout.detail(f'Command stderr: {result.stderr}')

    return result

def get_tkey_pubkey(device=None):
    """Get the public key from TKey with optional USS

    Args:
        device (str, optional): TKey device path. Defaults to None
            (auto-detect).

    Returns:
        str or None: Public key string on success, None on failure
    """
    with tempfile.NamedTemporaryFile(mode='w+', suffix='.pub',
                                     delete=False) as f:
        pubkey_file = f.name

    args = ['--getkey', '--public', pubkey_file, '--force']
    if device:
        args.extend(['--port', device])

    result = run_tkey_sign(args)
    if not result or result.returncode:
        err = result.stderr if result else 'Unknown error'
        tout.error(f'Error getting public key: {err}')
        if os.path.exists(pubkey_file):
            os.unlink(pubkey_file)
        return None

    pubkey = tools.read_file(pubkey_file, binary=False).strip()

    if os.path.exists(pubkey_file):
        os.unlink(pubkey_file)

    return pubkey

def check_tkey_mode(device=None):
    """Check if TKey is in firmware mode or has an app loaded

    Uses the same logic as U-Boot's tkey_get_name_version() function to detect
    mode. In firmware mode: returns name0='tk1 ' name1='mkdf'
    In app mode: firmware commands return error status with error code 0x00

    Args:
        device (str, optional): TKey device path. Defaults to None
            (auto-detect).

    Returns:
        str or None: 'firmware' if in firmware mode, 'app' if app loaded, None
            on error
    """
    # Try to get name/version to determine mode (same as U-Boot does)
    with tempfile.NamedTemporaryFile(mode='w', suffix='.temp',
                                     delete=False) as temp_f:
        temp_file = temp_f.name

    # Use a simple command that should work regardless of USS
    args = ['--getkey', '--public', temp_file, '--force']
    if device:
        args.extend(['--port', device])

    result = run_tkey_sign(args)

    # Read content before cleanup
    content = ''
    if os.path.exists(temp_file):
        content = tools.read_file(temp_file, binary=False).strip()
        os.unlink(temp_file)

    if not result:
        return None

    if result.returncode:
        # Command failed - could indicate various issues
        return None

    # In app mode with USS, tkey-sign warns about app already loaded
    if result.stderr and 'App already loaded' in result.stderr:
        return 'app'

    # In firmware mode, we get a public key without warnings
    if content:
        return 'firmware'

    return None

def check_tkey_mode_raw(device='/dev/ttyACM0'):
    """Check TKey mode by directly communicating with the device

    Sends a firmware NAME_VERSION command to determine if the TKey is in:
    - firmware mode (responds with 'tk1 mkdf' + version)
    - app mode (responds with error status)

    Args:
        device (str, optional): Serial device path. Defaults to '/dev/ttyACM0'.

    Returns:
        str or None: 'firmware', 'app' if app loaded, None on error
    """
    try:
        # Open serial connection with TKey baud rate
        ser = serial.Serial(device, baudrate=62500, timeout=.5)

        tout.info(f'Opened {device} at 62500 baud')

        # Build frame header: cmd=0, endpoint=2(firmware), status=0, len=1byte
        # Header format: [id:2][endpoint:2][status:1][len:2][reserved:1]
        header = (((TKEY_FRAME_ID_CMD_V1 & 0x3) << 5) |
                  ((TKEY_ENDPOINT_FIRMWARE & 0x3) << 3) |
                  ((TKEY_STATUS_OK & 0x1) << 2) |
                  (TKEY_LENGTH_1_BYTE & 0x3))

        # Frame: [header][command]
        frame = bytes([header, TKEY_FW_CMD_NAME_VERSION])

        tout.info(f'Sending NAME_VERSION: {frame.hex()}')

        # Send command
        ser.write(frame)
        ser.flush()

        # Read response (up to 64 bytes with timeout)
        resp = ser.read(64)
        ser.close()

        tout.info(f'Received ({len(resp)} bytes): {resp.hex()}')

        if not resp:
            tout.info('No response received')
            return None

        # Parse response header
        if len(resp) >= 1:
            hdr = resp[0]
            status_bit = (hdr >> 2) & 0x1

            tout.info(f'Response header: 0x{hdr:02x}, status: {status_bit}')

            if status_bit == 1:
                # Error status - likely app mode responding to firmware cmd
                tout.info('Error status bit set - device in app mode')
                return 'app'
            # Success status - check for firmware mode response pattern
            if len(resp) >= 13:  # Header + 4 + 4 + 4 bytes minimum
                # Look for 'tk1 ' and 'mkdf' in resp
                resp_str = resp[1:].decode('ascii', errors='ignore')
                if 'tk1' in resp_str and 'mkdf' in resp_str:
                    tout.info('Found firmware identifiers')
                    return 'firmware'

            # Got success resp but couldn't parse - assume firmware
            tout.info('Success response - assuming firmware mode')
            return 'firmware'

        return None

    except OSError as e:
        tout.info(f'Error during TKey communication: {e}')
        return None

def get_tkey_pubkey_uss(uss, device=None):
    '''Get public key from TKey using a User Supplied Secret

    Args:
        uss (str): User Supplied Secret (password/passphrase) for key derivation
        device (str, optional): TKey device path. Defaults to None (auto-detect)

    Returns:
        str or None: Public key string on success, None on failure
    '''
    # Check if TKey already has an app loaded first
    # Try direct serial communication first, fall back to tkey-sign
    mode = None
    if not device or device == '/dev/ttyACM0':
        mode = check_tkey_mode_raw('/dev/ttyACM0')

    if mode is None:
        # Fallback to tkey-sign method
        mode = check_tkey_mode(device)

    tout.info(f'TKey mode: {mode}')

    if mode == 'app':
        # App already loaded, need to wait for reinsertion
        wait_tkey_replug()
    else:
        # Show 'Setting up TKey' only when we're about to do the USS operation
        # (not when we need to wait for reinsertion)
        tout.notice('Setting up TKey')

    with tempfile.NamedTemporaryFile(mode='w+', suffix='.pub',
                                     delete=False) as f:
        pubfile = f.name

    # Create temporary file for USS
    with tempfile.NamedTemporaryFile(mode='w', suffix='.uss',
                                     delete=False) as uss_f:
        uss_file = uss_f.name
        uss_f.write(uss)

    args = ['--getkey', '--public', pubfile, '--uss-file', uss_file, '--force']
    if device:
        args.extend(['--port', device])

    result = run_tkey_sign(args)
    if not result or result.returncode:
        err = result.stderr if result else 'Unknown error'
        tout.error(f'Error getting public key with USS: {err}')
        if os.path.exists(pubfile):
            os.unlink(pubfile)
        if os.path.exists(uss_file):
            os.unlink(uss_file)
        return None

    pubkey = tools.read_file(pubfile, binary=False).strip()

    if os.path.exists(pubfile):
        os.unlink(pubfile)
    if os.path.exists(uss_file):
        os.unlink(uss_file)

    return pubkey

def derive_fde_key(uss, device=None):
    """Derive a full-disk encryption key using TKey hardware and USS.

    This uses the TKey's hardware-based key derivation where the USS (User
    Supplied Secret) affects the internal key generation, producing different
keys for different USS values.

    The key derivation matches U-Boot's tkey_derive_disk_key():
    1. Get the 32-byte Ed25519 public key from TKey
    2. Convert to lowercase hex string (64 characters)
    3. SHA256 hash the hex string to produce the disk key

    Args:
        uss (str): User Supplied Secret (password/passphrase) for key derivation
        device (str, optional): TKey device path. Defaults to None (auto-detect)

    Returns:
        bytes or None: 32-byte encryption key material on success, None on
            failure
    """

    # Get the public key using the USS
    # Different USS values produce different private/public key pairs inside the
    # TKey
    pubkey_signify = get_tkey_pubkey_uss(uss, device)
    if not pubkey_signify:
        return None

    tout.info(f'Signify public key:\n{pubkey_signify}')

    # Parse signify format: skip comment line, decode base64 of second line
    # Format: "untrusted comment: ...\n<base64-encoded-data>"
    lines = pubkey_signify.strip().split('\n')
    if len(lines) < 2:
        tout.error('Invalid public key format')
        return None

    # Decode base64 data (second line)
    try:
        pubkey_data = base64.b64decode(lines[1])
    except base64.binascii.Error as e:
        tout.error(f'Error decoding public key: {e}')
        return None

    # Signify format: 2-byte algorithm + 8-byte keynum + 32-byte pubkey
    # Extract the 32-byte Ed25519 public key (last 32 bytes)
    if len(pubkey_data) < 32:
        tout.error(f'Public key data too short ({len(pubkey_data)} bytes)')
        return None

    pubkey_bytes = pubkey_data[-32:]

    tout.info(f'Ed25519 public key (hex): {pubkey_bytes.hex()}')

    # Match U-Boot's tkey_derive_disk_key(): SHA256(hex_string_of_pubkey)
    pubkey_hex = pubkey_bytes.hex()
    key_material = hashlib.sha256(pubkey_hex.encode()).digest()

    tout.info(f'Derived disk key (hex): {key_material.hex()}')

    return key_material

def find_tkey_device():
    """Check if TKey USB device is present

    Looks for TKey device (vendor ID 1207, product ID 8887) via USB enumeration.

    Returns:
        bool: True if TKey device found, False otherwise
    """
    usb_devices = glob.glob('/sys/bus/usb/devices/*/idVendor')
    for vendor_file in usb_devices:
        try:
            vendor_id = tools.read_file(vendor_file, binary=False).strip()
            if vendor_id == '1207':  # Tillitis vendor ID
                product = vendor_file.replace('idVendor', 'idProduct')
                if os.path.exists(product):
                    product_id = tools.read_file(product, binary=False).strip()
                    if product_id == '8887':  # TKey product ID
                        return True
        except (IOError, OSError):
            continue
    return False

def detect_tkey(device=None):
    """Check if TKey is present at startup and prompt for insertion if needed

    Args:
        device (str, optional): TKey device path. Defaults to None (auto-detect)

    Returns:
        bool: True if TKey is available, False on error
    """
    # Check if specific device path exists
    if device and os.path.exists(device):
        tout.info(f'TKey device found at {device}')
        return True

    # Check for TKey via USB enumeration
    if find_tkey_device():
        tout.info('TKey detected via USB enumeration')
        return True

    # No TKey found - prompt for insertion
    tout.notice('Please insert your TKey...')

    # Wait for TKey to be inserted
    while not find_tkey_device():
        time.sleep(0.5)

    tout.info('TKey detected')

    # Give the device a moment to settle
    time.sleep(1)
    return True

def wait_tkey_replug():
    """Wait for TKey to be removed and then re-inserted"""
    tout.info('Waiting for TKey removal and reinsertion...')

    # Wait for device to be removed
    if find_tkey_device():
        tout.notice('Please remove your TKey...')
        while find_tkey_device():
            time.sleep(0.5)
        tout.info('TKey removed')

    # Wait for device to be inserted
    tout.notice('Please insert your TKey...')
    while not find_tkey_device():
        time.sleep(0.5)

    tout.info('TKey detected')

    tout.notice('Setting up TKey')
    # Give the device a moment to settle
    time.sleep(.2)

def format_key_for_luks(key_material):
    '''Format the key material as hex for LUKS

    Args:
        key_material (bytes): Raw key material bytes

    Returns:
        str: Hexadecimal representation of the key
    '''
    return key_material.hex()

def save_key_to_file(key_material, outfile, force=False):
    """Save the key material to a file

    Args:
        key_material (bytes): Raw key material to save
        outfile (str): Path to output file
        force (bool, optional): Overwrite existing files. Defaults to False.

    Returns:
        bool: True on success, False on failure
    """
    if os.path.exists(outfile) and not force:
        tout.error(f'Output file {outfile} already exists. '
                   'Use --force to overwrite.')
        return False

    tools.write_file(outfile, key_material)

    # Set restrictive permissions
    os.chmod(outfile, 0o600)
    tout.notice(f'Key saved to {outfile}')
    return True

def get_password(args):
    """Get password from user input or file

    Args:
        args (argparse.Namespace): Parsed command line arguments

    Returns:
        str or None: Password string on success, None on failure
    """
    if args.password_file:
        if args.password_file == '-':
            # Read from stdin
            tout.info('Reading password from stdin...')
            password = sys.stdin.readline().rstrip('\n\r')
        else:
            # Read from file
            tout.info(f'Reading password from file: {args.password_file}')
            password = tools.read_file(args.password_file, binary=False).strip()

        if not password:
            tout.error('Empty password not allowed')
            return None
        tout.info(f'Password length: {len(password)}, repr: {repr(password)}')
    else:
        # Interactive input
        password = getpass.getpass('Enter password for key derivation: ')
        if not password:
            tout.error('Empty password not allowed')
            return None

        # Confirm password only for interactive input
        confirm = getpass.getpass('Confirm password: ')
        if password != confirm:
            tout.error('Passwords do not match')
            return None

    return password

def check_broken_luks(disk_path):
    """Check if disk has broken LUKS metadata

    Args:
        disk_path (str): Path to disk image file

    Returns:
        bool: True if broken LUKS metadata detected, False otherwise
    """
    try:
        # Use cryptsetup luksDump to check for broken metadata
        result = subprocess.run(
            ['cryptsetup', 'luksDump', disk_path],
            capture_output=True,
            text=True,
            timeout=30,
            check=False
        )

        if result.returncode:
            # Check if the error specifically mentions broken metadata
            # But exclude common "not a LUKS device" messages
            error_lower = result.stderr.lower()

            # These indicate broken LUKS (partial/corrupted headers)
            broken_hints = ['broken', 'invalid', 'corrupted', 'damaged']

            # These indicate no LUKS at all (normal for fresh files)
            no_luks_hints = ['not a luks device', 'no luks header',
                             'unrecognized']

            # Check for "no LUKS" messages first (these are normal)
            if any(hint in error_lower for hint in no_luks_hints):
                tout.info('No LUKS header detected (normal for fresh disk)')
                return False

            # Check for broken LUKS hints
            if any(hint in error_lower for hint in broken_hints):
                tout.info(f'Detected broken LUKS metadata: {result.stderr}')
                return True

        return False

    except subprocess.TimeoutExpired:
        tout.info('Timeout checking for broken LUKS metadata')
        return False
    except OSError as e:
        tout.info(f'Error checking for broken LUKS: {e}')
        return False

def wipe_luks_header(disk_path):
    """Wipe broken LUKS header from disk

    Args:
        disk_path (str): Path to disk image file

    Returns:
        bool: True on success, False on failure
    """
    tout.info(f'Wiping LUKS header from {disk_path}')

    try:
        # Use dd to zero out the first 32MB (typical LUKS header size)
        result = subprocess.run(
            ['dd', 'if=/dev/zero', f'of={disk_path}', 'bs=1M', 'count=32',
             'conv=notrunc'],
            capture_output=True,
            text=True,
            timeout=60,
            check=True
        )

        if result.returncode:
            tout.error(f'Error wiping LUKS header: {result.stderr}')
            return False

        tout.info('LUKS header wiped successfully')
        if result.stderr:  # dd output goes to stderr
            tout.detail(f'dd output: {result.stderr}')

        return True

    except subprocess.TimeoutExpired:
        tout.error('LUKS header wipe operation timed out')
        return False
    except FileNotFoundError:
        tout.error('dd command not found')
        return False
    except OSError as e:
        tout.error(f'Error during LUKS header wipe: {e}')
        return False

def parse_fdisk_line(line, disk_path):
    """Parse a single fdisk output line into partition info

    Args:
        line (str): A line from fdisk -l output
        disk_path (str): Path to disk image file

    Returns:
        dict or None: Partition info dict, or None if line is not a partition
    """
    if disk_path not in line or not line.strip() or line.startswith('Disk'):
        return None

    parts = line.split()
    if len(parts) < 6 or not parts[0].startswith(disk_path):
        return None

    try:
        partition_num = int(parts[0].replace(disk_path, '').replace('p', ''))

        # Handle bootable flag - if '*' is present, it shifts other fields
        if '*' in parts[1]:
            bootable = True
            start_sector = int(parts[2])
            end_sector = int(parts[3])
            sectors = int(parts[4])
            size = parts[5]
            part_type = ' '.join(parts[7:]) if len(parts) > 7 else 'Unknown'
        else:
            bootable = False
            start_sector = int(parts[1])
            end_sector = int(parts[2])
            sectors = int(parts[3])
            size = parts[4]
            part_type = ' '.join(parts[6:]) if len(parts) > 6 else 'Unknown'

        return {
            'number': partition_num,
            'start': start_sector,
            'end': end_sector,
            'sectors': sectors,
            'size': size,
            'type': part_type,
            'bootable': bootable
        }
    except (ValueError, IndexError):
        return None

def get_disk_parts(disk_path):
    """Get partition information from disk image

    Args:
        disk_path (str): Path to disk image file

    Returns:
        list or None: List of partition info dicts, None on error
    """
    try:
        result = subprocess.run(
            ['fdisk', '-l', disk_path],
            capture_output=True,
            text=True,
            timeout=30,
            check=False
        )

        if result.returncode:
            tout.info(f'Error running fdisk: {result.stderr}')
            return None

        partitions = []
        for line in result.stdout.split('\n'):
            part_info = parse_fdisk_line(line, disk_path)
            if part_info:
                partitions.append(part_info)

        if partitions:
            tout.info(f'Found {len(partitions)} partitions:')
            for p in partitions:
                boot_flag = ' (bootable)' if p['bootable'] else ''
                tout.info(f"  Partition {p['number']}: {p['size']} "
                          f"{p['type']}{boot_flag}")

        return partitions

    except subprocess.TimeoutExpired:
        tout.info('Timeout reading partition table')
        return None
    except FileNotFoundError:
        tout.info('fdisk command not found')
        return None
    except OSError as e:
        tout.info(f'Error reading partition table: {e}')
        return None

def select_partition(disk_path, partitions, args):
    """Select which partition to encrypt

    Args:
        disk_path (str): Path to disk image
        partitions (list): List of partition info dicts
        args (Namespace): Command line arguments

    Returns:
        int or None: Selected partition number, None for whole disk
    """
    # If partition specified on command line, use it
    if args.partition:
        if any(p['number'] == args.partition for p in partitions):
            tout.info(f'Using partition {args.partition} from command line')
            return args.partition
        tout.error(f'Partition {args.partition} not found')
        return False  # Return False to indicate error

    # If no partitions found, encrypt whole disk
    if not partitions:
        tout.info('No partitions detected, will encrypt whole disk')
        return None

    # Show partition table and prompt for selection
    tout.notice(f'Disk {disk_path} contains partitions:')
    for p in partitions:
        boot_flag = ' (bootable)' if p['bootable'] else ''
        tout.notice(f"  {p['number']}: {p['size']} {p['type']}{boot_flag}")

    tout.notice('  0: Encrypt whole disk')

    while True:
        try:
            resp = input('Select partition to encrypt (0 for whole disk): ')
            choice = int(resp)

            if not choice:
                return None  # Whole disk
            if any(p['number'] == choice for p in partitions):
                return choice
            nums = [p["number"] for p in partitions]
            tout.notice(f'Invalid choice. Please select 0 or one of: {nums}')
        except (ValueError, KeyboardInterrupt):
            tout.notice('\nOperation cancelled')
            return None

def setup_loop_dev(disk_path):
    """Set up loop device for disk image with partition support

    Args:
        disk_path (str): Path to disk image

    Returns:
        str or None: Loop device path on success, None on failure
    """
    try:
        # First, ensure sudo credentials are cached with an interactive command
        if os.geteuid() != 0:
            tout.info('Requesting sudo privileges for loop device operations...')
            result = subprocess.run(['sudo', '-v'], timeout=30, check=False)
            if result.returncode:
                tout.error('Could not obtain sudo privileges')
                return None
        # Find available loop device
        result = run_sudo(
            ['losetup', '--find', '--show', '--partscan', disk_path],
            timeout=30,
            capture=True  # We need the output for the loop device path
        )

        if result.returncode:
            tout.error(f'Error setting up loop device: {result.stderr}')
            return None

        loop_device = result.stdout.strip()

        tout.info(f'Set up loop device {loop_device} for {disk_path}')

        # Force partition scan
        run_sudo(['partprobe', loop_device], timeout=10)

        return loop_device

    except subprocess.TimeoutExpired:
        tout.error('Loop device setup timed out')
        return None
    except OSError as e:
        tout.error(f'Error setting up loop device: {e}')
        return None

def cleanup_loop_dev(loop_device):
    """Clean up loop device

    Args:
        loop_device (str): Loop device path (e.g., /dev/loop0)

    Returns:
        bool: True on success, False on failure
    """
    try:
        result = run_sudo(
            ['losetup', '--detach', loop_device],
            timeout=30
        )

        if result.returncode:
            tout.error(f'Error cleaning up loop device: {result.stderr}')
            return False

        tout.info(f'Cleaned up loop device {loop_device}')

        return True

    except subprocess.TimeoutExpired:
        tout.error('Loop device cleanup timed out')
        return False
    except OSError as e:
        tout.error(f'Error cleaning up loop device: {e}')
        return False

def get_part_dev(loop_device, partition_num):
    """Get the device path for a specific partition

    Args:
        loop_device (str): Loop device path (e.g., /dev/loop0)
        partition_num (int): Partition number

    Returns:
        str: Partition device path (e.g., /dev/loop0p2)
    """
    return f'{loop_device}p{partition_num}'

def check_disk_image(disk_path):
    """Check disk image file and determine if it needs space for LUKS header

    Args:
        disk_path (str): Path to disk image file

    Returns:
        SimpleNamespace or None: Namespace with disk info on success, None on
            failure. Contains: size, needs_resize, luks_hdrsize,
            already_encrypted
    """
    if not os.path.exists(disk_path):
        tout.error(f'Disk image {disk_path} does not exist')
        return None

    if not os.path.isfile(disk_path):
        tout.error(f'{disk_path} is not a regular file')
        return None

    # Get current file size
    stat_info = os.stat(disk_path)
    current_size = stat_info.st_size

    size_mb = current_size / (1024 * 1024)
    tout.info(f'Disk image size: {current_size} bytes ({size_mb:.1f} MB)')

    # LUKS header is typically 16 MB, but we'll use 32 MB for safety
    luks_hdrsize = 32 * 1024 * 1024  # 32 MB

    # Check if disk is already LUKS encrypted
    # Read first few bytes to check for LUKS signature
    try:
        with open(disk_path, 'rb') as f:
            header = f.read(16)
            if header.startswith(b'LUKS'):
                tout.info('Disk image appears to already be LUKS encrypted')
                return SimpleNamespace(
                    size=current_size,
                    needs_resize=False,
                    luks_hdrsize=0,
                    already_encrypted=True,
                    broken_luks=False
                )
    except IOError as e:
        tout.error(f'Error reading disk image: {e}')
        return None

    # Check for broken LUKS metadata using cryptsetup
    broken_luks = check_broken_luks(disk_path)
    if broken_luks:
        return SimpleNamespace(
            size=current_size,
            needs_resize=False,
            luks_hdrsize=0,
            already_encrypted=False,
            broken_luks=True
        )

    # For unencrypted disks, we need to add space for LUKS header
    return SimpleNamespace(
        size=current_size,
        needs_resize=True,
        luks_hdrsize=luks_hdrsize,
        already_encrypted=False,
        broken_luks=False
    )

def resize_disk(disk_path, additional_size):
    """Resize disk image to add space for LUKS header

    Args:
        disk_path (str): Path to disk image file
        additional_size (int): Additional bytes to add to the image

    Returns:
        bool: True on success, False on failure
    """
    add_mb = additional_size / (1024 * 1024)
    tout.info(f'Resizing disk image to add {additional_size} bytes '
              f'({add_mb:.1f} MB)')

    try:
        # Use truncate to extend the file
        # This is safer than dd as it doesn't actually write the data
        current_size = os.path.getsize(disk_path)
        new_size = current_size + additional_size

        tout.info(f'Extending from {current_size} to {new_size} bytes')

        # Use truncate command which is available on most systems
        result = subprocess.run(
            ['truncate', '--size', str(new_size), disk_path],
            capture_output=True,
            text=True,
            timeout=30,
            check=True
        )

        if result.returncode:
            tout.error(f'Error resizing disk image: {result.stderr}')
            return False

        tout.info('Disk image resized successfully')

        return True

    except subprocess.TimeoutExpired:
        tout.error('Disk resize operation timed out')
        return False
    except OSError:
        return False

def backup_part_data(devpath, orig_mount, backup_dir):
    """Mount partition read-only and backup all data

    Args:
        devpath (str): Path to partition device
        orig_mount (str): Mount point for original partition
        backup_dir (str): Directory to backup data to

    Returns:
        bool: True on success, False on failure
    """
    tout.info(f'Mounting original partition at {orig_mount}')

    result = run_sudo(['mount', '-o', 'ro', devpath, orig_mount],
                      timeout=30)
    if result.returncode:
        tout.error(f'Error mounting original partition: {result.stderr}')
        return False

    tout.info(f'Backing up data to {backup_dir}')

    result = run_sudo(['cp', '-a', f'{orig_mount}/.', backup_dir], timeout=300)
    if result.returncode:
        tout.error(f'Error backing up data: {result.stderr}')
        run_sudo(['umount', orig_mount], timeout=30)
        return False

    tout.info('Data backup completed successfully')

    result = run_sudo(['du', '-sb', backup_dir], timeout=30)
    if not result.returncode:
        backup_size = int(result.stdout.split()[0])
        size_mb = backup_size / (1024 * 1024)
        tout.info(f'Backed up {backup_size} bytes ({size_mb:.1f} MB)')

    run_sudo(['umount', orig_mount], timeout=30)
    return True


def format_luks_part(devpath, keyfile):
    """Format a partition with LUKS2

    Args:
        devpath (str): Path to partition device
        keyfile (str): Path to key file

    Returns:
        bool: True on success, False on failure
    """
    tout.info(f'Creating LUKS partition on {devpath}')

    cmd = [
        'cryptsetup', 'luksFormat',
        '--type', 'luks2',
        '--cipher', 'aes-xts-plain64',
        '--key-size', '512',
        '--hash', 'sha256',
        '--use-random',
        '--key-file', keyfile,
        '--batch-mode',
        devpath
    ]

    result = run_sudo(cmd, timeout=120)
    if result.returncode:
        tout.error(f'Error formatting partition with LUKS: {result.stderr}')
        return False
    return True


def restore_to_luks(dev_path, keyfile, backup_dir, enc_mount):
    """Open LUKS, create filesystem, and restore backed up data

    Args:
        dev_path (str): Path to LUKS partition device
        keyfile (str): Path to key file
        backup_dir (str): Directory containing backed up data
        enc_mount (str): Mount point for encrypted partition

    Returns:
        bool: True on success, False on failure
    """
    mapper = f'tkey-temp-{os.getpid()}'
    tout.info(f'Opening LUKS partition as {mapper}')

    result = run_sudo(['cryptsetup', 'open', '--key-file', keyfile, dev_path,
                       mapper], timeout=30)
    if result.returncode:
        tout.error(f'Error opening LUKS partition: {result.stderr}')
        return False

    try:
        mapper_dev = f'/dev/mapper/{mapper}'
        tout.info(f'Creating ext4 filesystem on {mapper_dev}')

        result = run_sudo(['mkfs.ext4', '-F', mapper_dev], timeout=60)
        if result.returncode:
            tout.error(f'Error creating filesystem: {result.stderr}')
            return False

        tout.info(f'Mounting encrypted partition at {enc_mount}')
        result = run_sudo(['mount', mapper_dev, enc_mount], timeout=30)
        if result.returncode:
            tout.error(f'Error mounting encrypted partition: {result.stderr}')
            return False

        try:
            tout.info('Copying backed up data to encrypted partition')
            result = run_sudo(['cp', '-a', f'{backup_dir}/.', enc_mount],
                              timeout=300)
            if result.returncode:
                tout.error(f'Error copying data: {result.stderr}')
                return False

            run_sudo(['sync'], timeout=30)
            tout.info('Data successfully copied to encrypted partition')
            return True
        finally:
            run_sudo(['umount', enc_mount], timeout=30)

    finally:
        run_sudo(['cryptsetup', 'close', mapper], timeout=30)


def encrypt_part_luks_copy(dev_path, key_material):
    """Encrypt a partition with LUKS and copy existing data

    Args:
        dev_path (str): Path to partition device (e.g., /dev/loop0p2)
        key_material (bytes): Raw key material for encryption

    Returns:
        bool: True on success, False on failure
    """
    tout.info(f'Encrypting partition {dev_path} with data preservation')

    with tempfile.TemporaryDirectory() as temp_dir:
        orig_mount = os.path.join(temp_dir, 'original')
        enc_mount = os.path.join(temp_dir, 'encrypted')
        backup_dir = os.path.join(temp_dir, 'backup')
        os.makedirs(orig_mount)
        os.makedirs(enc_mount)
        os.makedirs(backup_dir)

        with tempfile.NamedTemporaryFile(mode='wb', suffix='.key',
                                         delete=False) as key_f:
            keyfile = key_f.name
            key_f.write(key_material)

        try:
            os.chmod(keyfile, 0o600)

            if not backup_part_data(dev_path, orig_mount, backup_dir):
                return False

            if not format_luks_part(dev_path, keyfile):
                return False

            if not restore_to_luks(dev_path, keyfile, backup_dir, enc_mount):
                return False

            tout.info(f'LUKS partition created successfully on {dev_path}')
            return True

        finally:
            try:
                os.unlink(keyfile)
            except OSError:
                pass

def encrypt_part_luks(devpath, key_material):
    """Encrypt a partition with LUKS, destroying existing filesystem
    Args:
        devpath (str): Path to partition device (e.g., /dev/loop0p2)
        key_material (bytes): Raw key material for encryption
    Returns:
        bool: True on success, False on failure
    """
    tout.info(f'Formatting partition {devpath} with LUKS')

    # Create temporary keyfile
    with tempfile.NamedTemporaryFile(mode='wb', suffix='.key',
                                     delete=False) as key_f:
        keyfile = key_f.name
        key_f.write(key_material)

    try:
        # Set restrictive permissions on keyfile
        os.chmod(keyfile, 0o600)

        # Use luksFormat to create fresh LUKS partition (destroys existing data)
        cmd = [
            'cryptsetup', 'luksFormat',
            '--type', 'luks2',
            '--cipher', 'aes-xts-plain64',
            '--key-size', '512',
            '--hash', 'sha256',
            '--use-random',
            '--key-file', keyfile,
            '--batch-mode',
            devpath
        ]

        result = run_sudo(cmd, timeout=120)
        if result.returncode:
            tout.error(f'Error formatting partition with LUKS: {result.stderr}')
            return False

        tout.info(f'LUKS partition created successfully on {devpath}')
        return True

    finally:
        # Clean up keyfile
        try:
            os.unlink(keyfile)
        except OSError:
            pass

def create_fs_in_luks(devpath, key_material, fstype='ext4'):
    """Open LUKS partition and create filesystem inside
    Args:
        devpath (str): Path to LUKS partition device
        key_material (bytes): Key material to open LUKS partition
        fstype (str): Type of filesystem to create (ext4, ext3, etc.)
    Returns:
        bool: True on success, False on failure
    """
    # Create temporary keyfile
    with tempfile.NamedTemporaryFile(mode='wb', suffix='.key',
                                     delete=False) as key_f:
        keyfile = key_f.name
        key_f.write(key_material)

    try:
        # Set restrictive permissions on keyfile
        os.chmod(keyfile, 0o600)

        # Generate unique mapper name
        name = f'tkey-fs-{os.getpid()}'

        # Open LUKS partition
        cmd = ['cryptsetup', 'open', '--key-file', keyfile, devpath, name]

        result = run_sudo(cmd, timeout=30)
        if result.returncode:
            tout.error(f'Error opening LUKS partition: {result.stderr}')
            return False

        try:
            # Create filesystem
            mapper_device = f'/dev/mapper/{name}'

            if fstype == 'ext4':
                fs_cmd = ['mkfs.ext4', '-F', mapper_device]
            elif fstype == 'ext3':
                fs_cmd = ['mkfs.ext3', '-F', mapper_device]
            else:
                tout.error(f'Unsupported filesystem type: {fstype}')
                return False

            result = run_sudo(fs_cmd, timeout=60)
            if result.returncode:
                tout.error(f'Error creating {fstype} filesystem: '
                           f'{result.stderr}')
                return False

            tout.info(f'{fstype} filesystem created in LUKS partition')
            return True  # Success path for inner operation
        finally:
            # Close LUKS partition
            close_cmd = ['cryptsetup', 'close', name]
            run_sudo(close_cmd, timeout=10)  # close mapper name

    except (subprocess.SubprocessError, OSError) as e:
        tout.error(f'Error during filesystem creation: {e}')
        return False

    finally:
        # Clean up keyfile
        if os.path.exists(keyfile):
            try:
                os.unlink(keyfile)
            except OSError:  # More specific exception for file operations
                pass

def encrypt_disk_luks(disk_path, key_material):
    """Encrypt disk image with LUKS using cryptsetup reencrypt

    Args:
        disk_path (str): Path to disk image file
        key_material (bytes): Raw key material for encryption

    Returns:
        bool: True on success, False on failure
    """
    tout.info(f'Encrypting disk image {disk_path} with LUKS')

    # Create temporary keyfile
    with tempfile.NamedTemporaryFile(mode='wb', suffix='.key',
                                     delete=False) as key_f:
        keyfile = key_f.name
        key_f.write(key_material)

    try:
        # Set restrictive permissions on keyfile
        os.chmod(keyfile, 0o600)

        # Assume fresh disk (no existing LUKS) for partition encryption
        is_luks = False

        if is_luks:
            # Existing LUKS - use reencrypt
            cmd = [
                'cryptsetup', 'reencrypt',
                '--encrypt',
                '--type', 'luks2',
                '--cipher', 'aes-xts-plain64',
                '--key-size', '512',
                '--hash', 'sha256',
                '--use-random',
                '--key-file', keyfile,
                '--batch-mode',
                disk_path
            ]

        else:
            # Fresh disk - use detached header to avoid corrupting filesystem
            header_file = f'{disk_path}.luks'
            cmd = [
                'cryptsetup', 'reencrypt',
                '--encrypt',
                '--type', 'luks2',
                '--cipher', 'aes-xts-plain64',
                '--key-size', '512',
                '--hash', 'sha256',
                '--use-random',
                '--key-file', keyfile,
                '--header', header_file,
                '--batch-mode',
                disk_path
            ]


        result = run_sudo(
            cmd,
            timeout=3600  # 1 hour timeout for large disks
        )

        if result.returncode:
            tout.error(f'Error encrypting disk with LUKS: {result.stderr}')
            return False

        tout.info('Disk encryption completed successfully')
        if result.stdout:
            tout.detail(f'cryptsetup output: {result.stdout}')

        return True

    except subprocess.TimeoutExpired:
        tout.error('Disk encryption operation timed out')
        return False
    except FileNotFoundError:
        tout.error('cryptsetup command not found. Please install cryptsetup.')
        return False
    except OSError as e:
        tout.error(f'Error during disk encryption: {e}')
        return False
    finally:
        # Clean up keyfile
        if os.path.exists(keyfile):
            os.unlink(keyfile)

def check_mapper_status(mapper_name):
    """Check if a device mapper name is already in use

    Args:
        mapper_name (str): Device mapper name to check

    Returns:
        bool: True if mapper is active, False otherwise
    """
    mapper_path = f'/dev/mapper/{mapper_name}'

    if os.path.exists(mapper_path):
        tout.info(f'Device mapper {mapper_name} is already active at '
                  f'{mapper_path}')
        return True

    # Also check via dmsetup
    try:
        result = run_sudo(
            ['dmsetup', 'info', mapper_name],
            timeout=10
        )

        if not result.returncode:
            tout.info(f'Device mapper {mapper_name} is active (dmsetup)')
            return True

    except (subprocess.TimeoutExpired, FileNotFoundError):
        pass

    return False

def open_luks_disk(disk_path, mapper_name, key_material):
    """Open LUKS encrypted disk using cryptsetup

    Args:
        disk_path (str): Path to LUKS encrypted disk image
        mapper_name (str): Device mapper name for the opened disk
        key_material (bytes): Raw key material for decryption

    Returns:
        str or None: Path to opened device (/dev/mapper/name) on success, None
        on failure
    """
    tout.info(f'Opening LUKS disk {disk_path} as {mapper_name}')

    # Check if already opened
    if check_mapper_status(mapper_name):
        mapper_path = f'/dev/mapper/{mapper_name}'
        tout.notice(f'Disk is already opened at {mapper_path}')
        return mapper_path

    # Check if disk is LUKS encrypted or has detached header
    header_file = f'{disk_path}.luks'
    has_detached_header = os.path.exists(header_file)

    if not has_detached_header:
        try:
            with open(disk_path, 'rb') as f:
                header = f.read(16)
                if not header.startswith(b'LUKS'):
                    tout.error(f'{disk_path} is not LUKS encrypted')
                    return None
        except IOError as e:
            tout.error(f'Error reading disk image: {e}')
            return None

    # Create temporary keyfile
    with tempfile.NamedTemporaryFile(mode='wb', suffix='.key',
                                     delete=False) as key_f:
        keyfile = key_f.name
        key_f.write(key_material)

    try:
        # Set restrictive permissions on keyfile
        os.chmod(keyfile, 0o600)

        # Use cryptsetup open (luksOpen) - requires root privileges
        cmd = ['cryptsetup', 'open', '--type', 'luks', '--key-file', keyfile]

        # Add detached header if it exists
        if has_detached_header:
            cmd.extend(['--header', header_file])

        cmd.extend([disk_path, mapper_name])

        result = run_sudo(cmd, timeout=60)
        if result.returncode:
            tout.error(f'Error opening LUKS disk: {result.stderr}')
            return None

        mapper_path = f'/dev/mapper/{mapper_name}'

        # Verify the device was created
        if not os.path.exists(mapper_path):
            tout.error(f'Device {mapper_path} was not created')
            return None

        tout.info(f'LUKS disk opened successfully at {mapper_path}')
        if result.stdout:
            tout.detail(f'cryptsetup output: {result.stdout}')

        return mapper_path

    except subprocess.TimeoutExpired:
        tout.error('Disk opening operation timed out')
    except FileNotFoundError:
        tout.error('cryptsetup command not found. Please install cryptsetup.')
    except OSError as e:
        tout.error(f'Error during disk opening: {e}')
    finally:
        # Clean up keyfile
        if os.path.exists(keyfile):
            os.unlink(keyfile)
    return None

def output_key(key_material, args):
    """Output the derived key material

    Args:
        key_material (bytes): Raw key material to output
        args (argparse.Namespace): Parsed command line arguments

    Returns:
        bool: True on success, False on failure
    """
    if args.output:
        if save_key_to_file(key_material, args.output, args.force):
            tout.notice(f'Encryption key derived and saved to {args.output}')
            if not args.binary:
                tout.notice(f'Key (hex): {format_key_for_luks(key_material)}')
            return True
        return False
    if args.binary:
        # Output raw binary to stdout
        sys.stdout.buffer.write(key_material)
    else:
        print(format_key_for_luks(key_material))
    return True

def validate_args(args):
    """Validate command line arguments

    Args:
        args (argparse.Namespace): Parsed command line arguments

    Returns:
        bool: True if arguments are valid, False otherwise
    """
    if args.encrypt_disk and args.binary and not args.output:
        tout.error('--binary to stdout not compatible with --encrypt-disk')
        tout.error('Use --output to save binary key when encrypting')
        return False

    if args.encrypt_disk and args.open_disk:
        tout.error('Cannot encrypt and open disk in same operation')
        return False

    if args.open_disk and args.binary and not args.output:
        tout.error('--binary to stdout not compatible with --open-disk')
        tout.error('Use --output to save binary key when opening')
        return False

    return True

def do_encrypt_disk(args, key):
    """Handle disk encryption

    Args:
        args (Namespace): Command line arguments
        key (bytes): Encryption key material

    Returns:
        bool: True on success, False on failure
    """
    loop_device = None
    try:
        partitions = get_disk_parts(args.encrypt_disk)
        sel_part = select_partition(args.encrypt_disk, partitions, args)
        if sel_part is False:
            return False

        if sel_part is None:
            target_device = args.encrypt_disk
            disk_info = check_disk_image(args.encrypt_disk)
            if not disk_info:
                return False

            if disk_info.already_encrypted:
                tout.warning(f'{args.encrypt_disk} is already LUKS encrypted')
                if input('Continue anyway? (y/N): ').lower() != 'y':
                    tout.notice('Cancelled')
                    return False

            if hasattr(disk_info, 'broken_luks') and disk_info.broken_luks:
                tout.error(f'{args.encrypt_disk} has broken LUKS metadata')
                tout.notice('Previous encryption may have been interrupted.')
                if input('Wipe broken LUKS header? (y/N): ').lower() != 'y':
                    tout.notice('Cancelled')
                    tout.notice('Manual fix: dd if=/dev/zero of=disk.img '
                                'bs=1M count=32 conv=notrunc')
                    return False

                if not wipe_luks_header(args.encrypt_disk):
                    tout.error('Failed to wipe LUKS header')
                    return False

                disk_info = check_disk_image(args.encrypt_disk)
                if not disk_info:
                    return False

            if disk_info.needs_resize and disk_info.luks_hdrsize:
                if not resize_disk(args.encrypt_disk, disk_info.luks_hdrsize):
                    return False
        else:
            tout.notice(f'Setting up loop device for partition {sel_part}...')
            loop_device = setup_loop_dev(args.encrypt_disk)
            if not loop_device:
                return False

            target_device = get_part_dev(loop_device, sel_part)
            time.sleep(1)

            if not os.path.exists(target_device):
                tout.error(f'Partition {target_device} not found')
                tout.info(f'Available devices: {glob.glob(f"{loop_device}*")}')
                return False

            tout.info(f'Will encrypt partition {sel_part} at {target_device}')

        if sel_part:
            if not encrypt_part_luks_copy(target_device, key):
                return False
            tout.notice(f'Partition {sel_part} of {args.encrypt_disk} '
                        'encrypted with LUKS')
        else:
            if not encrypt_disk_luks(target_device, key):
                return False
            tout.notice(f'{args.encrypt_disk} encrypted with LUKS')

        return True
    finally:
        if loop_device:
            cleanup_loop_dev(loop_device)

def do_open_disk(args, key):
    """Handle disk opening

    Args:
        args (Namespace): Command line arguments
        key (bytes): Encryption key material

    Returns:
        bool: True on success, False on failure
    """
    loop_device = None
    try:
        partitions = get_disk_parts(args.open_disk)

        if partitions:
            tout.notice('Checking partitions for LUKS...')
            loop_device = setup_loop_dev(args.open_disk)
            if not loop_device:
                return False

            enc_part = None
            for part in partitions:
                part_dev = get_part_dev(loop_device, part['number'])
                time.sleep(1)

                if os.path.exists(part_dev):
                    try:
                        with tempfile.NamedTemporaryFile() as temp_f:
                            cmd = ['dd', f'if={part_dev}', f'of={temp_f.name}',
                                   'bs=16', 'count=1']
                            result = run_sudo(cmd, timeout=10)
                            if not result.returncode:
                                temp_f.seek(0)
                                if temp_f.read(16).startswith(b'LUKS'):
                                    enc_part = part
                                    target_device = part_dev
                                    tout.info(f'Found LUKS partition '
                                              f'{part["number"]}')
                                    break
                    except (OSError, IOError) as e:
                        tout.info(f'Error checking partition '
                                  f'{part["number"]}: {e}')

            if not enc_part:
                tout.error('No LUKS partitions found')
                return False

            mapper_path = open_luks_disk(target_device, args.mapper_name, key)
            if not mapper_path:
                return False

            tout.notice(f'LUKS partition {enc_part["number"]} opened at '
                        f'{mapper_path}')
        else:
            mapper_path = open_luks_disk(args.open_disk, args.mapper_name, key)
            if not mapper_path:
                return False
            tout.notice(f'LUKS disk opened at {mapper_path}')

        tout.notice(f'Mount with: sudo mount {mapper_path} /mnt')
        tout.notice(f'Close with: sudo cryptsetup close {args.mapper_name}')
        return True
    finally:
        if loop_device:
            tout.info(f'Loop device {loop_device} left active for LUKS access')

def main():
    """Main function

    Returns:
        int: Exit code (0 for success, 1 for failure)
    """
    args = None
    try:
        args = parse_args()

        if args.debug:
            tout.init(tout.DEBUG)
        elif args.verbose:
            tout.init(tout.INFO)
        else:
            tout.init(tout.NOTICE)

        if not validate_args(args):
            return 1

        password = get_password(args)
        if not password:
            return 1

        if not detect_tkey(args.device):
            tout.error('Failed to detect TKey')
            return 1

        key = derive_fde_key(password, args.device)
        if not key:
            tout.error('Failed to derive encryption key')
            return 1

        password = None

        if args.encrypt_disk:
            if not do_encrypt_disk(args, key):
                return 1
            tout.notice('Disk encryption completed successfully')
        elif args.open_disk:
            if not do_open_disk(args, key):
                return 1
            tout.notice('Disk opening completed successfully')
        else:
            if not output_key(key, args):
                return 1
            tout.notice('Key derivation completed successfully')

        if args.output:
            if not output_key(key, args):
                return 1
        return 0

    except KeyboardInterrupt:
        tout.notice('\nCancelled')
        return 1
    except subprocess.TimeoutExpired:
        tout.error('Operation timed out')
        return 1
    except FileNotFoundError as e:
        if 'tkey-sign' in str(e):
            tout.error('tkey-sign not found')
            if os.environ.get('SUDO_USER') and not os.geteuid():
                tout.notice('Note: tkey-sign may not be in root PATH.')
                tout.notice('Try: sudo env PATH=$PATH ./scripts/tkey-fde-key.py')
        elif 'cryptsetup' in str(e):
            tout.error('cryptsetup not found. Install cryptsetup.')
        elif 'truncate' in str(e):
            tout.error('truncate not found')
        elif 'dmsetup' in str(e):
            tout.error('dmsetup not found')
        else:
            tout.error(f'File not found - {e}')
        return 1
    except PermissionError as e:
        tout.error(f'Permission denied - {e}')
        tout.notice('Note: Disk operations may require root privileges')
        return 1
    except OSError as e:
        tout.error(f'OS/IO operation failed - {e}')
        return 1
    except Exception as e:  # pylint: disable=broad-exception-caught
        if args and args.debug:
            import traceback  # pylint: disable=import-outside-toplevel
            tout.error('Full traceback:')
            traceback.print_exc()
        else:
            tout.error(f'Unexpected error: {e}')
        return 1


if __name__ == '__main__':
    sys.exit(main())

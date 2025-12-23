# SPDX-License-Identifier: GPL-2.0+
# Copyright 2025 Canonical Ltd
# Written by Simon Glass <simon.glass@canonical.com>
#
# Test for ext4l filesystem driver

"""
Test ext4l filesystem probing via C unit test.
"""

import os
from subprocess import CalledProcessError, check_call
from tempfile import NamedTemporaryFile

import pytest


@pytest.mark.boardspec('sandbox')
class TestExt4l:
    """Test ext4l filesystem operations."""

    @pytest.fixture(scope='class')
    def ext4_image(self, u_boot_config):
        """Create an ext4 filesystem image for testing.

        Args:
            u_boot_config (u_boot_config): U-Boot configuration.

        Yields:
            str: Path to the ext4 image file.
        """
        image_path = os.path.join(u_boot_config.persistent_data_dir,
                                  'ext4l_test.img')
        try:
            # Create a 64MB ext4 image
            check_call(f'dd if=/dev/zero of={image_path} bs=1M count=64 2>/dev/null',
                       shell=True)
            check_call(f'mkfs.ext4 -q {image_path}', shell=True)

            # Add test files using debugfs (no mount required)
            with NamedTemporaryFile(mode='w', delete=False) as tmp:
                tmp.write('hello world\n')
                tmp_path = tmp.name
            try:
                # Add a regular file
                check_call(f'debugfs -w {image_path} '
                           f'-R "write {tmp_path} testfile.txt" 2>/dev/null',
                           shell=True)
                # Add a subdirectory
                check_call(f'debugfs -w {image_path} '
                           f'-R "mkdir subdir" 2>/dev/null',
                           shell=True)
                # Add a file in the subdirectory
                check_call(f'debugfs -w {image_path} '
                           f'-R "write {tmp_path} subdir/nested.txt" 2>/dev/null',
                           shell=True)
                # Add a symlink
                check_call(f'debugfs -w {image_path} '
                           f'-R "symlink link.txt testfile.txt" 2>/dev/null',
                           shell=True)
            finally:
                os.unlink(tmp_path)
        except CalledProcessError:
            pytest.skip('Failed to create ext4 image')

        yield image_path

        # Cleanup
        if os.path.exists(image_path):
            os.remove(image_path)

    def test_probe(self, ubman, ext4_image):
        """Test that ext4l can probe an ext4 filesystem."""
        with ubman.log.section('Test ext4l probe'):
            output = ubman.run_command(
                f'ut -f fs fs_test_ext4l_probe_norun fs_image={ext4_image}')
            assert 'failures: 0' in output

    def test_msgs(self, ubman, ext4_image):
        """Test that ext4l_msgs env var produces mount messages."""
        with ubman.log.section('Test ext4l msgs'):
            output = ubman.run_command(
                f'ut -f fs fs_test_ext4l_msgs_norun fs_image={ext4_image}')
            assert 'failures: 0' in output

    def test_ls(self, ubman, ext4_image):
        """Test that ext4l can list directory contents."""
        with ubman.log.section('Test ext4l ls'):
            output = ubman.run_command(
                f'ut -f fs fs_test_ext4l_ls_norun fs_image={ext4_image}')
            assert 'failures: 0' in output

    def test_opendir(self, ubman, ext4_image):
        """Test that ext4l can iterate directory entries."""
        with ubman.log.section('Test ext4l opendir'):
            output = ubman.run_command(
                f'ut -f fs fs_test_ext4l_opendir_norun fs_image={ext4_image}')
            assert 'failures: 0' in output

    def test_exists(self, ubman, ext4_image):
        """Test that ext4l_exists reports file existence correctly."""
        with ubman.log.section('Test ext4l exists'):
            output = ubman.run_command(
                f'ut -f fs fs_test_ext4l_exists_norun fs_image={ext4_image}')
            assert 'failures: 0' in output

    def test_size(self, ubman, ext4_image):
        """Test that ext4l_size reports file size correctly."""
        with ubman.log.section('Test ext4l size'):
            output = ubman.run_command(
                f'ut -f fs fs_test_ext4l_size_norun fs_image={ext4_image}')
            assert 'failures: 0' in output

    def test_read(self, ubman, ext4_image):
        """Test that ext4l can read file contents."""
        with ubman.log.section('Test ext4l read'):
            output = ubman.run_command(
                f'ut -f fs fs_test_ext4l_read_norun fs_image={ext4_image}')
            assert 'failures: 0' in output

    def test_uuid(self, ubman, ext4_image):
        """Test that ext4l can return the filesystem UUID."""
        with ubman.log.section('Test ext4l uuid'):
            output = ubman.run_command(
                f'ut -f fs fs_test_ext4l_uuid_norun fs_image={ext4_image}')
            assert 'failures: 0' in output

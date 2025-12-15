# SPDX-License-Identifier:      GPL-2.0+
# Copyright (c) 2018, Linaro Limited
# Author: Takahiro Akashi <takahiro.akashi@linaro.org>
#
# U-Boot File System:Basic Test

"""
This test verifies basic read/write operation on file system.

Tests are implemented in C (test/fs/fs_basic.c) and called from here.
Python handles filesystem image setup and environment variable configuration.
"""

import pytest
from fstest_defs import SMALL_FILE, BIG_FILE
from fstest_helpers import assert_fs_integrity


def run_c_test(ubman, fs_type, fs_img, test_name, small=None, big=None,
               md5val=None):
    """Run a C unit test with proper setup.

    Args:
        ubman (ConsoleBase): U-Boot console manager
        fs_type (str): Filesystem type (ext4, fat, fs_generic, exfat)
        fs_img (str): Path to filesystem image
        test_name (str): Name of C test function (without _norun suffix)
        small (str): Filename of small test file (optional)
        big (str): Filename of big test file (optional)
        md5val (str): Expected MD5 value for verification (optional)

    Returns:
        bool: True if test passed, False otherwise
    """
    # Build the command with arguments
    cmd = f'ut -f fs {test_name}_norun fs_type={fs_type} fs_image={fs_img}'
    if small:
        cmd += f' small={small}'
    if big:
        cmd += f' big={big}'
    if md5val:
        cmd += f' md5val={md5val}'

    # Run the C test
    ubman.run_command(cmd)

    # Check result
    result = ubman.run_command('echo $?')
    return result.strip() == '0'


@pytest.mark.boardspec('sandbox')
@pytest.mark.slow
class TestFsBasic:
    """Test basic filesystem operations via C unit tests."""

    def test_fs1(self, ubman, fs_obj_basic):
        """Test Case 1 - ls command, listing root and invalid directories"""
        fs_type, fs_img, _ = fs_obj_basic
        with ubman.log.section('Test Case 1 - ls'):
            assert run_c_test(ubman, fs_type, fs_img, 'fs_test_ls',
                              small=SMALL_FILE, big=BIG_FILE)

    def test_fs2(self, ubman, fs_obj_basic):
        """Test Case 2 - size command for a small file"""
        fs_type, fs_img, _ = fs_obj_basic
        with ubman.log.section('Test Case 2 - size (small)'):
            assert run_c_test(ubman, fs_type, fs_img, 'fs_test_size_small',
                              small=SMALL_FILE)

    def test_fs3(self, ubman, fs_obj_basic):
        """Test Case 3 - size command for a large file"""
        fs_type, fs_img, _ = fs_obj_basic
        with ubman.log.section('Test Case 3 - size (large)'):
            assert run_c_test(ubman, fs_type, fs_img, 'fs_test_size_big',
                              big=BIG_FILE)

    def test_fs4(self, ubman, fs_obj_basic):
        """Test Case 4 - load a small file, 1MB"""
        fs_type, fs_img, md5val = fs_obj_basic
        with ubman.log.section('Test Case 4 - load (small)'):
            assert run_c_test(ubman, fs_type, fs_img, 'fs_test_load_small',
                              small=SMALL_FILE, md5val=md5val[0])

    def test_fs5(self, ubman, fs_obj_basic):
        """Test Case 5 - load, reading first 1MB of 3GB file"""
        fs_type, fs_img, md5val = fs_obj_basic
        with ubman.log.section('Test Case 5 - load (first 1MB)'):
            assert run_c_test(ubman, fs_type, fs_img, 'fs_test_load_big_first',
                              big=BIG_FILE, md5val=md5val[1])

    def test_fs6(self, ubman, fs_obj_basic):
        """Test Case 6 - load, reading last 1MB of 3GB file"""
        fs_type, fs_img, md5val = fs_obj_basic
        with ubman.log.section('Test Case 6 - load (last 1MB)'):
            assert run_c_test(ubman, fs_type, fs_img, 'fs_test_load_big_last',
                              big=BIG_FILE, md5val=md5val[2])

    def test_fs7(self, ubman, fs_obj_basic):
        """Test Case 7 - load, 1MB from the last 1MB in 2GB"""
        fs_type, fs_img, md5val = fs_obj_basic
        with ubman.log.section('Test Case 7 - load (last 1MB in 2GB)'):
            assert run_c_test(ubman, fs_type, fs_img,
                              'fs_test_load_big_2g_last',
                              big=BIG_FILE, md5val=md5val[3])

    def test_fs8(self, ubman, fs_obj_basic):
        """Test Case 8 - load, reading first 1MB in 2GB"""
        fs_type, fs_img, md5val = fs_obj_basic
        with ubman.log.section('Test Case 8 - load (first 1MB in 2GB)'):
            assert run_c_test(ubman, fs_type, fs_img,
                              'fs_test_load_big_2g_first',
                              big=BIG_FILE, md5val=md5val[4])

    def test_fs9(self, ubman, fs_obj_basic):
        """Test Case 9 - load, 1MB crossing 2GB boundary"""
        fs_type, fs_img, md5val = fs_obj_basic
        with ubman.log.section('Test Case 9 - load (crossing 2GB boundary)'):
            assert run_c_test(ubman, fs_type, fs_img,
                              'fs_test_load_big_2g_cross',
                              big=BIG_FILE, md5val=md5val[5])

    def test_fs10(self, ubman, fs_obj_basic):
        """Test Case 10 - load, reading beyond file end"""
        fs_type, fs_img, _ = fs_obj_basic
        with ubman.log.section('Test Case 10 - load (beyond file end)'):
            assert run_c_test(ubman, fs_type, fs_img, 'fs_test_load_beyond',
                              big=BIG_FILE)

    def test_fs11(self, ubman, fs_obj_basic):
        """Test Case 11 - write"""
        fs_type, fs_img, md5val = fs_obj_basic
        with ubman.log.section('Test Case 11 - write'):
            assert run_c_test(ubman, fs_type, fs_img, 'fs_test_write',
                              small=SMALL_FILE, md5val=md5val[0])
            assert_fs_integrity(fs_type, fs_img)

    def test_fs12(self, ubman, fs_obj_basic):
        """Test Case 12 - write to "." directory"""
        fs_type, fs_img, _ = fs_obj_basic
        with ubman.log.section('Test Case 12 - write (".")'):
            assert run_c_test(ubman, fs_type, fs_img, 'fs_test_write_dot')
            assert_fs_integrity(fs_type, fs_img)

    def test_fs13(self, ubman, fs_obj_basic):
        """Test Case 13 - write to a file with '/./<filename>'"""
        fs_type, fs_img, md5val = fs_obj_basic
        with ubman.log.section('Test Case 13 - write  ("./<file>")'):
            assert run_c_test(ubman, fs_type, fs_img, 'fs_test_write_dotpath',
                              small=SMALL_FILE, md5val=md5val[0])
            assert_fs_integrity(fs_type, fs_img)

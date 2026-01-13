#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0+
#
# Copyright 2025 Canonical Ltd
#
"""Unit tests for output.py CSV generation"""

import csv
import os
import shutil
import sys
import tempfile
import unittest
from collections import namedtuple

# Test configuration
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

# Import the module to test
sys.path.insert(0, SCRIPT_DIR)
sys.path.insert(0, os.path.join(SCRIPT_DIR, '..'))
import output  # pylint: disable=wrong-import-position
from u_boot_pylib import tools  # pylint: disable=wrong-import-position


# Mock FileResult for testing
FileResult = namedtuple('FileResult',
                        ['total_lines', 'active_lines', 'inactive_lines'])


class TestGenerateCsv(unittest.TestCase):
    """Test cases for generate_csv function"""

    def setUp(self):
        """Create temporary directory with test files"""
        self.test_dir = tempfile.mkdtemp(prefix='test_output_')

        # Create source files
        self.src_dir = os.path.join(self.test_dir, 'src')
        os.makedirs(os.path.join(self.src_dir, 'boot'))
        os.makedirs(os.path.join(self.src_dir, 'drivers', 'net'))
        os.makedirs(os.path.join(self.src_dir, 'tools', 'codman'))

        # Create test source files with known content
        self.files = {
            'boot/bootm.c': '// boot\n' * 100,
            'boot/image.c': '// image\n' * 50,
            'drivers/net/eth.c': '// eth\n' * 200,
        }
        for path, content in self.files.items():
            full_path = os.path.join(self.src_dir, path)
            tools.write_file(full_path, content, binary=False)

        # Create category.cfg
        cfg_content = '''
[categories.load-boot]
description = "Loading & Boot"

[categories.drivers]
description = "Drivers"

[features.boot-core]
category = "load-boot"
description = "Core boot"
files = ["boot/"]

[features.ethernet]
category = "drivers"
description = "Ethernet"
files = ["drivers/net/"]
'''
        cfg_path = os.path.join(self.src_dir, 'tools', 'codman', 'category.cfg')
        tools.write_file(cfg_path, cfg_content, binary=False)

        self.csv_file = os.path.join(self.test_dir, 'report.csv')

    def tearDown(self):
        """Clean up temporary directory"""
        if os.path.exists(self.test_dir):
            shutil.rmtree(self.test_dir)

    def test_csv_basic(self):
        """Test basic CSV generation"""
        all_sources = {
            os.path.join(self.src_dir, p) for p in self.files
        }
        used_sources = all_sources.copy()

        result = output.generate_csv(
            all_sources, used_sources, None, self.src_dir,
            by_subdirs=True, show_files=True, show_empty=False,
            use_kloc=False, csv_file=self.csv_file)

        self.assertTrue(result)
        self.assertTrue(os.path.exists(self.csv_file))

        # Read and verify CSV content
        data = tools.read_file(self.csv_file, binary=False)
        rows = list(csv.reader(data.splitlines()))

        # Check header
        self.assertEqual(rows[0][0], 'Type')
        self.assertEqual(rows[0][1], 'Path')
        self.assertEqual(rows[0][2], 'Category')
        self.assertEqual(rows[0][3], 'Feature')

    def test_csv_files_only(self):
        """Test CSV generation with files_only option"""
        all_sources = {
            os.path.join(self.src_dir, p) for p in self.files
        }
        used_sources = all_sources.copy()

        result = output.generate_csv(
            all_sources, used_sources, None, self.src_dir,
            by_subdirs=True, show_files=True, show_empty=False,
            use_kloc=False, csv_file=self.csv_file, files_only=True)

        self.assertTrue(result)

        data = tools.read_file(self.csv_file, binary=False)
        rows = list(csv.reader(data.splitlines()))

        # Check simplified header for files_only
        self.assertEqual(rows[0][0], 'Path')
        self.assertEqual(rows[0][1], 'Category')
        self.assertEqual(rows[0][2], 'Feature')
        self.assertEqual(rows[0][3], '%Code')

        # No 'dir' or 'total' rows
        for row in rows[1:]:
            self.assertNotIn(row[0], ['dir', 'total'])

    def test_csv_category_matching(self):
        """Test that files are matched to correct categories"""
        all_sources = {
            os.path.join(self.src_dir, p) for p in self.files
        }
        used_sources = all_sources.copy()

        # Create mock file results
        file_results = {}
        for path, content in self.files.items():
            full_path = os.path.join(self.src_dir, path)
            lines = len(content.split('\n'))
            file_results[full_path] = FileResult(lines, lines, 0)

        result = output.generate_csv(
            all_sources, used_sources, file_results, self.src_dir,
            by_subdirs=True, show_files=True, show_empty=False,
            use_kloc=False, csv_file=self.csv_file, files_only=True)

        self.assertTrue(result)

        data = tools.read_file(self.csv_file, binary=False)
        rows = list(csv.reader(data.splitlines()))

        # Find boot files and verify category
        boot_rows = [r for r in rows[1:] if 'boot/' in r[0]]
        self.assertEqual(len(boot_rows), 2)  # bootm.c and image.c
        for row in boot_rows:
            self.assertEqual(row[1], 'load-boot')
            self.assertEqual(row[2], 'boot-core')

        # Find driver files and verify category
        driver_rows = [r for r in rows[1:] if 'drivers/' in r[0]]
        self.assertEqual(len(driver_rows), 1)  # eth.c
        for row in driver_rows:
            self.assertEqual(row[1], 'drivers')
            self.assertEqual(row[2], 'ethernet')

    def test_csv_with_ignore(self):
        """Test CSV generation with ignored files"""
        # Add ignore section to config
        cfg_path = os.path.join(self.src_dir, 'tools', 'codman', 'category.cfg')
        existing = tools.read_file(cfg_path, binary=False)
        tools.write_file(cfg_path,
                         existing + '\n[ignore]\nfiles = ["drivers/net/"]\n',
                         binary=False)

        all_sources = {
            os.path.join(self.src_dir, p) for p in self.files
        }
        used_sources = all_sources.copy()

        # Create mock file results
        file_results = {}
        for path, content in self.files.items():
            full_path = os.path.join(self.src_dir, path)
            lines = len(content.split('\n'))
            file_results[full_path] = FileResult(lines, lines, 0)

        result = output.generate_csv(
            all_sources, used_sources, file_results, self.src_dir,
            by_subdirs=True, show_files=True, show_empty=False,
            use_kloc=False, csv_file=self.csv_file, files_only=True)

        self.assertTrue(result)

        data = tools.read_file(self.csv_file, binary=False)
        rows = list(csv.reader(data.splitlines()))

        # Verify ignored files are not in output
        paths = [r[0] for r in rows[1:]]
        self.assertFalse(any('drivers/net/' in p for p in paths))

        # Boot files should still be there
        self.assertTrue(any('boot/' in p for p in paths))


if __name__ == '__main__':
    unittest.main()

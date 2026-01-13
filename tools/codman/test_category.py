#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0+
#
# Copyright 2025 Canonical Ltd
#
"""Unit tests for category.py module"""

import os
import shutil
import sys
import tempfile
import unittest

# Test configuration
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

# Import the module to test
sys.path.insert(0, SCRIPT_DIR)
sys.path.insert(0, os.path.join(SCRIPT_DIR, '..'))
import category  # pylint: disable=wrong-import-position
from u_boot_pylib import tools  # pylint: disable=wrong-import-position


class TestMatchFileToFeature(unittest.TestCase):
    """Test cases for get_file_feature function"""

    def test_exact_match(self):
        """Test exact file path matching"""
        features = {
            'test-feature': {
                'category': 'test-cat',
                'files': ['path/to/file.c'],
            }
        }
        feat_id, cat_id = category.get_file_feature(
            'path/to/file.c', features)
        self.assertEqual(feat_id, 'test-feature')
        self.assertEqual(cat_id, 'test-cat')

    def test_glob_pattern(self):
        """Test glob pattern matching"""
        features = {
            'test-feature': {
                'category': 'test-cat',
                'files': ['drivers/video/*.c'],
            }
        }
        feat_id, cat_id = category.get_file_feature(
            'drivers/video/console.c', features)
        self.assertEqual(feat_id, 'test-feature')
        self.assertEqual(cat_id, 'test-cat')

    def test_directory_prefix(self):
        """Test directory prefix matching (pattern ending with /)"""
        features = {
            'efi-loader': {
                'category': 'efi',
                'files': ['lib/efi_loader/'],
            }
        }
        # Should match files directly in directory
        feat_id, cat_id = category.get_file_feature(
            'lib/efi_loader/efi_acpi.c', features)
        self.assertEqual(feat_id, 'efi-loader')
        self.assertEqual(cat_id, 'efi')

        # Should match files in subdirectories
        feat_id, cat_id = category.get_file_feature(
            'lib/efi_loader/subdir/file.c', features)
        self.assertEqual(feat_id, 'efi-loader')
        self.assertEqual(cat_id, 'efi')

    def test_no_match(self):
        """Test when no feature matches"""
        features = {
            'test-feature': {
                'category': 'test-cat',
                'files': ['other/path/*.c'],
            }
        }
        feat_id, cat_id = category.get_file_feature(
            'different/path/file.c', features)
        self.assertIsNone(feat_id)
        self.assertIsNone(cat_id)

    def test_empty_features(self):
        """Test with empty features dict"""
        feat_id, cat_id = category.get_file_feature('any/file.c', {})
        self.assertIsNone(feat_id)
        self.assertIsNone(cat_id)

    def test_feature_without_files(self):
        """Test feature with empty files list"""
        features = {
            'test-feature': {
                'category': 'test-cat',
                'files': [],
            }
        }
        feat_id, cat_id = category.get_file_feature(
            'any/file.c', features)
        self.assertIsNone(feat_id)
        self.assertIsNone(cat_id)

    def test_first_match_wins(self):
        """Test that first matching feature is returned"""
        features = {
            'feature-a': {
                'category': 'cat-a',
                'files': ['lib/'],
            },
            'feature-b': {
                'category': 'cat-b',
                'files': ['lib/specific.c'],
            }
        }
        # Order depends on dict iteration, but one should match
        feat_id, _ = category.get_file_feature(
            'lib/specific.c', features)
        self.assertIsNotNone(feat_id)
        self.assertIn(feat_id, ['feature-a', 'feature-b'])


class TestLoadCategoryConfig(unittest.TestCase):
    """Test cases for load_category_config functions"""

    def setUp(self):
        """Create temporary directory for test files"""
        self.test_dir = tempfile.mkdtemp(prefix='test_category_')

    def tearDown(self):
        """Clean up temporary directory"""
        if os.path.exists(self.test_dir):
            shutil.rmtree(self.test_dir)

    def test_load_valid_config(self):
        """Test loading a valid TOML config file"""
        cfg_content = '''
[categories.load-boot]
description = "Loading & Boot"

[features.boot-direct]
category = "load-boot"
description = "Direct boot"
files = ["boot/bootm.c"]
'''
        cfg_path = os.path.join(self.test_dir, 'category.cfg')
        tools.write_file(cfg_path, cfg_content, binary=False)

        result = category.load_config_file(cfg_path)

        self.assertIsNotNone(result)
        self.assertIn('load-boot', result.categories)
        self.assertEqual(result.categories['load-boot']['description'],
                         'Loading & Boot')
        self.assertIn('boot-direct', result.features)
        self.assertEqual(result.features['boot-direct']['category'],
                         'load-boot')

    def test_load_missing_file(self):
        """Test loading from non-existent file"""
        cfg_path = os.path.join(self.test_dir, 'nonexistent.cfg')
        result = category.load_config_file(cfg_path)
        self.assertIsNone(result)

    def test_load_invalid_toml(self):
        """Test loading invalid TOML file"""
        cfg_path = os.path.join(self.test_dir, 'invalid.cfg')
        tools.write_file(cfg_path, 'this is not valid TOML [[[', binary=False)
        result = category.load_config_file(cfg_path)
        self.assertIsNone(result)

    def test_load_from_srcdir(self):
        """Test load_category_config with srcdir parameter"""
        # Create tools/codman directory structure
        codman_dir = os.path.join(self.test_dir, 'tools', 'codman')
        os.makedirs(codman_dir)

        cfg_content = '''
[categories.test]
description = "Test category"

[features.test-feat]
category = "test"
description = "Test feature"
files = []
'''
        cfg_path = os.path.join(codman_dir, 'category.cfg')
        tools.write_file(cfg_path, cfg_content, binary=False)

        result = category.load_category_config(self.test_dir)

        self.assertIsNotNone(result)
        self.assertIn('test', result.categories)


class TestShouldIgnoreFile(unittest.TestCase):
    """Test cases for should_ignore_file function"""

    def test_ignore_directory_prefix(self):
        """Test ignoring files by directory prefix"""
        ignore = ['lib/external/']
        self.assertTrue(category.should_ignore_file(
            'lib/external/foo.c', ignore))
        self.assertTrue(category.should_ignore_file(
            'lib/external/sub/bar.c', ignore))
        self.assertFalse(category.should_ignore_file(
            'lib/internal/foo.c', ignore))

    def test_ignore_exact_path(self):
        """Test ignoring files by exact path"""
        ignore = ['lib/external/specific.c']
        self.assertTrue(category.should_ignore_file(
            'lib/external/specific.c', ignore))
        self.assertFalse(category.should_ignore_file(
            'lib/external/other.c', ignore))

    def test_ignore_glob_pattern(self):
        """Test ignoring files by glob pattern"""
        ignore = ['lib/external/*.c']
        self.assertTrue(category.should_ignore_file(
            'lib/external/foo.c', ignore))
        self.assertFalse(category.should_ignore_file(
            'lib/external/foo.h', ignore))

    def test_empty_ignore_list(self):
        """Test with empty ignore list"""
        self.assertFalse(category.should_ignore_file('any/file.c', []))
        self.assertFalse(category.should_ignore_file('any/file.c', None))

    def test_multiple_ignore_patterns(self):
        """Test with multiple ignore patterns"""
        ignore = ['lib/external/', 'vendor/*.c']
        self.assertTrue(category.should_ignore_file(
            'lib/external/foo.c', ignore))
        self.assertTrue(category.should_ignore_file(
            'vendor/bar.c', ignore))
        self.assertFalse(category.should_ignore_file(
            'src/main.c', ignore))


class TestHelperFunctions(unittest.TestCase):
    """Test cases for helper functions"""

    def test_get_category_desc(self):
        """Test get_category_desc function"""
        categories = {
            'load-boot': {'description': 'Loading & Boot'},
            'storage': {'description': 'Storage'},
        }
        desc = category.get_category_desc(categories, 'load-boot')
        self.assertEqual(desc, 'Loading & Boot')

        desc = category.get_category_desc(categories, 'nonexistent')
        self.assertIsNone(desc)

        desc = category.get_category_desc(None, 'load-boot')
        self.assertIsNone(desc)

    def test_get_feature_desc(self):
        """Test get_feature_desc function"""
        features = {
            'boot-direct': {
                'description': 'Direct boot',
                'category': 'load-boot',
            },
        }
        desc = category.get_feature_desc(features, 'boot-direct')
        self.assertEqual(desc, 'Direct boot')

        desc = category.get_feature_desc(features, 'nonexistent')
        self.assertIsNone(desc)

        desc = category.get_feature_desc(None, 'boot-direct')
        self.assertIsNone(desc)


if __name__ == '__main__':
    unittest.main()

# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Canonical Ltd
#
"""Category and feature management for codman.

This module provides functions for loading category configuration and
matching source files to features/categories.
"""

from collections import namedtuple
import fnmatch
import os

try:
    import tomllib
except ImportError:
    import tomli as tomllib

from u_boot_pylib import tools

# Return type for load_category_config functions
CategoryConfig = namedtuple('CategoryConfig',
                            ['categories', 'features', 'ignore'])


def load_category_config(srcdir):
    """Load category configuration from category.cfg.

    Args:
        srcdir (str): Root directory of the source tree

    Returns:
        CategoryConfig: namedtuple with (categories, features, ignore) or
                        None if not found
    """
    cfg_path = os.path.join(srcdir, 'tools', 'codman', 'category.cfg')
    if not os.path.exists(cfg_path):
        return None

    try:
        data = tools.read_file(cfg_path, binary=False)
        config = tomllib.loads(data)
        ignore = config.get('ignore', {}).get('files', [])
        return CategoryConfig(config.get('categories', {}),
                              config.get('features', {}), ignore)
    except (IOError, tomllib.TOMLDecodeError):
        return None


def load_config_file(cfg_path):
    """Load category configuration from a specific file path.

    Args:
        cfg_path (str): Path to the category configuration file

    Returns:
        CategoryConfig: namedtuple with (categories, features, ignore) or
                        None if not found
    """
    if not os.path.exists(cfg_path):
        return None

    try:
        data = tools.read_file(cfg_path, binary=False)
        config = tomllib.loads(data)
        ignore = config.get('ignore', {}).get('files', [])
        return CategoryConfig(config.get('categories', {}),
                              config.get('features', {}), ignore)
    except (IOError, tomllib.TOMLDecodeError):
        return None


def should_ignore_file(filepath, ignore_patterns):
    """Check if a file should be ignored based on ignore patterns.

    Args:
        filepath (str): Relative file path to check
        ignore_patterns (list): List of patterns to ignore

    Returns:
        bool: True if file should be ignored
    """
    if not ignore_patterns:
        return False

    for pattern in ignore_patterns:
        # Directory prefix: pattern ending with '/' matches all files under it
        if pattern.endswith('/'):
            if filepath.startswith(pattern):
                return True
        # Exact match or glob pattern
        elif fnmatch.fnmatch(filepath, pattern) or filepath == pattern:
            return True
    return False


def get_file_feature(filepath, features):
    """Match a file path to a feature based on the feature's file patterns.

    Args:
        filepath (str): Relative file path to match
        features (dict): Features dict from category config

    Returns:
        tuple: (feature_id, category_id) or (None, None) if no match
    """
    for feat_id, feat_data in features.items():
        for pattern in feat_data.get('files', []):
            # Directory prefix: pattern ending with '/' matches all under it
            if pattern.endswith('/'):
                if filepath.startswith(pattern):
                    return feat_id, feat_data.get('category')
            # Exact match or glob pattern
            elif fnmatch.fnmatch(filepath, pattern) or filepath == pattern:
                return feat_id, feat_data.get('category')
    return None, None


def get_category_desc(categories, category_id):
    """Get the description for a category.

    Args:
        categories (dict): Categories dict from category config
        category_id (str): Category identifier

    Returns:
        str: Category description or None if not found
    """
    if categories and category_id in categories:
        return categories[category_id].get('description')
    return None


def get_feature_desc(features, feature_id):
    """Get the description for a feature.

    Args:
        features (dict): Features dict from category config
        feature_id (str): Feature identifier

    Returns:
        str: Feature description or None if not found
    """
    if features and feature_id in features:
        return features[feature_id].get('description')
    return None

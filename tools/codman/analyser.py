# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Canonical Ltd
#
"""Base classes for source code analysis.

This module provides base classes and data structures for analyzing which lines
in source files are active vs inactive.
"""

import os
from collections import namedtuple

# Named tuple for file analysis results
# Fields:
#   total_lines: Total number of lines in the file
#   active_lines: Number of lines that are active (not removed by
#       preprocessor)
#   inactive_lines: Number of lines that are inactive (removed by
#       preprocessor)
#   line_status: Dict mapping line numbers to status ('active',
#       'inactive', etc.)
FileResult = namedtuple('FileResult',
                        ['total_lines', 'active_lines',
                         'inactive_lines', 'line_status'])


class Analyser:  # pylint: disable=too-few-public-methods
    """Base class for source code analysers.

    This class provides common initialisation for analysers that determine
    which lines in source files are active vs inactive based on various
    methods (preprocessor analysis, debug info, etc.).
    """

    def __init__(self, srcdir, keep_temps=False):
        """Set up the analyser.

        Args:
            srcdir (str): Path to source root directory
            keep_temps (bool): If True, keep temporary files for debugging
        """
        self.srcdir = srcdir
        self.keep_temps = keep_temps

    def find_object_files(self, build_dir):
        """Find all object files in the build directory.

        Args:
            build_dir (str): Build directory to search

        Returns:
            list: List of absolute paths to .o files
        """
        obj_files = []
        for root, _, files in os.walk(build_dir):
            for fname in files:
                if fname.endswith('.o'):
                    obj_files.append(os.path.join(root, fname))
        return obj_files

    @staticmethod
    def count_lines(file_path):
        """Count the number of lines in a file.

        Args:
            file_path (str): Path to file to count lines in

        Returns:
            int: Number of lines in the file, or 0 on error
        """
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                return len(f.readlines())
        except IOError:
            return 0

#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0+
#
# Copyright 2025 Canonical Ltd
#
"""Very basic tests for codman.py script"""

import os
import shutil
import subprocess
import sys
import tempfile
import unittest

# Test configuration
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

# Import the module to test
sys.path.insert(0, SCRIPT_DIR)
sys.path.insert(0, os.path.join(SCRIPT_DIR, '..'))
# pylint: disable=wrong-import-position
from u_boot_pylib import terminal, tools
import output  # pylint: disable=wrong-import-position
import codman  # pylint: disable=wrong-import-position


class TestSourceUsage(unittest.TestCase):
    """Test cases for codman.py"""

    def setUp(self):
        """Set up test environment with fake source tree and build"""
        self.test_dir = tempfile.mkdtemp(prefix='test_source_usage_')
        self.src_dir = os.path.join(self.test_dir, 'src')
        self.build_dir = os.path.join(self.test_dir, 'build')
        os.makedirs(self.src_dir)
        os.makedirs(self.build_dir)

        # Create fake source files
        self._create_fake_sources()

        # Create fake Makefile
        self._create_makefile()

        # Create fake .config
        self._create_config()

    def tearDown(self):
        """Clean up test environment"""
        if os.path.exists(self.test_dir):
            shutil.rmtree(self.test_dir)

    def _create_fake_sources(self):
        """Create a fake source tree with various files"""
        # Create directory structure
        dirs = [
            'common',
            'drivers/video',
            'drivers/serial',
            'lib',
            'arch/sandbox',
        ]
        for dir_path in dirs:
            os.makedirs(os.path.join(self.src_dir, dir_path), exist_ok=True)

        # Create source files
        # common/main.c - will be compiled
        self._write_file('common/main.c', '''#include <common.h>

void board_init(void)
{
#ifdef CONFIG_FEATURE_A
    feature_a_init();
#endif
#ifdef CONFIG_FEATURE_B
    feature_b_init();
#endif
    common_init();
}
''')

        # common/unused.c - will NOT be compiled
        self._write_file('common/unused.c', '''#include <common.h>

void unused_function(void)
{
    /* This file is never compiled */
}
''')

        # drivers/video/display.c - will be compiled
        self._write_file('drivers/video/display.c', '''#include <video.h>

#ifdef CONFIG_VIDEO_LOGO
static void show_logo(void)
{
    /* Show boot logo */
}
#endif

void display_init(void)
{
#ifdef CONFIG_VIDEO_LOGO
    show_logo();
#endif
    /* Init display */
}
''')

        # drivers/serial/serial.c - will be compiled
        self._write_file('drivers/serial/serial.c', '''#include <serial.h>

void serial_init(void)
{
    /* Init serial port */
}
''')

        # lib/string.c - will be compiled
        self._write_file('lib/string.c', '''#include <linux/string.h>

int strlen(const char *s)
{
    int len = 0;
    while (*s++)
        len++;
    return len;
}
''')

        # arch/sandbox/cpu.c - will be compiled
        self._write_file('arch/sandbox/cpu.c', '''#include <common.h>

void cpu_init(void)
{
    /* Sandbox CPU init */
}
''')

        # Create header files
        self._write_file('include/common.h', '''#ifndef __COMMON_H
#define __COMMON_H
void board_init(void);
#endif
''')

        self._write_file('include/video.h', '''#ifndef __VIDEO_H
#define __VIDEO_H
void display_init(void);
#endif
''')

        self._write_file('include/serial.h', '''#ifndef __SERIAL_H
#define __SERIAL_H
void serial_init(void);
#endif
''')

        self._write_file('include/linux/string.h', '''#ifndef __LINUX_STRING_H
#define __LINUX_STRING_H
int strlen(const char *s);
#endif
''')

    def _create_makefile(self):
        """Create a simple Makefile that generates .cmd files"""
        makefile = f'''# Simple test Makefile
SRCDIR := {self.src_dir}
O ?= .
BUILD_DIR = $(O)

# Compiler flags
CFLAGS := -Iinclude
ifeq ($(DEBUG),1)
CFLAGS += -g
endif

# Source files to compile
OBJS = $(BUILD_DIR)/common/main.o \\
       $(BUILD_DIR)/drivers/video/display.o \\
       $(BUILD_DIR)/drivers/serial/serial.o \\
       $(BUILD_DIR)/lib/string.o \\
       $(BUILD_DIR)/arch/sandbox/cpu.o

all: $(OBJS)
\t@echo "Build complete"

# Rule to compile .c files
$(BUILD_DIR)/%.o: %.c
\t@mkdir -p $(dir $@)
\t@echo "  CC      $<"
\t@gcc $(CFLAGS) -c -o $@ $(SRCDIR)/$<
\t@echo "cmd_$@ := gcc $(CFLAGS) -c -o $@ $<" > $(dir $@).$(notdir $@).cmd
\t@echo "source_$@ := $(SRCDIR)/$<" >> $(dir $@).$(notdir $@).cmd
\t@echo "deps_$@ := \\\\" >> $(dir $@).$(notdir $@).cmd
\t@echo "  $(SRCDIR)/$< \\\\" >> $(dir $@).$(notdir $@).cmd
\t@echo "" >> $(dir $@).$(notdir $@).cmd

clean:
\t@rm -rf $(BUILD_DIR)

.PHONY: all clean
'''
        self._write_file('Makefile', makefile)

    def _create_config(self):
        """Create a fake .config file"""
        config = '''CONFIG_FEATURE_A=y
# CONFIG_FEATURE_B is not set
CONFIG_VIDEO_LOGO=y
'''
        self._write_file(os.path.join(self.build_dir, '.config'), config)

    def _write_file(self, rel_path, content):
        """Write a file relative to src_dir"""
        if rel_path.startswith('/'):
            # Absolute path for build dir files
            file_path = rel_path
        else:
            file_path = os.path.join(self.src_dir, rel_path)
        os.makedirs(os.path.dirname(file_path), exist_ok=True)
        tools.write_file(file_path, content.encode('utf-8'))

    def _build(self, debug=False):
        """Run the test build.

        Args:
            debug (bool): If True, build with debug symbols (DEBUG=1)
        """
        cmd = ['make', '-C', self.src_dir, f'O={self.build_dir}']
        if debug:
            cmd.append('DEBUG=1')
        result = subprocess.run(cmd, capture_output=True, text=True,
                                check=False)
        if result.returncode != 0:
            print(f'Build failed: {result.stderr}')
            print(f'Build stdout: {result.stdout}')
            self.fail('Test build failed')

    def test_basic_file_stats(self):
        """Test basic file-level statistics"""
        self._build()

        # Call select_sources() directly
        _all_srcs, used, skipped = codman.select_sources(
            self.src_dir, self.build_dir, None)

        # Verify counts - we have 5 compiled .c files
        self.assertEqual(len(used), 5,
                         f'Expected 5 used files, got {len(used)}')

        # Should have 1 unused .c file (common/unused.c)
        unused_c_files = [f for f in skipped if f.endswith('.c')]
        self.assertEqual(len(unused_c_files), 1,
                        f'Expected 1 unused .c file, got {len(unused_c_files)}')

        # Check that specific files are in used set
        used_basenames = {os.path.basename(f) for f in used}
        self.assertIn('main.c', used_basenames)
        self.assertIn('display.c', used_basenames)
        self.assertIn('serial.c', used_basenames)
        self.assertIn('string.c', used_basenames)
        self.assertIn('cpu.c', used_basenames)

        # Check that unused.c is not in used set
        self.assertNotIn('unused.c', used_basenames)

    def test_list_unused(self):
        """Test listing unused files"""
        self._build()

        _all_srcs, _used, skipped = codman.select_sources(
            self.src_dir, self.build_dir, None)

        # Check that unused.c is in skipped set
        skipped_basenames = {os.path.basename(f) for f in skipped}
        self.assertIn('unused.c', skipped_basenames)

        # Check that used files are not in skipped set
        self.assertNotIn('main.c', skipped_basenames)
        self.assertNotIn('display.c', skipped_basenames)

    def test_by_dir(self):
        """Test directory breakdown by collecting stats"""
        self._build()

        all_srcs, used, _skipped = codman.select_sources(
            self.src_dir, self.build_dir, None)

        # Collect directory stats
        dir_stats = output.collect_dir_stats(
            all_srcs, used, None, self.src_dir, False, False)

        # Should have stats for top-level directories
        self.assertIn('common', dir_stats)
        self.assertIn('drivers', dir_stats)
        self.assertIn('lib', dir_stats)
        self.assertIn('arch', dir_stats)

        # Check common directory has 2 files (main.c and unused.c)
        self.assertEqual(dir_stats['common'].total, 2)
        # Only 1 is used (main.c)
        self.assertEqual(dir_stats['common'].used, 1)

    def test_subdirs(self):
        """Test subdirectory breakdown"""
        self._build()

        all_srcs, used, _skipped = codman.select_sources(
            self.src_dir, self.build_dir, None)

        # Collect subdirectory stats (by_subdirs=True)
        dir_stats = output.collect_dir_stats(
            all_srcs, used, None, self.src_dir, True, False)

        # Should have stats for subdirectories
        self.assertIn('drivers/video', dir_stats)
        self.assertIn('drivers/serial', dir_stats)
        self.assertIn('arch/sandbox', dir_stats)

    def test_filter(self):
        """Test filtering by pattern"""
        self._build()

        # Apply video filter
        all_srcs, _used, _skipped = codman.select_sources(
            self.src_dir, self.build_dir, '*video*')

        # Should only have video-related files
        all_basenames = {os.path.basename(f) for f in all_srcs}
        self.assertIn('display.c', all_basenames)
        self.assertIn('video.h', all_basenames)

        # Should not have non-video files
        self.assertNotIn('main.c', all_basenames)
        self.assertNotIn('serial.c', all_basenames)

    def test_no_build_required(self):
        """Test that analysis works with existing build"""
        self._build()

        # Should work without building
        all_srcs, used, _skipped = codman.select_sources(
            self.src_dir, self.build_dir, None)

        # Verify we got results
        self.assertGreater(len(all_srcs), 0)
        self.assertGreater(len(used), 0)

    def test_do_analysis_unifdef(self):
        """Test do_analysis() with unifdef"""
        self._build()

        _all_srcs, used, _skipped = codman.select_sources(
            self.src_dir, self.build_dir, None)

        # Run unifdef analysis
        unifdef_path = shutil.which('unifdef') or '/usr/bin/unifdef'
        results = codman.do_analysis(used, self.build_dir, self.src_dir,
                                     unifdef_path, include_headers=False,
                                     jobs=1, use_lsp=False)

        # Should get results
        self.assertIsNotNone(results)
        self.assertGreater(len(results), 0)

        # Check that results have the expected structure
        for _file_path, result in results.items():
            self.assertGreater(result.total_lines, 0)
            self.assertGreaterEqual(result.active_lines, 0)
            self.assertGreaterEqual(result.inactive_lines, 0)
            self.assertEqual(result.total_lines,
                           result.active_lines + result.inactive_lines)

    def test_do_analysis_dwarf(self):
        """Test do_analysis() with DWARF"""
        # Build with debug symbols
        self._build(debug=True)

        _all_srcs, used, _skipped = codman.select_sources(
            self.src_dir, self.build_dir, None)

        # Run DWARF analysis (unifdef_path=None)
        results = codman.do_analysis(used, self.build_dir, self.src_dir,
                                     unifdef_path=None, include_headers=False,
                                     jobs=1, use_lsp=False)

        # Should get results
        self.assertIsNotNone(results)
        self.assertGreater(len(results), 0)

        # Check that results have the expected structure
        for _file_path, result in results.items():
            self.assertGreater(result.total_lines, 0)
            self.assertGreaterEqual(result.active_lines, 0)
            self.assertGreaterEqual(result.inactive_lines, 0)
            self.assertEqual(result.total_lines,
                           result.active_lines + result.inactive_lines)

    def test_do_analysis_unifdef_missing_config(self):
        """Test do_analysis() with unifdef when config file is missing"""
        self._build()

        _all_srcs, used, _skipped = codman.select_sources(
            self.src_dir, self.build_dir, None)

        # Remove .config file
        config_file = os.path.join(self.build_dir, '.config')
        if os.path.exists(config_file):
            os.remove(config_file)

        # Capture terminal output
        with terminal.capture() as (_stdout, stderr):
            # Run unifdef analysis - should return None
            unifdef_path = shutil.which('unifdef') or '/usr/bin/unifdef'
            results = codman.do_analysis(used, self.build_dir, self.src_dir,
                                         unifdef_path,
                                         include_headers=False, jobs=1,
                                         use_lsp=False)

        # Should return None when config is missing
        self.assertIsNone(results)

        # Check that error message was printed to stderr
        error_text = stderr.getvalue()
        self.assertIn('Config file not found', error_text)
        self.assertIn('.config', error_text)

    def test_do_analysis_lsp(self):
        """Test do_analysis() with LSP (clangd)"""
        # Disabled for now
        self.skipTest('LSP test disabled')
        # Check if clangd is available
        if not shutil.which('clangd'):
            self.skipTest('clangd not found - skipping LSP test')

        # Build with compile commands
        self._build()

        _all_srcs, used, _skipped = codman.select_sources(
            self.src_dir, self.build_dir, None)

        # Run LSP analysis (unifdef_path=None, use_lsp=True)
        results = codman.do_analysis(used, self.build_dir, self.src_dir,
                                     unifdef_path=None, include_headers=False,
                                     jobs=1, use_lsp=True)

        # Should get results
        self.assertIsNotNone(results)
        self.assertGreater(len(results), 0)

        # Check that results have the expected structure
        for _file_path, result in results.items():
            self.assertGreater(result.total_lines, 0)
            self.assertGreaterEqual(result.active_lines, 0)
            self.assertGreaterEqual(result.inactive_lines, 0)
            self.assertEqual(result.total_lines,
                           result.active_lines + result.inactive_lines)

        # Check specific file results
        main_file = os.path.join(self.src_dir, 'common/main.c')
        if main_file in results:
            result = results[main_file]
            # main.c has some conditional code, so should have some lines
            self.assertGreater(result.total_lines, 0)
            # Should have identified some active lines
            self.assertGreater(result.active_lines, 0)


if __name__ == '__main__':
    unittest.main(argv=['test_codman.py'], verbosity=2)

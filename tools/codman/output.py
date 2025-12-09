# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Canonical Ltd
#
"""Output formatting and display functions for srcman.

This module provides functions for displaying analysis results in various
formats:
- Statistics views (file-level and line-level)
- Directory breakdowns (top-level and subdirectories)
- Per-file summaries
- Detailed line-by-line views
- File listings (used/unused)
- File copying operations
"""

import os
import shutil
import sys
from collections import defaultdict

# Import from tools directory
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))
from u_boot_pylib import terminal, tout  # pylint: disable=wrong-import-position


class DirStats:  # pylint: disable=too-few-public-methods
    """Statistics for a directory.

    Attributes:
        total: Total number of files in directory
        used: Number of files used (compiled)
        unused: Number of files not used
        lines_total: Total lines of code in directory
        lines_used: Number of active lines (after preprocessing)
        files: List of file info dicts (for --show-files)
    """
    def __init__(self):
        self.total = 0
        self.used = 0
        self.unused = 0
        self.lines_total = 0
        self.lines_used = 0
        self.files = []


def count_lines(file_path):
    """Count lines in a file"""
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            return sum(1 for _ in f)
    except IOError:
        return 0


def klocs(lines):
    """Format line count in thousands, rounded to 1 decimal place.

    Args:
        lines (int): Line count (e.g., 3500)

    Returns:
        Formatted string in thousands (e.g., '3.5')
    """
    kloc = round(lines / 1000, 1)
    return f'{kloc:.1f}'


def percent(numerator, denominator):
    """Calculate percentage, handling division by zero.

    Args:
        numerator (int/float): The numerator
        denominator (int/float): The denominator

    Returns:
        float: Percentage (0-100), or 0 if denominator is 0
    """
    return 100 * numerator / denominator if denominator else 0


def print_heading(text, width=70, char='='):
    """Print a heading with separator lines.

    Args:
        text (str): Heading text to display (empty for separator only)
        width (int): Width of the separator line
        char (str): Character to use for separator
    """
    print(char * width)
    if text:
        print(text)
        print(char * width)


def show_file_detail(detail_file, file_results, srcdir):
    """Show detailed line-by-line analysis for a specific file.

    Args:
        detail_file (str): Path to the file to show details for (relative or
            absolute)
        file_results (dict): Dictionary mapping file paths to analysis results
        srcdir (str): Root directory of the source tree

    Returns:
        True on success, False on error
    """
    detail_path = os.path.realpath(detail_file)
    if detail_path not in file_results:
        # Try relative to source root
        detail_path = os.path.realpath(os.path.join(srcdir, detail_file))

    if detail_path in file_results:
        result = file_results[detail_path]
        rel_path = os.path.relpath(detail_path, srcdir)

        print_heading(f'DETAIL FOR: {rel_path}', width=70)
        print(f'Total lines:    {result.total_lines:6}')
        pct_active = percent(result.active_lines, result.total_lines)
        pct_inactive = percent(result.inactive_lines, result.total_lines)
        print(f'Active lines:   {result.active_lines:6} ({pct_active:.1f}%)')
        print(f'Inactive lines: {result.inactive_lines:6} ' +
              f'({pct_inactive:.1f}%)')
        print()

        # Show the file with status annotations
        with open(detail_path, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()

        col = terminal.Color()
        for line_num, line in enumerate(lines, 1):
            status = result.line_status.get(line_num, 'unknown')
            marker = '-' if status == 'inactive' else ' '
            prefix = f'{marker} {line_num:4} | '
            code = line.rstrip()

            if status == 'active':
                # Normal color for active code
                print(prefix + code)
            else:
                # Non-bright cyan for inactive code
                print(prefix + col.build(terminal.Color.CYAN, code,
                                         bright=False))
        return True

    # File not found - caller handles errors
    return False


def show_file_summary(file_results, srcdir):
    """Show per-file summary of line analysis.

    Args:
        file_results (dict): Dictionary mapping file paths to analysis results
        srcdir (str): Root directory of the source tree

    Returns:
        bool: True on success
    """
    print_heading('PER-FILE SUMMARY', width=90)
    print(f"{'File':<50} {'Total':>8} {'Active':>8} "
          f"{'Inactive':>8} {'%Active':>8}")
    print('-' * 90)

    for source_file in sorted(file_results.keys()):
        result = file_results[source_file]
        rel_path = os.path.relpath(source_file, srcdir)
        if len(rel_path) > 47:
            rel_path = '...' + rel_path[-44:]

        pct_active = percent(result.active_lines, result.total_lines)
        print(f'{rel_path:<50} {result.total_lines:>8} '
              f'{result.active_lines:>8} {result.inactive_lines:>8} '
              f'{pct_active:>7.1f}%')

    return True


def list_unused_files(skipped_sources, srcdir):
    """List unused source files.

    Args:
        skipped_sources (set of str): Set of unused source file paths (relative
            to srcdir)
        srcdir (str): Root directory of the source tree

    Returns:
        bool: True on success
    """
    print(f'Unused source files ({len(skipped_sources)}):')
    for source_file in sorted(skipped_sources):
        try:
            rel_path = os.path.relpath(source_file, srcdir)
        except ValueError:
            rel_path = source_file
        print(f'  {rel_path}')

    return True


def list_used_files(used_sources, srcdir):
    """List used source files.

    Args:
        used_sources (set of str): Set of used source file paths (relative
            to srcdir)
        srcdir (str): Root directory of the source tree

    Returns:
        bool: True on success
    """
    print(f'Used source files ({len(used_sources)}):')
    for source_file in sorted(used_sources):
        try:
            rel_path = os.path.relpath(source_file, srcdir)
        except ValueError:
            rel_path = source_file
        print(f'  {rel_path}')

    return True


def copy_used_files(used_sources, srcdir, dest_dir):
    """Copy used source files to a destination directory, preserving structure.

    Args:
        used_sources (set): Set of used source file paths (relative to srcdir)
        srcdir (str): Root directory of the source tree
        dest_dir (str): Destination directory for the copy

    Returns:
        True on success, False if errors occurred
    """
    if os.path.exists(dest_dir):
        tout.error(f'Destination directory already exists: {dest_dir}')
        return False

    tout.progress(f'Copying {len(used_sources)} used source files to ' +
                  f'{dest_dir}')

    copied_count = 0
    error_count = 0

    for source_file in sorted(used_sources):
        src_path = os.path.join(srcdir, source_file)
        dest_path = os.path.join(dest_dir, source_file)

        try:
            # Create parent directory if needed
            dest_parent = os.path.dirname(dest_path)
            os.makedirs(dest_parent, exist_ok=True)

            # Copy the file
            shutil.copy2(src_path, dest_path)
            copied_count += 1
        except IOError as e:
            error_count += 1
            tout.error(f'Error copying {source_file}: {e}')

    tout.progress(f'Copied {copied_count} files to {dest_dir}')
    if error_count:
        tout.error(f'Failed to copy {error_count} files')
        return False

    return True


def collect_dir_stats(all_sources, used_sources, file_results, srcdir,
                      by_subdirs, show_files):
    """Collect statistics organized by directory.

    Args:
        all_sources (set): Set of all source file paths
        used_sources (set): Set of used source file paths
        file_results (dict): Optional dict mapping file paths to line
            analysis results (or None)
        srcdir (str): Root directory of the source tree
        by_subdirs (bool): If True, use full subdirectory paths;
            otherwise top-level only
        show_files (bool): If True, collect individual file info within
            each directory

    Returns:
        dict: Directory statistics keyed by directory path
    """
    dir_stats = defaultdict(DirStats)

    for source_file in all_sources:
        rel_path = os.path.relpath(source_file, srcdir)

        if by_subdirs:
            # Use the full directory path (not including the filename)
            dir_path = os.path.dirname(rel_path)
            if not dir_path:
                dir_path = '.'
        else:
            # Use only the top-level directory
            dir_path = (rel_path.split(os.sep)[0] if os.sep in rel_path
                        else rel_path)

        line_count = count_lines(source_file)
        dir_stats[dir_path].total += 1
        dir_stats[dir_path].lines_total += line_count

        if source_file in used_sources:
            dir_stats[dir_path].used += 1
            # Use active line count if line-level analysis was performed
            # Normalize path to match file_results keys (absolute paths)
            abs_source = os.path.realpath(source_file)

            # Try to find the file in file_results
            result = None
            if file_results:
                if abs_source in file_results:
                    result = file_results[abs_source]
                elif source_file in file_results:
                    result = file_results[source_file]

            if result:
                active_lines = result.active_lines
                inactive_lines = result.inactive_lines
                dir_stats[dir_path].lines_used += active_lines
                # Store file info for --show-files (exclude .h files)
                if show_files and not rel_path.endswith('.h'):
                    dir_stats[dir_path].files.append({
                        'path': rel_path,
                        'total': line_count,
                        'active': active_lines,
                        'inactive': inactive_lines
                    })
            else:
                # File not found in results - count all lines
                tout.debug(f'File not in results (using full count): '
                           f'{rel_path}')
                dir_stats[dir_path].lines_used += line_count
                if show_files and not rel_path.endswith('.h'):
                    dir_stats[dir_path].files.append({
                        'path': rel_path,
                        'total': line_count,
                        'active': line_count,
                        'inactive': 0
                    })
        else:
            dir_stats[dir_path].unused += 1

    return dir_stats


def print_dir_stats(dir_stats, file_results, by_subdirs, show_files,
                    show_empty):
    """Print directory statistics table.

    Args:
        dir_stats (dict): Directory statistics keyed by directory path
        file_results (dict): Optional dict mapping file paths to line analysis
            results (or None)
        by_subdirs (bool): If True, show full subdirectory breakdown; otherwise
            top-level only
        show_files (bool): If True, show individual files within directories
        show_empty (bool): If True, show directories with 0 lines used
    """
    # Sort alphabetically by directory name
    sorted_dirs = sorted(dir_stats.items(), key=lambda x: x[0])

    for dir_path, stats in sorted_dirs:
        # Skip subdirectories with 0 lines used unless --show-zero-lines is set
        if by_subdirs and not show_empty and stats.lines_used == 0:
            continue

        pct_used = percent(stats.used, stats.total)
        pct_code = percent(stats.lines_used, stats.lines_total)
        # Truncate long paths
        display_path = dir_path
        if len(display_path) > 37:
            display_path = '...' + display_path[-34:]
        print(f'{display_path:<40} {stats.total:>7} {stats.used:>7} '
              f'{pct_used:>6.0f} {pct_code:>6.0f} '
              f'{klocs(stats.lines_total):>8} {klocs(stats.lines_used):>7}')

        # Show individual files if requested
        if show_files and stats.files:
            # Sort files alphabetically by filename
            sorted_files = sorted(stats.files, key=lambda x: os.path.basename(x['path']))

            for info in sorted_files:
                # Skip files with 0 active lines unless show_empty is set
                if not show_empty and info['active'] == 0:
                    continue

                filename = os.path.basename(info['path'])
                if len(filename) > 35:
                    filename = filename[:32] + '...'

                if file_results:
                    # Show line-level details
                    pct_active = percent(info['active'], info['total'])
                    # Align with directory format: skip Files/Used columns,
                    # show %code, then lines in kLOC column, active in Used column
                    print(f"  {filename:<38} {'':>7} {'':>7} {'':>6} "
                          f"{pct_active:>6.0f} {klocs(info['total']):>8} "
                          f"{klocs(info['active']):>7}")
                else:
                    # Show file-level only
                    print(f"  {filename:<38} {info['total']:>7} lines")

            # Add blank line after file list
            print()


def show_dir_breakdown(all_sources, used_sources, file_results, srcdir,
                       by_subdirs, show_files, show_empty):
    """Show breakdown by directory (top-level or subdirectories).

    Args:
        all_sources (set): Set of all source file paths
        used_sources (set): Set of used source file paths
        file_results (dict): Optional dict mapping file paths to line analysis
            results (or None)
        srcdir (str): Root directory of the source tree
        by_subdirs (bool): If True, show full subdirectory breakdown; otherwise
             top-level only
        show_files (bool): If True, show individual files within each directory
        show_empty (bool): If True, show directories with 0 lines used

    Returns:
        bool: True on success
    """
    # Width of the main table (Directory + Total + Used columns)
    table_width = 87

    print_heading('BREAKDOWN BY TOP-LEVEL DIRECTORY' if by_subdirs else '',
                  width=table_width)
    print(f"{'Directory':<40} {'Files':>7} {'Used':>7} {'%Used':>6} " +
          f"{'%Code':>6} {'kLOC':>8} {'Used':>7}")
    print('-' * table_width)

    # Collect directory statistics
    dir_stats = collect_dir_stats(all_sources, used_sources, file_results,
                                  srcdir, by_subdirs, show_files)

    # Print directory statistics
    print_dir_stats(dir_stats, file_results, by_subdirs, show_files, show_empty)

    print('-' * table_width)
    total_lines_all = sum(count_lines(f) for f in all_sources)
    # Calculate used lines: if we have file_results, use active_lines from there
    # Otherwise, count all lines in used files
    if file_results:
        total_lines_used = sum(r.active_lines for r in file_results.values())
    else:
        total_lines_used = sum(count_lines(f) for f in used_sources)
    pct_files = percent(len(used_sources), len(all_sources))
    pct_code = percent(total_lines_used, total_lines_all)
    print(f"{'TOTAL':<40} {len(all_sources):>7} {len(used_sources):>7} "
          f"{pct_files:>6.0f} {pct_code:>6.0f} "
          f"{klocs(total_lines_all):>8} {klocs(total_lines_used):>7}")
    print_heading('', width=table_width)
    print()

    return True


def show_statistics(all_sources, used_sources, skipped_sources, file_results,
                    srcdir, top_n):
    """Show overall statistics about source file usage.

    Args:
        all_sources (set of str): Set of all source file paths
        used_sources (set of str): Set of used source file paths
        skipped_sources (set of str): Set of unused source file paths
        file_results (dict): Optional dict mapping file paths to line analysis
            results
        srcdir (str): Root directory of the source tree
        top_n (int): Number of top files with most inactive code to show

    Returns:
        bool: True on success
    """
    # Calculate line counts - use file_results (DWARF/unifdef) if available
    if file_results:
        # Use active lines from analysis results
        used_lines = sum(r.active_lines for r in file_results.values())
    else:
        # Fall back to counting all lines in used files
        used_lines = sum(count_lines(f) for f in used_sources)

    unused_lines = sum(count_lines(f) for f in skipped_sources)
    total_lines = used_lines + unused_lines

    print_heading('FILE-LEVEL STATISTICS', width=70)
    print(f'Total source files:   {len(all_sources):6}')
    used_pct = percent(len(used_sources), len(all_sources))
    print(f'Used source files:    {len(used_sources):6} ({used_pct:.1f}%)')
    unused_pct = percent(len(skipped_sources), len(all_sources))
    print(f'Unused source files:  {len(skipped_sources):6} ' +
          f'({unused_pct:.1f}%)')
    print()
    print(f'Total lines of code:  {total_lines:6}')
    used_lines_pct = percent(used_lines, total_lines)
    print(f'Used lines of code:   {used_lines:6} ({used_lines_pct:.1f}%)')
    unused_lines_pct = percent(unused_lines, total_lines)
    print(f'Unused lines of code: {unused_lines:6} ' +
          f'({unused_lines_pct:.1f}%)')
    print_heading('', width=70)

    # If line-level analysis was performed, show those stats too
    if file_results:
        print()
        total_lines_analysed = sum(r.total_lines for r in file_results.values())
        active_lines = sum(r.active_lines for r in file_results.values())
        inactive_lines = sum(r.inactive_lines for r in file_results.values())

        print_heading('LINE-LEVEL STATISTICS (within compiled files)', width=70)
        print(f'Files analysed:           {len(file_results):6}')
        print(f'Total lines in used files:{total_lines_analysed:6}')
        active_pct = percent(active_lines, total_lines_analysed)
        print(f'Active lines:             {active_lines:6} ' +
              f'({active_pct:.1f}%)')
        inactive_pct = percent(inactive_lines, total_lines_analysed)
        print(f'Inactive lines:           {inactive_lines:6} ' +
              f'({inactive_pct:.1f}%)')
        print_heading('', width=70)
        print()

        # Show top files with most inactive code
        files_by_inactive = sorted(
            file_results.items(),
            key=lambda x: x[1].inactive_lines,
            reverse=True
        )

        print(f'TOP {top_n} FILES WITH MOST INACTIVE CODE:')
        print('-' * 70)
        for source_file, result in files_by_inactive[:top_n]:
            rel_path = os.path.relpath(source_file, srcdir)
            pct_inactive = percent(result.inactive_lines, result.total_lines)
            print(f'  {result.inactive_lines:5} inactive lines ' +
                  f'({pct_inactive:4.1f}%) - {rel_path}')

    return True

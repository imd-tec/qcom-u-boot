#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0+
"""Check alignment of U-Boot linker lists.

Auto-discover and verify the uniform spacing of all U-Boot linker list symbols.

Analyze the symbol table of a U-Boot ELF file to ensure that all entries in all
linker-generated lists are separated by a consistent number of bytes. Detect
problems caused by linker-inserted alignment padding.

By default, produce no output if no problems are found.
Use the -v flag to force output even on success.

Exit Codes:
  0: Success - no alignment problems were found
  1: Usage Error - the script was not called with the correct arguments
  2: Execution Error - failed to run `nm` or the ELF file was not found
  3: Problem Found - an inconsistent gap was detected in at least one list
"""

import sys
import subprocess
import re
import argparse
from statistics import mode
from collections import defaultdict, namedtuple

# Information about a symbol: address, size (from nm -S), and name
Symbol = namedtuple('Symbol', ['address', 'size', 'name'])

# Information about the gap between two consecutive symbols
Gap = namedtuple('Gap', ['gap', 'prev_sym', 'next_sym', 'prev_size'])

# Start and end marker addresses for a list
Markers = namedtuple('Markers', ['start', 'end'])

# Holds all the analysis results from checking the lists
Results = namedtuple('Results', [
    'total_problems', 'total_symbols', 'all_lines', 'max_name_len',
    'list_count'])

def eprint(*args, **kwargs):
    """Print to stderr"""
    print(*args, file=sys.stderr, **kwargs)

def check_single_list(name, symbols, max_name_len, marker_info=None):
    """Check alignment for a single list and return its findings

    Args:
        name (str): The cleaned-up name of the list for display
        symbols (list): A list of Symbol tuples, sorted by address
        max_name_len (int): The max length of list names for column formatting
        marker_info (Markers): Optional namedtuple with start and end addresses

    Returns:
        tuple: (problem_count, list_of_output_lines)
    """
    lines = []
    if len(symbols) < 2:
        return 0, []

    gaps = []
    for i in range(len(symbols) - 1):
        sym1, sym2 = symbols[i], symbols[i+1]
        gaps.append(Gap(gap=sym2.address - sym1.address, prev_sym=sym1.name,
                        next_sym=sym2.name, prev_size=sym1.size))

    expected_gap = mode(g.gap for g in gaps)

    problem_count = 0
    hex_gap = f'0x{expected_gap:x}'
    line = f'{name:<{max_name_len + 2}}  {len(symbols):>12}  {hex_gap:>17}'
    lines.append(line)

    for g in gaps:
        if g.gap != expected_gap:
            problem_count += 1
            lines.append(
                f'  - Bad gap (0x{g.gap:x}) before symbol: {g.next_sym}')
        elif g.prev_size and g.gap > g.prev_size:
            # Gap is larger than symbol size - padding was inserted
            problem_count += 1
            lines.append(
                f'  - Padding: gap 0x{g.gap:x} > size 0x{g.prev_size:x}'
                f' before: {g.next_sym}')

    # Check if start/end marker span is a multiple of the struct size
    # If not, pointer subtraction (end - start) will produce wrong results
    # due to compiler optimization using magic number multiplication
    if marker_info:
        total_span = marker_info.end - marker_info.start
        if total_span % expected_gap != 0:
            problem_count += 1
            remainder = total_span % expected_gap
            lines.append(
                f'  - Pointer arithmetic bug: span 0x{total_span:x} is not a '
                f'multiple of struct size 0x{expected_gap:x} '
                f'(remainder: {remainder})')

    return problem_count, lines

def run_nm_and_get_lists(elf_path):
    """Run `nm -S` and parse the output to discover all linker lists

    Args:
        elf_path (str): The path to the ELF file to process

    Returns:
        tuple or None: (lists_dict, markers_dict) or None on error
            lists_dict: entries keyed by base_name
            markers_dict: start/end marker addresses keyed by base_name
    """
    cmd = ['nm', '-S', '-n', elf_path]
    try:
        proc = subprocess.run(cmd, capture_output=True, text=True, check=True)
    except FileNotFoundError:
        eprint(
            "Error: The 'nm' command was not found. "
            'Please ensure binutils is installed')
        return None
    except subprocess.CalledProcessError as e:
        eprint(
            f"Error: Failed to execute 'nm' on '{elf_path}'.\n"
            f'  Return Code: {e.returncode}\n  Stderr:\n{e.stderr}')
        return None

    # Pattern to match _2_ entries (the actual list elements)
    entry_pattern = re.compile(
        r'^(?P<base_name>_u_boot_list_\d+_\w+)(?:_info)?_2_')
    # Pattern to match _1 (start) and _3 (end) markers
    marker_pattern = re.compile(
        r'^(?P<base_name>_u_boot_list_\d+_\w+)_(?P<marker>[13])$')

    lists = defaultdict(list)
    markers = defaultdict(dict)  # {base_name: {'start': addr, 'end': addr}}

    for line in proc.stdout.splitlines():
        if '_u_boot_list_' not in line:
            continue
        try:
            parts = line.strip().split()
            name = parts[-1]
            address = int(parts[0], 16)
            # Size is present if we have 4 parts and parts[2] is a single char
            if len(parts) == 4 and len(parts[2]) == 1:
                size = int(parts[1], 16)
            else:
                size = 0  # Size not available

            # Check for entry (_2_) symbols - must be uppercase D
            if ' D _u_boot_list_' in line:
                match = entry_pattern.match(name)
                if match:
                    base_name = match.group('base_name')
                    lists[base_name].append(Symbol(address, size, name))
                continue

            # Check for marker (_1 or _3) symbols - can be any type
            match = marker_pattern.match(name)
            if match:
                base_name = match.group('base_name')
                marker_type = match.group('marker')
                if marker_type == '1':
                    markers[base_name]['start'] = address
                else:  # marker_type == '3'
                    markers[base_name]['end'] = address

        except (ValueError, IndexError):
            eprint(f'Warning: Could not parse line: {line}')

    # Convert marker dicts to Markers namedtuples (only if both start/end exist)
    marker_tuples = {}
    for base_name, m in markers.items():
        if 'start' in m and 'end' in m:
            marker_tuples[base_name] = Markers(m['start'], m['end'])

    return lists, marker_tuples

def collect_data(lists, markers):
    """Collect alignment check data for all lists

    Args:
        lists (dict): A dictionary of lists and their symbols
        markers (dict): A dictionary of start/end marker addresses per list

    Returns:
        Results: A namedtuple containing the analysis results
    """
    if markers is None:
        markers = {}

    names = {}
    prefix_to_strip = '_u_boot_list_2_'
    for list_name in lists.keys():
        name = list_name[len(prefix_to_strip):] if list_name.startswith(
            prefix_to_strip) else list_name
        names[list_name] = name

    max_name_len = max(len(name) for name in names.values()) if names else 0

    total_problems = 0
    total_symbols = 0
    all_lines = []
    for list_name in sorted(lists.keys()):
        symbols = lists[list_name]
        total_symbols += len(symbols)
        name = names[list_name]
        marker_info = markers.get(list_name)
        problem_count, lines = check_single_list(name, symbols, max_name_len,
                                                 marker_info)
        total_problems += problem_count
        all_lines.extend(lines)

    return Results(
        total_problems=total_problems,
        total_symbols=total_symbols,
        all_lines=all_lines,
        max_name_len=max_name_len,
        list_count=len(lists))

def show_output(results, verbose):
    """Print the collected results to stderr based on verbosity

    Args:
        results (Results): The analysis results from collect_data()
        verbose (bool): True to print output even on success
    """
    if results.total_problems == 0 and not verbose:
        return

    header = (f"{'List Name':<{results.max_name_len + 2}}  {'# Symbols':>12}  "
                f"{'Struct Size (hex)':>17}")
    sep = f"{'-' * (results.max_name_len + 2)}  {'-' * 12}  {'-' * 17}"
    eprint(header)
    eprint(sep)
    for line in results.all_lines:
        eprint(line)

    # Print footer
    eprint(f"{'-' * (results.max_name_len + 2)}  {'-' * 12}")
    eprint(f"{f'{results.list_count} lists':<{results.max_name_len + 2}}  "
            f"{results.total_symbols:>12}")

    if results.total_problems > 0:
        eprint(f'\nFAILURE: Found {results.total_problems} alignment problems')
    elif verbose:
        eprint('\nSUCCESS: All discovered lists have consistent alignment')

def main():
    """Main entry point of the script, returns an exit code"""
    epilog_text = '''
Auto-discover all linker-generated lists in a U-Boot ELF file
(e.g., for drivers, commands, etc.) and verify their integrity.

Problems detected (cause build failure):

1. Inconsistent gaps: Elements in a list should all be separated by the same
   number of bytes (the struct size). If the linker inserts padding between
   some elements but not others, this is detected and reported.

2. Padding detection: Using symbol sizes from nm -S, the script compares each
   symbol's size to the gap after it. If gap > size, the linker inserted
   padding, which breaks U-Boot's assumption that the list is a contiguous
   array of same-sized structs.

3. Pointer arithmetic bugs: Each list has start (_1) and end (_3) markers.
   If the span (end - start) is not a multiple of struct size, pointer
   subtraction produces garbage due to GCC's magic-number division.
'''
    parser = argparse.ArgumentParser(
        description='Check alignment of U-Boot linker lists in an ELF file.',
        epilog=epilog_text,
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument('elf_path', metavar='path_to_elf_file',
                        help='Path to the U-Boot ELF file to check')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Print detailed output even on success')

    args = parser.parse_args()

    result = run_nm_and_get_lists(args.elf_path)
    if result is None:
        return 2  # Error running nm

    lists, markers = result
    if not lists:
        if args.verbose:
            eprint('Success: No U-Boot linker lists found to check')
        return 0

    results = collect_data(lists, markers)
    show_output(results, args.verbose)

    return 3 if results.total_problems > 0 else 0

if __name__ == '__main__':
    sys.exit(main())

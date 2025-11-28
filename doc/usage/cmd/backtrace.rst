.. SPDX-License-Identifier: GPL-2.0+

.. index::
   single: backtrace (command)

backtrace command
=================

Synopsis
--------

::

    backtrace

Description
-----------

The *backtrace* command prints a backtrace of the current call stack. This can
be useful for debugging to see how a particular code path was reached.

The output shows each stack frame with the function name, source file, and line
number (when debug information is available). This includes static functions.

Example
-------

::

    => backtrace
    backtrace: 14 addresses
      backtrace_show() at lib/backtrace.c:18
      do_backtrace() at cmd/backtrace.c:17
      cmd_process() at common/command.c:637
      run_list_real() at common/cli_hush.c:1868
      parse_stream_outer() at common/cli_hush.c:3207
      parse_string_outer() at common/cli_hush.c:3257
      run_command_list() at common/cli.c:168
      sandbox_main_loop_init() at arch/sandbox/cpu/start.c:153
      board_init_r() at common/board_r.c:774
      ...

Configuration
-------------

The backtrace command is enabled by CONFIG_CMD_BACKTRACE which depends on
CONFIG_BACKTRACE. Currently this is only available on sandbox.

The sandbox implementation uses libbacktrace (bundled with GCC) to provide
detailed symbol information including function names, source files, and line
numbers.

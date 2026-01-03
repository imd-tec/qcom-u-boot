.. SPDX-License-Identifier: GPL-2.0+:

.. index::
   single: malloc (command)

malloc command
==============

Synopsis
--------

::

    malloc info
    malloc dump
    malloc log [start|stop]

Description
-----------

The malloc command shows information about the malloc heap.

info
    Shows memory-allocation statistics, including the total heap size, the
    amount currently in use, and call counts for malloc(), free(), and
    realloc().

dump
    Walks the heap and prints each chunk's address, size (in hex), and status.
    In-use chunks show no status label, while free chunks show ``<free>``.
    Special entries show ``(chunk header)``, ``top``, or ``end``. This is useful
    for debugging memory allocation issues. When CONFIG_MCHECK_HEAP_PROTECTION
    is enabled, the caller string is also shown if available.

log
    Controls the malloc traffic log. With no argument, dumps the recorded log
    entries. Use ``start`` to begin recording malloc/free/realloc calls, and
    ``stop`` to stop recording. Each entry shows the operation type, pointer
    address, size, and caller backtrace. This is useful for tracking down
    memory leaks or understanding allocation patterns.

The total heap size is set by ``CONFIG_SYS_MALLOC_LEN``.

Example
-------

::

    => malloc info
    total bytes   = 96 MiB
    in use bytes  = 700.9 KiB
    malloc count  = 1234
    free count    = 567
    realloc count = 89

    => malloc dump
    Heap dump: 19a0e000 - 1fa10000
         Address        Size  Status
    ----------------------------------
        19a0e000          10  (chunk header)
        19a0e010          a0
        19a0e0b0        6070
        19adfc30          60  <free>
        19adff90     5f3f030  top
        1fa10000              end
    ----------------------------------
    Used: c2ef0 bytes in 931 chunks
    Free: 5f3f0c0 bytes in 2 chunks + top

With CONFIG_MCHECK_HEAP_PROTECTION enabled, the caller backtrace is shown::

    => malloc dump
    Heap dump: 18a1d000 - 1ea1f000
         Address        Size  Status
    ----------------------------------
        18a1d000          10  (chunk header)
        18a1d010          90  used  log_init:453 <-board_init_r:774
        18a1d0a0        6060  used  membuf_new:420 <-console_record
        18a3b840          90  used  of_alias_scan:911 <-board_init_
        ...

With CONFIG_CMD_MALLOC_LOG enabled, the log subcommand is available::

    => malloc log start
    Malloc logging started
    => ... do some operations ...
    => malloc log stop
    Malloc logging stopped
    => malloc log
    Malloc log: 5 entries (max 524288, total 5)
     Seq  Type                   Ptr      Size  Caller
    ----  --------  ----------------  --------  ------
       0  alloc             16a01b90        20  hush_file_init:3277
                  <-parse_file_outer:3295 <-run_pipe_real:1986
       1  alloc             16a01bc0       100  xmalloc:107 <-xzalloc:117
                  <-new_pipe:1498 <-run_list_real:1702
       2  free              16a01bc0         0  free_pipe_list:2001
                  <-parse_stream_outer:3208 <-parse_file_outer:3300
       ...

Configuration
-------------

The malloc command is enabled by CONFIG_CMD_MALLOC which depends on
CONFIG_MALLOC_DEBUG. The log subcommand is enabled by CONFIG_CMD_MALLOC_LOG
which additionally requires CONFIG_MCHECK_LOG.

Return value
------------

The return value $? is 0 (true) on success, 1 (false) on failure.

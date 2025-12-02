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

Configuration
-------------

The malloc command is enabled by CONFIG_CMD_MALLOC which depends on
CONFIG_MALLOC_DEBUG.

Return value
------------

The return value $? is 0 (true) on success, 1 (false) on failure.

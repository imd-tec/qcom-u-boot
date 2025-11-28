.. SPDX-License-Identifier: GPL-2.0+:

.. index::
   single: malloc (command)

malloc command
==============

Synopsis
--------

::

    malloc info

Description
-----------

The malloc command shows information about the malloc heap.

info
    Shows memory-allocation statistics, including the total heap size and the
    amount currently in use.

The total heap size is set by ``CONFIG_SYS_MALLOC_LEN``.

Example
-------

::

    => malloc info
    total bytes  = 96 MiB
    in use bytes = 700.9 KiB

Configuration
-------------

The malloc command is enabled by CONFIG_CMD_MALLOC which depends on
CONFIG_MALLOC_DEBUG.

Return value
------------

The return value $? is 0 (true) on success, 1 (false) on failure.

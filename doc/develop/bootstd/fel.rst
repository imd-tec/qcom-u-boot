.. SPDX-License-Identifier: GPL-2.0+:

FEL Bootmeth
=============

The FEL bootmeth supports booting Allwinner (sunxi) boards that have been
started via the USB FEL protocol. The SPL places a boot script in memory and
records its address before jumping to U-Boot proper.

This is a global bootmeth: it is not tied to a particular bootdev but is
invoked once during each scan.

During discovery, ``fel_read_bootflow()`` checks for the ``fel_booted`` and
``fel_scriptaddr`` environment variables. These are set by SPL when the board
is FEL-booted. If either is missing the method is skipped.

At boot time, ``fel_boot()`` reads the script address from ``fel_scriptaddr``
and executes it with ``cmd_source_script()``. No files are loaded from a
filesystem; the script was already placed in memory by the FEL loader.

The compatible string "u-boot,fel-bootmeth" is used for the driver. It is
present if `CONFIG_BOOTMETH_FEL` is enabled.

See :doc:`/board/allwinner/sunxi` for general Allwinner board information.

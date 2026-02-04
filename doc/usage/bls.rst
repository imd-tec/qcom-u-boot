.. SPDX-License-Identifier: GPL-2.0+

Boot Loader Specification (BLS) Type #1 Support
================================================

U-Boot supports Boot Loader Specification (BLS) Type #1 boot entries as defined
in the `Boot Loader Specification`_.

.. _Boot Loader Specification: https://uapi-group.org/specifications/specs/boot_loader_specification/

Overview
--------

BLS provides a standardised way to describe boot entries. U-Boot's BLS support
allows it to boot operating systems configured with BLS entries, which is used
by Fedora, RHEL, and other distributions.

The current implementation supports a single BLS entry file at
``loader/entry.conf``. Future versions may support multiple entries in
``loader/entries/``.

Configuration
-------------

Enable BLS support with::

    CONFIG_BOOTMETH_BLS=y

This automatically selects ``CONFIG_PXE_UTILS`` for boot execution.

BLS Entry Format
----------------

BLS entries use a simple key-value format, one field per line. Lines starting
with ``#`` are comments. Example::

    title Fedora Linux 39
    version 6.7.0-1.fc39.x86_64
    options root=/dev/sda3 ro quiet
    linux /vmlinuz-6.7.0-1.fc39.x86_64
    initrd /initramfs-6.7.0-1.fc39.x86_64.img
    devicetree /dtbs/6.7.0-1.fc39.x86_64/board.dtb

Supported Fields
----------------

**Required (at least one):**

* ``linux`` - Path to Linux kernel image (Type #1); supports FITs with
  ``path#config`` syntax

**Optional:**

* ``title`` - Human-readable menu display name
* ``version`` - OS version identifier (parsed but not used for sorting)
* ``options`` - Kernel command line parameters (may appear multiple times; all
  occurrences are concatenated)
* ``initrd`` - Initial ramdisk path (may appear multiple times, but only first
  is used due to PXE limitation)
* ``devicetree`` - Device tree blob path
* ``devicetree-overlay`` - Device tree overlays (parsed but not yet supported)
* ``architecture`` - Target architecture (parsed but not used for filtering)
* ``machine-id`` - OS identifier (parsed but not used for filtering)
* ``sort-key`` - Primary sorting key (parsed but not used for sorting)

**Not supported (out of scope for Type #1):**

* ``efi`` - EFI program path (Type #2/UKI)
* ``uki`` - Unified Kernel Image path
* ``uki-url`` - Remote UKI reference
* ``profile`` - Multi-profile UKI selector

FIT Support
-----------

U-Boot's BLS implementation works seamlessly with FITs using the standard
``path#config`` syntax in the ``linux`` field::

    linux /boot/image.fit#config-1

The PXE boot infrastructure handles FIT parsing automatically.

Multiple Values
---------------

Fields that support multiple occurrences:

* ``options`` - All values are concatenated with spaces
* ``initrd`` - Multiple paths can be specified, but only the first is used
  (limitation of PXE boot infrastructure)

Usage
-----

BLS boot entries are discovered automatically during standard boot::

    => bootflow scan
    => bootflow list
    => bootflow select 0
    => bootflow boot

The BLS entry at ``loader/entry.conf`` is discovered as a bootflow.

Implementation Notes
--------------------

* Single BLS entry file support (``loader/entry.conf``)
* Boot execution reuses U-Boot's PXE infrastructure for kernel loading
* Unknown fields are ignored for forward compatibility
* The bootmethod is ordered as ``bootmeth_2bls`` (after extlinux)
* Zero-copy parsing: most fields point into bootflow buffer (except ``options``
  which is allocated for concatenation)

Current Limitations
-------------------

* Only single entry file, not multiple entries directory scanning
* Only first initrd used (PXE infrastructure limitation)
* No devicetree-overlay support
* No architecture/machine-id filtering
* No version-based or sort-key sorting
* No UKI/Type #2 support

See Also
--------

* doc/develop/bootstd.rst - Standard boot framework
* doc/usage/cmd/bootflow.rst - Bootflow command reference

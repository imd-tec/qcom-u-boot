.. SPDX-License-Identifier: GPL-2.0+:

BLS Bootmeth
============

The `Boot Loader Specification`_ (BLS) bootmeth allows U-Boot to boot
operating systems configured with BLS Type #1 entries. This format is used by
Fedora, RHEL and other distributions.

.. _Boot Loader Specification: https://uapi-group.org/specifications/specs/boot_loader_specification/

The entry file ``loader/entry.conf`` is searched for under each boot prefix
(``{"/", "/boot"}`` by default). These prefixes can be selected with the
`filename-prefixes` property in the bootstd device.

When invoked on a bootdev, the ``bls_read_bootflow()`` function searches for the
entry file, reads it and passes it to ``bls_parse_entry()`` which processes
the key-value pairs into a ``struct bls_entry``. The parser uses an enum-based
token lookup to map field names, with most values pointing directly into the
bootflow buffer (zero-copy). Only ``options`` is allocated separately since
multiple occurrences are concatenated. Unknown fields are silently ignored for
forward compatibility. Images (kernel, initrd, devicetree) are registered in the
bootflow with ``bootflow_img_add()`` during discovery but not loaded until boot.

At boot time, ``bls_to_pxe_label()`` converts the bootflow into a PXE label
structure, mapping BLS fields to their PXE equivalents (``title`` to menu,
``options`` to append, etc.). The existing ``pxe_load_files()`` and
``pxe_boot()`` infrastructure then handles file loading and execution, including
FIT support.

The compatible string "u-boot,boot-loader-specification" is used for the driver.
It is present if `CONFIG_BOOTMETH_BLS` is enabled.

See :doc:`/usage/bls` for usage details and field reference.

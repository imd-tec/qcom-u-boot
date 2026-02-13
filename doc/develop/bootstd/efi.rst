.. SPDX-License-Identifier: GPL-2.0+:

EFI Bootmeth
=============

The EFI bootmeth allows U-Boot to boot an operating system by loading and
running an EFI application from a disk or network device. This follows the
approach used by many Linux distributions to provide a standard boot path.

When invoked on a block device, ``distro_efi_try_bootflow_files()`` searches for
the architecture-specific EFI binary in ``/EFI/BOOT/`` (e.g.
``bootaa64.efi``). If found, the bootflow is marked as ready. The function also
tries to locate a matching device tree using ``efi_get_distro_fdt_name()``.

When invoked on a network device, ``distro_efi_read_bootflow_net()`` performs a
DHCP request with PXE vendor-class and architecture identifiers, then retrieves
the EFI binary via TFTP. A device tree is also fetched if available.

At boot time, ``distro_efi_boot()`` loads the EFI binary into memory (for block
devices) and calls ``efi_bootflow_run()`` to execute it via the EFI loader.

The compatible string "u-boot,distro-efi" is used for the driver. It is present
if `CONFIG_BOOTMETH_EFI` is enabled.

See :doc:`/develop/uefi/uefi` for general UEFI implementation details.

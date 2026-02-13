.. SPDX-License-Identifier: GPL-2.0+:

EFI Boot Manager Bootmeth
=========================

The EFI boot-manager bootmeth delegates boot-device selection to the UEFI boot
manager. Rather than scanning filesystems for a specific binary, it checks
whether a ``BootOrder`` EFI variable exists and, if so, marks the bootflow as
ready.

This is a global bootmeth: it is not tied to a particular bootdev but is
invoked once during each scan. The ``BOOTMETHF_GLOBAL`` flag is set at bind
time, and the global priority is ``BOOTDEVP_6_NET_BASE`` so that it runs just
before very slow devices, giving filesystem-based methods a chance to complete
first.

During discovery, ``efi_mgr_read_bootflow()`` initialises the EFI object list
and looks up the ``BootOrder`` variable. If the variable is present the
bootflow is marked ready; otherwise the method is skipped.

At boot time, ``efi_mgr_boot()`` calls ``efi_bootmgr_run()`` which walks the
``BootOrder`` list and launches the first viable EFI application. No file
loading is done by the bootmeth itself.

The compatible string "u-boot,efi-bootmgr" is used for the driver. It is
present if `CONFIG_BOOTMETH_EFI_BOOTMGR` is enabled.

See :doc:`/develop/uefi/uefi` for general UEFI implementation details and
:doc:`/usage/cmd/eficonfig` for configuring boot entries.

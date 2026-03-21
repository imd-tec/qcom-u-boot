.. SPDX-License-Identifier: GPL-2.0+:

Extlinux Bootmeth
=================

`Extlinux <https://uapi-group.org/specifications/specs/boot_loader_specification>`_
(sometimes called syslinux) allows U-Boot to provide a menu of available
operating systems from which the user can choose.

U-Boot includes a parser for the `extlinux.conf` file. It consists primarily of
a list of named operating systems along with the kernel, initial ramdisk and
other settings. The file is stored in the `extlinux/` subdirectory, possibly
under the `boot/` subdirectory. This list of prefixes (``{"/", "/boot"}`` by
default) can be selected with the `filename-prefixes` property in the bootstd
device.

Note that the :doc:`pxelinux` uses the same file format, but in a
network context.

When invoked on a bootdev, this bootmeth searches for the file and creates a
bootflow for each label defined in the configuration. Since an extlinux config
can contain several labels (each pointing to a different kernel), the bootmeth
sets the ``BOOTMETHF_MULTI`` flag so that the iterator produces one bootflow per
label. The ``bflow->entry`` field selects which label to use. Include directives
are processed during scanning so labels from included files are also discovered.

When the bootflow is booted, ``pxe_boot_entry()`` parses the config, walks to
the selected label and boots it directly.

The compatible string "u-boot,extlinux" is used for the driver. It is present
if `CONFIG_BOOTMETH_EXTLINUX` is enabled.

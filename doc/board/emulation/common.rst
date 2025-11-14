.. SPDX-License-Identifier: GPL-2.0+

Common features
===============

It is possible to specify the boot command directly using the fw_cfg interface.
This allows QEMU to control the boot command, which can be useful for automated
testing or scripting. To use this feature, create a file containing the boot
command and pass it to QEMU using the fw_cfg option.

Here is an x86 example::

   $ echo "qfw load; zboot 01000000 - 04000000 1b1ab50" > bootcmd.txt
   $ qemu-system-x86_64 -nographic -bios path/to/u-boot.rom \
     -fw_cfg name=opt/u-boot/bootcmd,file=bootcmd.txt

U-Boot will read the boot command from the firmware configuration and execute it
automatically during the boot process. This bypasses the normal distro boot
sequence.

Note that the boot command is limited in length and should not exceed the boot
command buffer size. If the command is too long, U-Boot will fail to read it and
fall back to the default boot behavior.

The :doc:`script` and build-efi scripts provide a `-c` option for this feature,
although it uses a string rather than a file.

Note that ``CONFIG_QFW`` must be enabled for this feature to work.

# SPDX-License-Identifier: GPL-2.0+
# Copyright 2026 Canonical Ltd

"""Create BLS test disk images"""

import gzip
import os

import utils
from fs_helper import DiskHelper, FsHelper
from img.common import mkdir_cond


def setup_bls_image(config, log, devnum, basename):
    """Create a 20MB BLS disk image with a single FAT partition

    Args:
        config (ArbitraryAttributeContainer): Configuration
        log (multiplexed_log.Logfile): Log to write to
        devnum (int): Device number to use, e.g. 5
        basename (str): Base name to use in the filename, e.g. 'mmc'
    """
    vmlinux = 'vmlinuz-6.8.0'
    initrd = 'initrd.img-6.8.0'
    dtb = 'sandbox.dtb'

    # BLS Type #1 entry format
    script = f'''title Test Boot
version 6.8.0
linux /{vmlinux}
options root=/dev/mmcblk0p2 ro quiet
initrd /{initrd}
devicetree /{dtb}'''

    fsh = FsHelper(config, 'vfat', 18, prefix=basename)
    fsh.setup()

    # Create loader directory for BLS entry
    loader = os.path.join(fsh.srcdir, 'loader')
    mkdir_cond(loader)

    # Create BLS entry file
    conf = os.path.join(loader, 'entry.conf')
    with open(conf, 'w', encoding='ascii') as fd:
        print(script, file=fd)

    # Create compressed kernel image
    inf = os.path.join(config.persistent_data_dir, 'inf')
    with open(inf, 'wb') as fd:
        fd.write(gzip.compress(b'vmlinux'))
    mkimage = config.build_dir + '/tools/mkimage'
    utils.run_and_log_no_ubman(
        log, f'{mkimage} -f auto -d {inf} {os.path.join(fsh.srcdir, vmlinux)}')

    # Create initrd file
    with open(os.path.join(fsh.srcdir, initrd), 'w', encoding='ascii') as fd:
        print('initrd', file=fd)

    # Create device tree blob
    dtb_file = os.path.join(fsh.srcdir, dtb)
    utils.run_and_log_no_ubman(
        log, f'dtc -o {dtb_file}', stdin=b'/dts-v1/; / {};')

    fsh.mk_fs()

    # Create disk image with single bootable partition
    img = DiskHelper(config, devnum, basename)
    img.add_fs(fsh, DiskHelper.VFAT, bootable=True)
    img.create()
    fsh.cleanup()

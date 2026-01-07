# SPDX-License-Identifier: GPL-2.0+
# Copyright 2026 Canonical Ltd

"""
Test the PXE/extlinux parser APIs

These tests verify that the extlinux.conf parser can be used independently
to inspect boot labels without loading kernel/initrd/FDT files.

Tests are implemented in C (test/boot/pxe.c) and called from here.
Python handles filesystem image setup and configuration.
"""

import os
import pytest

from fs_helper import FsHelper


def create_extlinux_conf(srcdir, labels, menu_opts=None):
    """Create an extlinux.conf file with the given labels

    Args:
        srcdir (str): Directory to create the extlinux directory in
        labels (list): List of dicts with label properties:
            - name: Label name (required)
            - menu: Menu label text (optional)
            - kernel: Kernel path (optional)
            - linux: Linux kernel path (alternative to kernel)
            - initrd: Initrd path (optional)
            - append: Kernel arguments (optional)
            - fdt: Device tree path (optional)
            - fdtdir: Device tree directory (optional)
            - fdtoverlays: Device tree overlays (optional)
            - localboot: Local boot flag (optional)
            - ipappend: IP append flags (optional)
            - fit: FIT config path (optional)
            - kaslrseed: Enable KASLR seed (optional)
            - default: If True, this is the default label (optional)
        menu_opts (dict): Menu-level options:
            - title: Menu title
            - timeout: Timeout in tenths of a second
            - prompt: Prompt flag
            - fallback: Fallback label name
            - ontimeout: Label to boot on timeout
            - background: Background image path
            - say: Message to print
            - include: File to include

    Returns:
        str: Path to the config file relative to srcdir
    """
    if menu_opts is None:
        menu_opts = {}

    extdir = os.path.join(srcdir, 'extlinux')
    os.makedirs(extdir, exist_ok=True)

    conf_path = os.path.join(extdir, 'extlinux.conf')
    with open(conf_path, 'w', encoding='ascii') as fd:
        # Menu-level options
        title = menu_opts.get('title', 'Test Boot Menu')
        fd.write(f'menu title {title}\n')
        fd.write(f"timeout {menu_opts.get('timeout', 1)}\n")
        if 'prompt' in menu_opts:
            fd.write(f"prompt {menu_opts['prompt']}\n")
        if 'fallback' in menu_opts:
            fd.write(f"fallback {menu_opts['fallback']}\n")
        if 'ontimeout' in menu_opts:
            fd.write(f"ontimeout {menu_opts['ontimeout']}\n")
        if 'background' in menu_opts:
            fd.write(f"menu background {menu_opts['background']}\n")
        if 'say' in menu_opts:
            fd.write(f"say {menu_opts['say']}\n")

        for label in labels:
            if label.get('default'):
                fd.write(f"default {label['name']}\n")

        for label in labels:
            fd.write(f"\nlabel {label['name']}\n")
            if 'menu' in label:
                fd.write(f"    menu label {label['menu']}\n")
            if 'kernel' in label:
                fd.write(f"    kernel {label['kernel']}\n")
            if 'linux' in label:
                fd.write(f"    linux {label['linux']}\n")
            if 'initrd' in label:
                fd.write(f"    initrd {label['initrd']}\n")
            if 'append' in label:
                fd.write(f"    append {label['append']}\n")
            if 'fdt' in label:
                fd.write(f"    fdt {label['fdt']}\n")
            if 'fdtdir' in label:
                fd.write(f"    fdtdir {label['fdtdir']}\n")
            if 'fdtoverlays' in label:
                fd.write(f"    fdtoverlays {label['fdtoverlays']}\n")
            if 'localboot' in label:
                fd.write(f"    localboot {label['localboot']}\n")
            if 'ipappend' in label:
                fd.write(f"    ipappend {label['ipappend']}\n")
            if 'fit' in label:
                fd.write(f"    fit {label['fit']}\n")
            if label.get('kaslrseed'):
                fd.write("    kaslrseed\n")
            if 'say' in label:
                fd.write(f"    say {label['say']}\n")

        # Write include at the end so included labels come after main labels
        if 'include' in menu_opts:
            fd.write(f"\ninclude {menu_opts['include']}\n")

    return '/extlinux/extlinux.conf'


@pytest.fixture
def pxe_image(u_boot_config):
    """Create a filesystem image with an extlinux.conf file"""
    fsh = FsHelper(u_boot_config, 'vfat', 4, prefix='pxe_test')
    fsh.setup()

    # Create a simple extlinux.conf with multiple labels
    labels = [
        {
            'name': 'linux',
            'menu': 'Boot Linux',
            'kernel': '/vmlinuz',
            'initrd': '/initrd.img',
            'append': 'root=/dev/sda1 quiet',
            'fdt': '/dtb/board.dtb',
            'fdtoverlays': '/dtb/overlay1.dtbo /dtb/overlay2.dtbo',
            'kaslrseed': True,
            'say': 'Booting default Linux kernel',
            'default': True,
        },
        {
            'name': 'rescue',
            'menu': 'Rescue Mode',
            'linux': '/vmlinuz-rescue',  # test 'linux' keyword
            'append': 'single',
            'fdtdir': '/dtb/',
            'ipappend': '3',
        },
        {
            'name': 'local',
            'menu': 'Local Boot',
            'localboot': '1',
        },
        {
            'name': 'fitboot',
            'menu': 'FIT Boot',
            'fit': '/boot/image.fit#config-1',
            'append': 'console=ttyS0',
        },
    ]

    menu_opts = {
        'title': 'Test Boot Menu',
        'timeout': 50,
        'prompt': 1,
        'fallback': 'rescue',
        'ontimeout': 'linux',
        'background': '/boot/background.bmp',
        'include': '/extlinux/extra.conf',
    }

    cfg_path = create_extlinux_conf(fsh.srcdir, labels, menu_opts)

    # Create a chain of 16 nested include files to test MAX_NEST_LEVEL
    # Level 1 is extlinux.conf, levels 2-16 are extra.conf, nest3.conf, etc.
    for level in range(2, 17):
        if level == 2:
            fname = 'extra.conf'
            label_name = 'included'
            label_menu = 'Included Label'
        else:
            fname = f'nest{level}.conf'
            label_name = f'level{level}'
            label_menu = f'Level {level} Label'

        fpath = os.path.join(fsh.srcdir, 'extlinux', fname)
        with open(fpath, 'w', encoding='ascii') as fd:
            fd.write(f"# Level {level} configuration\n")
            fd.write(f"label {label_name}\n")
            fd.write(f"    menu label {label_menu}\n")
            fd.write(f"    kernel /boot/{label_name}-kernel\n")
            fd.write(f"    append root=/dev/sd{chr(ord('a') + level - 1)}1\n")
            # Include next level unless we're at level 16
            if level < 16:
                next_fname = f'nest{level + 1}.conf'
                fd.write(f"\ninclude /extlinux/{next_fname}\n")

    # Create the filesystem
    fsh.mk_fs()

    yield fsh.fs_img, cfg_path

    # Cleanup
    if not u_boot_config.persist:
        fsh.cleanup()


@pytest.mark.boardspec('sandbox')
class TestPxeParser:
    """Test PXE/extlinux parser APIs via C unit tests"""

    def test_pxe_parse(self, ubman, pxe_image):
        """Test parsing an extlinux.conf and verifying label properties"""
        fs_img, cfg_path = pxe_image
        with ubman.log.section('Test PXE parse'):
            ubman.run_ut('pxe', 'pxe_test_parse',
                         fs_image=fs_img, cfg_path=cfg_path)

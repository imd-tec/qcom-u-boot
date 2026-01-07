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
import subprocess

from fs_helper import FsHelper


# Simple base DTS with symbols enabled (for overlay support)
BASE_DTS = """\
/dts-v1/;

/ {
	model = "Test Board";
	compatible = "test,board";

	test: test-node {
		test-property = <42>;
		status = "okay";
	};
};
"""

# Simple overlay that modifies the test node
OVERLAY1_DTS = """\
/dts-v1/;
/plugin/;

&test {
	overlay1-property = "from-overlay1";
};
"""

# Another overlay that adds a different property
OVERLAY2_DTS = """\
/dts-v1/;
/plugin/;

&test {
	overlay2-property = "from-overlay2";
};
"""


def compile_dts(dts_content, output_path, is_overlay=False):
    """Compile DTS content to DTB/DTBO file

    Args:
        dts_content (str): DTS source content
        output_path (str): Path to output DTB/DTBO file
        is_overlay (bool): True if this is an overlay (needs -@)

    Raises:
        subprocess.CalledProcessError: If dtc fails
    """
    # Use -@ for both base (to generate __symbols__) and overlays
    cmd = ['dtc', '-@', '-I', 'dts', '-O', 'dtb', '-o', output_path]
    subprocess.run(cmd, input=dts_content.encode(), check=True,
                   capture_output=True)


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
            - devicetree: Device tree path (alias for fdt)
            - fdtdir: Device tree directory (optional)
            - devicetreedir: Device tree directory (alias for fdtdir)
            - fdtoverlays: Device tree overlays (optional)
            - devicetree-overlay: Device tree overlays (alias)
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
            if 'devicetree' in label:
                fd.write(f"    devicetree {label['devicetree']}\n")
            if 'fdtdir' in label:
                fd.write(f"    fdtdir {label['fdtdir']}\n")
            if 'devicetreedir' in label:
                fd.write(f"    devicetreedir {label['devicetreedir']}\n")
            if 'fdtoverlays' in label:
                fd.write(f"    fdtoverlays {label['fdtoverlays']}\n")
            if 'devicetree-overlay' in label:
                fd.write(f"    devicetree-overlay {label['devicetree-overlay']}\n")
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
            # Use aliases to test devicetree/devicetree-overlay keywords
            'devicetree': '/dtb/board.dtb',
            'devicetree-overlay': '/dtb/overlay1.dtbo /dtb/overlay2.dtbo',
            'kaslrseed': True,
            'say': 'Booting default Linux kernel',
            'default': True,
        },
        {
            'name': 'rescue',
            'menu': 'Rescue Mode',
            'linux': '/vmlinuz-rescue',  # test 'linux' keyword
            'append': 'single',
            'devicetreedir': '/dtb/',  # test alias for fdtdir
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

    # Create DTB and overlay files for testing
    dtbdir = os.path.join(fsh.srcdir, 'dtb')
    os.makedirs(dtbdir, exist_ok=True)
    compile_dts(BASE_DTS, os.path.join(dtbdir, 'board.dtb'))
    compile_dts(OVERLAY1_DTS, os.path.join(dtbdir, 'overlay1.dtbo'),
                is_overlay=True)
    compile_dts(OVERLAY2_DTS, os.path.join(dtbdir, 'overlay2.dtbo'),
                is_overlay=True)

    # Create dummy kernel and initrd files with identifiable content
    with open(os.path.join(fsh.srcdir, 'vmlinuz'), 'wb') as fd:
        fd.write(b'kernel')
    with open(os.path.join(fsh.srcdir, 'vmlinuz-rescue'), 'wb') as fd:
        fd.write(b'rescue')
    with open(os.path.join(fsh.srcdir, 'initrd.img'), 'wb') as fd:
        fd.write(b'ramdisk')

    # Create the filesystem
    fsh.mk_fs()

    yield fsh.fs_img, cfg_path

    # Cleanup
    if not u_boot_config.persist:
        fsh.cleanup()


@pytest.fixture
def pxe_fdtdir_image(u_boot_config):
    """Create a filesystem image with fdtdir-based configuration

    This tests the fdtdir path-resolution logic where the FDT filename
    is constructed from environment variables.
    """
    fsh = FsHelper(u_boot_config, 'vfat', 4, prefix='pxe_fdtdir')
    fsh.setup()

    # Create labels using fdtdir instead of explicit fdt path
    labels = [
        {
            'name': 'fdtfile-test',
            'menu': 'Test fdtfile env var',
            'kernel': '/vmlinuz',
            'append': 'console=ttyS0',
            'fdtdir': '/dtb/',  # Will use fdtfile env var
            'fdtoverlays': '/dtb/overlay1.dtbo',
            'default': True,
        },
        {
            'name': 'socboard-test',
            'menu': 'Test soc/board construction',
            'kernel': '/vmlinuz',
            'append': 'console=ttyS0',
            'fdtdir': '/dtb',  # No trailing slash - tests slash insertion
        },
    ]

    cfg_path = create_extlinux_conf(fsh.srcdir, labels)

    # Create DTB directory with files for different naming conventions
    dtbdir = os.path.join(fsh.srcdir, 'dtb')
    os.makedirs(dtbdir, exist_ok=True)

    # DTB for fdtfile env var test (fdtfile=test-board.dtb)
    compile_dts(BASE_DTS, os.path.join(dtbdir, 'test-board.dtb'))

    # DTB for soc-board construction (soc=tegra, board=jetson)
    compile_dts(BASE_DTS, os.path.join(dtbdir, 'tegra-jetson.dtb'))

    # Overlay for fdtdir test
    compile_dts(OVERLAY1_DTS, os.path.join(dtbdir, 'overlay1.dtbo'),
                is_overlay=True)

    # Create dummy kernel
    with open(os.path.join(fsh.srcdir, 'vmlinuz'), 'wb') as fd:
        fd.write(b'kernel')

    fsh.mk_fs()

    yield fsh.fs_img, cfg_path

    if not u_boot_config.persist:
        fsh.cleanup()


@pytest.fixture
def pxe_error_image(u_boot_config):
    """Create a filesystem image for testing error handling

    This tests various error conditions:
    - Explicit FDT file that doesn't exist (should fail label)
    - fdtdir with missing FDT file (should continue)
    - Missing overlay file (should continue)
    """
    fsh = FsHelper(u_boot_config, 'vfat', 4, prefix='pxe_error')
    fsh.setup()

    labels = [
        {
            # Explicit FDT that doesn't exist - should fail this label
            'name': 'missing-fdt',
            'menu': 'Missing explicit FDT',
            'kernel': '/vmlinuz',
            'fdt': '/dtb/nonexistent.dtb',
            'default': True,
        },
        {
            # fdtdir with missing FDT - should warn but continue
            'name': 'missing-fdtdir',
            'menu': 'Missing fdtdir FDT',
            'kernel': '/vmlinuz',
            'fdtdir': '/dtb/',
        },
        {
            # Valid FDT but missing overlay - should continue
            'name': 'missing-overlay',
            'menu': 'Missing overlay',
            'kernel': '/vmlinuz',
            'fdt': '/dtb/board.dtb',
            'fdtoverlays': '/dtb/nonexistent.dtbo /dtb/overlay1.dtbo',
        },
    ]

    cfg_path = create_extlinux_conf(fsh.srcdir, labels)

    # Create DTB directory with only some files
    dtbdir = os.path.join(fsh.srcdir, 'dtb')
    os.makedirs(dtbdir, exist_ok=True)

    # Only create board.dtb and overlay1.dtbo - others are missing
    compile_dts(BASE_DTS, os.path.join(dtbdir, 'board.dtb'))
    compile_dts(OVERLAY1_DTS, os.path.join(dtbdir, 'overlay1.dtbo'),
                is_overlay=True)

    # Create dummy kernel
    with open(os.path.join(fsh.srcdir, 'vmlinuz'), 'wb') as fd:
        fd.write(b'kernel')

    fsh.mk_fs()

    yield fsh.fs_img, cfg_path

    if not u_boot_config.persist:
        fsh.cleanup()


@pytest.mark.boardspec('sandbox')
@pytest.mark.requiredtool('dtc')
class TestPxeParser:
    """Test PXE/extlinux parser APIs via C unit tests"""

    def test_pxe_parse(self, ubman, pxe_image):
        """Test parsing an extlinux.conf and verifying label properties"""
        fs_img, cfg_path = pxe_image
        with ubman.log.section('Test PXE parse'):
            ubman.run_ut('pxe', 'pxe_test_parse',
                         fs_image=fs_img, cfg_path=cfg_path)

    def test_pxe_sysboot(self, ubman, pxe_image):
        """Test booting via sysboot command"""
        fs_img, cfg_path = pxe_image
        with ubman.log.section('Test PXE sysboot'):
            ubman.run_ut('pxe', 'pxe_test_sysboot',
                         fs_image=fs_img, cfg_path=cfg_path)

    def test_pxe_fdtdir(self, ubman, pxe_fdtdir_image):
        """Test fdtdir path resolution with fdtfile and soc/board env vars"""
        fs_img, cfg_path = pxe_fdtdir_image
        with ubman.log.section('Test PXE fdtdir'):
            ubman.run_ut('pxe', 'pxe_test_fdtdir',
                         fs_image=fs_img, cfg_path=cfg_path)

    def test_pxe_errors(self, ubman, pxe_error_image):
        """Test error handling for missing FDT and overlay files"""
        fs_img, cfg_path = pxe_error_image
        with ubman.log.section('Test PXE errors'):
            ubman.run_ut('pxe', 'pxe_test_errors',
                         fs_image=fs_img, cfg_path=cfg_path)

    def test_pxe_pxelinux_path(self, ubman, pxe_image):
        """Test get_pxelinux_path() path length checking"""
        fs_img, cfg_path = pxe_image
        with ubman.log.section('Test PXE pxelinux path'):
            ubman.run_ut('pxe', 'pxe_test_pxelinux_path',
                         fs_image=fs_img)

    def test_pxe_ipappend(self, ubman, pxe_image):
        """Test ipappend functionality for IP and MAC appending"""
        fs_img, cfg_path = pxe_image
        with ubman.log.section('Test PXE ipappend'):
            ubman.run_ut('pxe', 'pxe_test_ipappend',
                         fs_image=fs_img, cfg_path=cfg_path)

    def test_pxe_label_override(self, ubman, pxe_image):
        """Test pxe_label_override environment variable"""
        fs_img, cfg_path = pxe_image
        with ubman.log.section('Test PXE label override'):
            ubman.run_ut('pxe', 'pxe_test_label_override',
                         fs_image=fs_img, cfg_path=cfg_path)

    def test_pxe_alloc(self, ubman, pxe_image):
        """Test file loading with no address env vars (LMB allocation path)"""
        fs_img, cfg_path = pxe_image
        with ubman.log.section('Test PXE alloc'):
            ubman.run_ut('pxe', 'pxe_test_alloc',
                         fs_image=fs_img, cfg_path=cfg_path)

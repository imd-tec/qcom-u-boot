# SPDX-License-Identifier: GPL-2.0+
# Copyright 2025 Canonical Ltd
# Written by Simon Glass <simon.glass@canonical.com>

"""Test for FIT image printing"""

import os

import pytest

import fit_util
import utils

# ITS for testing FIT printing with hashes, ramdisk, and multiple configs
PRINT_ITS = '''
/dts-v1/;

/ {
	description = "Test FIT image for printing";
	#address-cells = <1>;

	images {
		kernel {
			description = "Test kernel";
			data = /incbin/("%(kernel)s");
			type = "kernel";
			arch = "sandbox";
			os = "linux";
			compression = "gzip";
			load = <0x1000000>;
			entry = <0x1000000>;
			hash-1 {
				algo = "sha256";
			};
		};
		ramdisk {
			description = "Test ramdisk";
			data = /incbin/("%(ramdisk)s");
			type = "ramdisk";
			arch = "sandbox";
			os = "linux";
			compression = "none";
			load = <0x2000000>;
			hash-1 {
				algo = "sha256";
			};
		};
		fdt {
			description = "Test FDT";
			data = /incbin/("%(fdt)s");
			type = "flat_dt";
			arch = "sandbox";
			compression = "none";
			hash-1 {
				algo = "sha256";
			};
		};
	};
	configurations {
		default = "conf-1";
		conf-1 {
			description = "Test configuration";
			kernel = "kernel";
			fdt = "fdt";
			ramdisk = "ramdisk";
		};
		conf-2 {
			description = "Alternate configuration";
			kernel = "kernel";
			fdt = "fdt";
		};
	};
};
'''

@pytest.mark.boardspec('sandbox')
@pytest.mark.buildconfigspec('fit_print')
@pytest.mark.requiredtool('dtc')
def test_fit_print(ubman):
    """Test fit_print_contents() via C unit test"""
    mkimage = os.path.join(ubman.config.build_dir, 'tools/mkimage')

    # Create test files (make kernel ~6.3K)
    kernel = fit_util.make_kernel(ubman, 'test-kernel.bin',
                                  'kernel with some extra test data')

    # Compress the kernel (with -n to avoid timestamps for reproducibility)
    kernel_gz = kernel + '.gz'
    utils.run_and_log(ubman, ['gzip', '-f', '-n', '-k', kernel])

    fdt = fit_util.make_dtb(ubman, '''
/dts-v1/;
/ {
	#address-cells = <1>;
	#size-cells = <0>;
	model = "Test";
};
''', 'test-fdt')
    ramdisk = fit_util.make_kernel(ubman, 'test-ramdisk.bin', 'ramdisk')

    # Compress the ramdisk (with -n to avoid timestamps for reproducibility)
    ramdisk_gz = ramdisk + '.gz'
    utils.run_and_log(ubman, ['gzip', '-f', '-n', '-k', ramdisk])

    # Create FIT image with fixed timestamp for reproducible output
    params = {
        'kernel': kernel_gz,
        'fdt': fdt,
        'ramdisk': ramdisk_gz,
    }
    env = os.environ.copy()
    env['SOURCE_DATE_EPOCH'] = '1234567890'  # 2009-02-13 23:31:30 UTC
    fit = os.path.join(ubman.config.persistent_data_dir, 'test-fit.fit')
    its = fit_util.make_its(ubman, PRINT_ITS, params)
    utils.run_and_log(ubman, [mkimage, '-f', its, fit], env=env)

    # Run the C test which will load and verify this FIT
    ubman.run_command('ut -f bootstd test_fit_print_norun')
    result = ubman.run_command('echo $?')
    assert '0' == result

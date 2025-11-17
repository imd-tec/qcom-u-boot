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
			signature {
				algo = "sha256,rsa2048";
				padding = "pkcs-1.5";
				key-name-hint = "test-key";
				sign-images = "fdt", "kernel", "ramdisk";
			};
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
@pytest.mark.requiredtool('openssl')
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

    # Use a fixed RSA key pair for reproducible signatures
    tmpdir = ubman.config.result_dir + '/'

    # Fixed 2048-bit RSA private key for testing (for reproducible signatures)
    key_pem = '''-----BEGIN PRIVATE KEY-----
MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQDU18AB+xpQw+GX
mywzH4nsEIECgLVnBTNaAnE4XSIqbiviZetumBP6Ib2W+0OIOn8/hIh3UnzzyWIP
aRus94CVfFQPqwhi6/M9ptL7N7lCXq9DwQ0EY55GquwoO9jZnnDsCSU76jgKg+Nc
dsbvprfFDxBjkrLBfdEgzJtNUaJnUCd58RG8sII7EOP4JTGnXn2wVTsKYcTmr8y6
bOZTUQfsYj9BGFTbskkLYj1RJ6Dpzk4yBqyUn4fUYhfqsAHwlJs/64Byx2m7J7Ia
rfp49NkqgOFlTvDzKnecxGt4pmgEA+4MtRxUFDliZ/bG3TvG/xNlXvWaHp9DG05u
4h9jy2NPAgMBAAECggEAAMZKOheeWGXmF8WmSwdV2qiSt54dSuMvdSfmHpTkL3BY
M4o4aZ4fEH138ak3bTL9TI9gacLAlqiIdVLmGWKLMsARlD8EmEuQhoxpXyWsRGwQ
yjfVIst0A4DSvDC/kMctVQaRfp7TFmK1fJwoDC44o/xyjFI32VFqZeqotAbUhvi3
gIYvP5Q4Kvbaq9aZNURqazJHuEVD9LpwbnroUd4cBrcorstJzaDmTIyb5swLX+IX
FjMOVtHtBDKOG9Ce1wlEOXZtSsoZtAEgkd1IQYBCTBUDkxPdx+ZKPdfT4aKWX3S4
WQ65lDEGAnplMmetFRV+k9NNJvEia9JoX/SJqhUWGQKBgQD1/rffQZnFWqGM2dD1
CEkXpCN23xAEaZjQtuIhPMBWEWufAPZhyZSbq3eLjcqSS5mzU8B+n1c9Zxw6r0qM
BXlcUftreFPKvEXeyp1YWh7loxHiVVuasp2lEDx4arwUrI61XtAaixUb9Opxxj/x
UDrY5cj7BIRhrkDZtnor/EbRaQKBgQDdf9pymbbxRmHHFGERSzo6/gbr1GRK9fUA
ZNrzfBM5Sdvmm2aKgYd7hIKhOgeKIkS858gEOsRw75x6nvlrjZvFZGIfetXXxaN9
c6Uqq/f6rTRUTB9/SqvMgKZMuJ2SFms8I1nbxSE/PMD0T6TRbhjaFoZwZP42HVsM
wAN2Oiq/9wKBgQC7sQHyYkdFgYVJxtfcXdoHI8G7bS73buqeNSwMWCIYiWooA7/5
lKjCre2kmSc6wFwhq4FwG3ug6g9r51tlwrd6bUL8GO81/LkC6G1tgDWa2PVIUAB4
5FfMHbtF1Ypz68VnNVRrLDuK/S/0Z2NaZ/C+lXTnseaf8Sih9Mz6yp3uIQKBgBc4
61cuhH6hSWkM2uxsPaunrGQXPXiadthWupnifUV5V+PCkSqeT+0ERInQwq+Zzikc
B91hp+zLQlWcyzuaeiVk0+DHCRp5Lx3c/QkPRI10kVLxNDAtTPvA1S6gAG0rioyg
jDA9Z7Hwla5Hl1kZuONMj0XDYN+djkk07Gf9yzObAoGAbiS3mRID0pLFhWR1L64h
NlRJpZjsHNRPd0WFVxXnJRzZxkStoTwL2BhPtG3Xx1ReIkNVCxlu1Dk0rLLKl1nj
4B/X9Qu6aejXnOsbqp1/JBXYxD8l5B2yg5//wz18um/SOSagpAPeH4i/V3NxOup5
S0n8gbs0Ht/ZckLk8mPclbk=
-----END PRIVATE KEY-----'''

    with open(tmpdir + 'test-key.key', 'w', encoding='utf-8') as f:
        f.write(key_pem)

    utils.run_and_log(ubman,
        f'openssl req -batch -new -x509 -key {tmpdir}test-key.key '
        f'-out {tmpdir}test-key.crt')

    # Create a dummy DTB for the public key
    dtb = fit_util.make_fname(ubman, 'test-key.dtb')
    utils.run_and_log(ubman, ['dtc', '-I', 'dts', '-O', 'dtb', '-o', dtb],
                      stdin=b'/dts-v1/; / { };')

    # Sign the FIT configuration (use env for reproducible timestamp)
    utils.run_and_log(ubman, [mkimage, '-F', '-k', tmpdir, '-K', dtb,
                              '-r', fit, '-c', 'Configuration signing'],
                      env=env)

    # Run the C test which will load and verify this FIT
    ubman.run_command('ut -f bootstd test_fit_print_norun')
    result = ubman.run_command('echo $?')
    assert '0' == result

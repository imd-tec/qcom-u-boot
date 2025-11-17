// SPDX-License-Identifier: GPL-2.0+
/*
 * Test for FIT image printing
 *
 * Copyright 2025 Canonical Ltd
 * Written by Simon Glass <simon.glass@canonical.com>
 */

#include <image.h>
#include <mapmem.h>
#include <os.h>
#include <test/ut.h>
#include <linux/libfdt.h>
#include "bootstd_common.h"

/* Test fit_print_contents() output */
static int test_fit_print_norun(struct unit_test_state *uts)
{
	char fname[256];
	void *fit;
	void *buf;
	ulong addr;
	int size;

	/* Load the FIT created by the Python test */
	ut_assertok(os_persistent_file(fname, sizeof(fname), "test-fit.fit"));
	ut_assertok(os_read_file(fname, &buf, &size));

	/* Copy to address 0x10000 and print from there */
	addr = 0x10000;
	fit = map_sysmem(addr, size);
	memcpy(fit, buf, size);

	/* Print it and check output line by line */
	console_record_reset_enable();
	fit_print_contents(fit);

	/* Check every line of output */
	ut_assert_nextline("   FIT description: Test FIT image for printing");
	ut_assert_nextline("   Created:         2009-02-13  23:31:30 UTC");
	ut_assert_nextline("    Image 0 (kernel)");
	ut_assert_nextline("     Description:  Test kernel");
	ut_assert_nextline("     Created:      2009-02-13  23:31:30 UTC");
	ut_assert_nextline("     Type:         Kernel Image");
	ut_assert_nextline("     Compression:  gzip compressed");
	ut_assert_nextline("     Data Start:   0x000100c4");
	ut_assert_nextline("     Data Size:    327 Bytes = 327 Bytes");
	ut_assert_nextline("     Architecture: Sandbox");
	ut_assert_nextline("     OS:           Linux");
	ut_assert_nextline("     Load Address: 0x01000000");
	ut_assert_nextline("     Entry Point:  0x01000000");
	ut_assert_nextline("     Hash algo:    sha256");
	ut_assert_nextline("     Hash value:   fad998b94ef12fdac0c347915d8b9b6069a4011399e1a2097638a2cb33244cee");
	ut_assert_nextline("    Image 1 (ramdisk)");
	ut_assert_nextline("     Description:  Test ramdisk");
	ut_assert_nextline("     Created:      2009-02-13  23:31:30 UTC");
	ut_assert_nextline("     Type:         RAMDisk Image");
	ut_assert_nextline("     Compression:  uncompressed");
	ut_assert_nextline("     Data Start:   0x00010304");
	ut_assert_nextline("     Data Size:    301 Bytes = 301 Bytes");
	ut_assert_nextline("     Architecture: Sandbox");
	ut_assert_nextline("     OS:           Linux");
	ut_assert_nextline("     Load Address: 0x02000000");
	ut_assert_nextline("     Entry Point:  unavailable");
	ut_assert_nextline("     Hash algo:    sha256");
	ut_assert_nextline("     Hash value:   53e2a65d92ad890dcd89d83a1f95ad6b8206e0e4889548b035062fc494e7f655");
	ut_assert_nextline("    Image 2 (fdt)");
	ut_assert_nextline("     Description:  Test FDT");
	ut_assert_nextline("     Created:      2009-02-13  23:31:30 UTC");
	ut_assert_nextline("     Type:         Flat Device Tree");
	ut_assert_nextline("     Compression:  uncompressed");
	ut_assert_nextline("     Data Start:   0x00010514");
	ut_assert_nextline("     Data Size:    157 Bytes = 157 Bytes");
	ut_assert_nextline("     Architecture: Sandbox");
	ut_assert_nextline("     Hash algo:    sha256");
	ut_assert_nextline("     Hash value:   51918524b06745cae06331047c7e566909431bf71338e5f703dffba1823274f4");
	ut_assert_nextline("    Default Configuration: 'conf-1'");
	ut_assert_nextline("    Configuration 0 (conf-1)");
	ut_assert_nextline("     Description:  Test configuration");
	ut_assert_nextline("     Kernel:       kernel");
	ut_assert_nextline("     Init Ramdisk: ramdisk");
	ut_assert_nextline("     FDT:          fdt");
	ut_assert_nextline("     Sign algo:    sha256,rsa2048:test-key");
	ut_assert_nextline("     Sign padding: pkcs-1.5");
	ut_assert_nextlinen("     Sign value:   9ed5738204714c0ecf46");
	ut_assert_nextline("     Timestamp:    2009-02-13  23:31:30 UTC");
	ut_assert_nextline("    Configuration 1 (conf-2)");
	ut_assert_nextline("     Description:  Alternate configuration");
	ut_assert_nextline("     Kernel:       kernel");
	ut_assert_nextline("     FDT:          fdt");
	ut_assert_console_end();

	os_free(buf);

	return 0;
}
BOOTSTD_TEST(test_fit_print_norun, UTF_CONSOLE | UTF_MANUAL);

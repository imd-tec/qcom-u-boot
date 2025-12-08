// SPDX-License-Identifier: GPL-2.0+
/*
 *  Copyright 2021 Broadcom
 */

#include <command.h>
#include <malloc.h>

static int do_test_stackprot_fail(struct cmd_tbl *cmdtp, int flag, int argc,
				  char *const argv[])
{
	/*
	 * In order to avoid having the compiler optimize away the stack smashing
	 * we need to do a little something here.
	 */
	char a[128];

	/*
	 * Disable backtrace collection before corrupting the stack.
	 * Otherwise, any malloc (e.g., from printf/font rendering) will
	 * attempt to collect a backtrace from the corrupted stack and crash.
	 */
	malloc_backtrace_skip(true);
	memset(a, 0xa5, 512);

	printf("We have smashed our stack as this should not exceed 128: sizeof(a) = %zd\n",
	       strlen(a));

	return 0;
}

U_BOOT_CMD(stackprot_test, 1, 1, do_test_stackprot_fail,
	   "test stack protector fail", "");

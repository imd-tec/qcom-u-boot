// SPDX-License-Identifier: GPL-2.0+
/*
 * Backtrace command
 *
 * Copyright 2025 Canonical Ltd
 * Written by Simon Glass <simon.glass@canonical.com>
 */

#include <backtrace.h>
#include <command.h>

static int do_backtrace(struct cmd_tbl *cmdtp, int flag, int argc,
			char *const argv[])
{
	int ret;

	ret = backtrace_show();
	if (ret) {
		printf("backtrace failed: %d\n", ret);
		return CMD_RET_FAILURE;
	}

	return 0;
}

U_BOOT_CMD(backtrace, 1, 1, do_backtrace,
	   "Print backtrace",
	   "\n"
	   "    - Print a backtrace of the current call stack"
);

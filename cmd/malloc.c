// SPDX-License-Identifier: GPL-2.0+
/*
 * malloc command - show malloc information
 *
 * Copyright 2025 Canonical Ltd
 * Written by Simon Glass <simon.glass@canonical.com>
 */

#include <command.h>
#include <display_options.h>
#include <malloc.h>

static int do_malloc_info(struct cmd_tbl *cmdtp, int flag, int argc,
			  char *const argv[])
{
	struct malloc_info info;
	char buf[12];
	int ret;

	ret = malloc_get_info(&info);
	if (ret)
		return CMD_RET_FAILURE;

	printf("total bytes   = %s\n", format_size(buf, info.total_bytes));
	printf("in use bytes  = %s\n", format_size(buf, info.in_use_bytes));
	printf("malloc count  = %lu\n", info.malloc_count);
	printf("free count    = %lu\n", info.free_count);
	printf("realloc count = %lu\n", info.realloc_count);

	return 0;
}

static int do_malloc_dump(struct cmd_tbl *cmdtp, int flag, int argc,
			  char *const argv[])
{
	malloc_dump();

	return 0;
}

U_BOOT_LONGHELP(malloc,
	"info - display malloc statistics\n"
	"malloc dump - dump heap chunks (address, size, status)\n");

U_BOOT_CMD_WITH_SUBCMDS(malloc, "malloc information", malloc_help_text,
	U_BOOT_SUBCMD_MKENT(info, 1, 1, do_malloc_info),
	U_BOOT_SUBCMD_MKENT(dump, 1, 1, do_malloc_dump));

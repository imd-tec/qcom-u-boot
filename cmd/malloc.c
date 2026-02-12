// SPDX-License-Identifier: GPL-2.0+
/*
 * malloc command - show malloc information
 *
 * Copyright 2025 Canonical Ltd
 * Written by Simon Glass <simon.glass@canonical.com>
 */

#include <command.h>
#include <linux/string.h>
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

static int __maybe_unused do_malloc_log(struct cmd_tbl *cmdtp, int flag,
					int argc, char *const argv[])
{
	if (argc < 2) {
		malloc_log_dump();
		return 0;
	}

	if (!strcmp(argv[1], "start")) {
		malloc_log_start();
		printf("Malloc logging started\n");
	} else if (!strcmp(argv[1], "stop")) {
		malloc_log_stop();
		printf("Malloc logging stopped\n");
	} else if (!strcmp(argv[1], "dump")) {
		malloc_log_dump();
	} else {
		return CMD_RET_USAGE;
	}

	return 0;
}

#if CONFIG_IS_ENABLED(CMD_MALLOC_LOG)
#define MALLOC_LOG_HELP	\
	"malloc log [start|stop|dump] - log malloc traffic\n" \
	"    start - start recording malloc/free calls\n" \
	"    stop  - stop recording\n" \
	"    dump  - print the log (or just 'malloc log')\n"
#define MALLOC_LOG_SUBCMD , U_BOOT_SUBCMD_MKENT(log, 3, 1, do_malloc_log)
#else
#define MALLOC_LOG_HELP
#define MALLOC_LOG_SUBCMD
#endif

U_BOOT_LONGHELP(malloc,
	"info - display malloc statistics\n"
	"malloc dump - dump heap chunks (address, size, status)\n"
	MALLOC_LOG_HELP);

U_BOOT_CMD_WITH_SUBCMDS(malloc, "malloc information", malloc_help_text,
	U_BOOT_SUBCMD_MKENT(info, 1, 1, do_malloc_info),
	U_BOOT_SUBCMD_MKENT(dump, 1, 1, do_malloc_dump)
	MALLOC_LOG_SUBCMD);

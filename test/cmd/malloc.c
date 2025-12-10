// SPDX-License-Identifier: GPL-2.0+
/*
 * Test for 'malloc' command
 *
 * Copyright 2025 Canonical Ltd
 * Written by Simon Glass <simon.glass@canonical.com>
 */

#include <malloc.h>
#include <dm/test.h>
#include <test/cmd.h>
#include <test/ut.h>

/* Test 'malloc info' command */
static int cmd_test_malloc_info(struct unit_test_state *uts)
{
	struct malloc_info info;

	ut_assertok(malloc_get_info(&info));
	ut_assert(info.total_bytes >= CONFIG_SYS_MALLOC_LEN);
	ut_assert(info.in_use_bytes < info.total_bytes);
	ut_assert(info.malloc_count > 0);

	ut_assertok(run_command("malloc info", 0));
	ut_assert_nextlinen("total bytes   = ");
	ut_assert_nextlinen("in use bytes  = ");
	ut_assert_nextlinen("malloc count  = ");
	ut_assert_nextlinen("free count    = ");
	ut_assert_nextlinen("realloc count = ");
	ut_assert_console_end();

	return 0;
}
CMD_TEST(cmd_test_malloc_info, UTF_CONSOLE);

/* Test 'malloc dump' command */
static int cmd_test_malloc_dump(struct unit_test_state *uts)
{
	/* this takes a long time to dump, with truetype enabled, so skip it */
	return -EAGAIN;

	ut_assertok(run_command("malloc dump", 0));
	ut_assert_nextlinen("Heap dump: ");
	ut_assert_nextline("%12s  %10s  %s", "Address", "Size", "Status");
	ut_assert_nextline("----------------------------------");
	ut_assert_nextline("%12lx  %10x  (chunk header)", mem_malloc_start, 0x10);
	ut_assert_skip_to_line("----------------------------------");
	ut_assert_nextlinen("Used: ");
	ut_assert_nextlinen("Free: ");
	ut_assert_console_end();

	return 0;
}
CMD_TEST(cmd_test_malloc_dump, UTF_CONSOLE);

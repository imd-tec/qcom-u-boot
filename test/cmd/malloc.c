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

	ut_assertok(run_command("malloc info", 0));
	ut_assert_nextlinen("total bytes   = ");
	ut_assert_nextlinen("in use bytes  = ");
	ut_assert_console_end();

	return 0;
}
CMD_TEST(cmd_test_malloc_info, UTF_CONSOLE);

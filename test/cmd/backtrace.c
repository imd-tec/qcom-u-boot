// SPDX-License-Identifier: GPL-2.0+
/*
 * Test for backtrace command
 *
 * Copyright 2025 Canonical Ltd
 * Written by Simon Glass <simon.glass@canonical.com>
 */

#include <dm.h>
#include <dm/test.h>
#include <test/test.h>
#include <test/ut.h>

/* Test 'backtrace' command */
static int cmd_test_backtrace(struct unit_test_state *uts)
{
	ut_assertok(run_command("backtrace", 0));

	ut_assert_nextlinen("backtrace:");
	ut_assert_nextlinen("  backtrace_show() at lib/backtrace.c:");
	ut_assert_nextlinen("  do_backtrace() at cmd/backtrace.c:");
	ut_assert_nextlinen("  cmd_process() at common/command.c:");
	ut_assert_skip_to_linen("  cmd_test_backtrace() at test/cmd/backtrace.c:");

	return 0;
}
DM_TEST(cmd_test_backtrace, UTF_SCAN_FDT);

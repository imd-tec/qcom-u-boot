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
	/* for now, just run the command */
	ut_assertok(run_command("backtrace", 0));

	return 0;
}
DM_TEST(cmd_test_backtrace, UTF_SCAN_FDT);

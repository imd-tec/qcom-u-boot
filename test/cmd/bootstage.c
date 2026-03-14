// SPDX-License-Identifier: GPL-2.0+
/*
 * Tests for bootstage command
 *
 * Copyright 2025 Canonical Ltd
 */

#include <bootstage.h>
#include <test/cmd.h>
#include <test/ut.h>

static int cmd_bootstage_report(struct unit_test_state *uts)
{
	uint count;

	/* Get the current record count */
	count = bootstage_get_rec_count();
	ut_assert(count > 0);

	/* Test the bootstage report command runs successfully */
	ut_assertok(run_command("bootstage report", 0));

	/* Verify the report contains expected headers and stages */
	ut_assert_nextline("Timer summary in microseconds (%u records):",
			   count);
	ut_assert_nextline("       Mark    Elapsed  Stage");
	ut_assert_nextline("          0          0  reset");
	ut_assert_skip_to_line("Accumulated time:");

	return 0;
}
CMD_TEST(cmd_bootstage_report, UTF_CONSOLE);

static int cmd_bootstage_save_restore(struct unit_test_state *uts)
{
	uint count;

	count = bootstage_get_rec_count();
	ut_assert(count > 0);

	/* Save the current count */
	ut_assertok(run_command("bootstage save", 0));
	ut_assert_console_end();

	/* Add a new record and check the count grows by one */
	bootstage_mark_name(BOOTSTAGE_ID_USER + 60, "test_save_restore");
	ut_asserteq(count + 1, bootstage_get_rec_count());

	/* Restore should bring the count back */
	ut_assertok(run_command("bootstage restore", 0));
	ut_assert_console_end();
	ut_asserteq(count, bootstage_get_rec_count());

	return 0;
}
CMD_TEST(cmd_bootstage_save_restore, UTF_CONSOLE);

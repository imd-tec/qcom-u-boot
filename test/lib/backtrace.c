// SPDX-License-Identifier: GPL-2.0+
/*
 * Test for backtrace functions
 *
 * Copyright 2025 Canonical Ltd
 * Written by Simon Glass <simon.glass@canonical.com>
 */

#include <backtrace.h>
#include <string.h>
#include <test/lib.h>
#include <test/test.h>
#include <test/ut.h>

/* Test backtrace_init() and backtrace_get_syms() */
static int lib_test_backtrace(struct unit_test_state *uts)
{
	char buf[BACKTRACE_BUFSZ];
	struct backtrace_ctx ctx;
	bool found_self = false;
	bool found_ut_run_list = false;
	uint i;

	ut_assert(backtrace_init(&ctx, 0) > 2);
	ut_assertok(backtrace_get_syms(&ctx, buf, sizeof(buf)));

	/*
	 * Check for known functions in the call stack. With libbacktrace
	 * we can find static functions too, so check for this test function.
	 */
	for (i = 0; i < ctx.count; i++) {
		if (ctx.syms[i]) {
			if (strstr(ctx.syms[i], "lib_test_backtrace"))
				found_self = true;
			if (strstr(ctx.syms[i], "ut_run_list"))
				found_ut_run_list = true;
		}
	}

	ut_assert(found_self);
	ut_assert(found_ut_run_list);

	backtrace_uninit(&ctx);

	return 0;
}
LIB_TEST(lib_test_backtrace, 0);

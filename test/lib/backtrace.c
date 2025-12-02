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
	static struct backtrace_ctx ctx;
	bool found_self = false;
	bool found_ut_run_list = false;
	uint i;

	ut_assert(backtrace_init(&ctx, 0) > 2);
	ut_assertok(backtrace_get_syms(&ctx, NULL, 0));

	/*
	 * Check for known functions in the call stack. With libbacktrace
	 * we can find static functions too, so check for this test function.
	 */
	for (i = 0; i < ctx.count; i++) {
		const struct backtrace_frame *frame = &ctx.frame[i];

		if (frame->sym) {
			if (strstr(frame->sym, "lib_test_backtrace"))
				found_self = true;
			if (strstr(frame->sym, "ut_run_list"))
				found_ut_run_list = true;
		}
	}

	ut_assert(found_self);
	ut_assert(found_ut_run_list);

	backtrace_uninit(&ctx);

	return 0;
}
LIB_TEST(lib_test_backtrace, 0);

/* Test backtrace_strf() and backtrace_str() */
static int lib_test_backtrace_str(struct unit_test_state *uts)
{
	char pattern[128];
	char buf[256];
	const char *cstr;
	char *str;
	int line;

	/* Test backtrace_strf() with skip=1 to skip backtrace_strf() itself */
	line = __LINE__ + 1;
	str = backtrace_strf(1, buf, sizeof(buf));
	ut_assertnonnull(str);
	ut_asserteq_ptr(buf, str);

	printf("backtrace_strf: %s\n", str);
	snprintf(pattern, sizeof(pattern),
		 "lib_test_backtrace_str:%d <-ut_run_test:\\d+ <-ut_run_test_live_flat:\\d+",
		 line);
	ut_asserteq_regex(pattern, str);

	/* Test backtrace_str() */
	line = __LINE__ + 1;
	cstr = backtrace_str(0);
	ut_assertnonnull(cstr);

	printf("backtrace_str: %s\n", cstr);
	snprintf(pattern, sizeof(pattern),
		 "lib_test_backtrace_str:%d <-ut_run_test:\\d+ <-ut_run_test_live_flat:\\d+",
		 line);
	ut_asserteq_regex(pattern, cstr);

	return 0;
}
LIB_TEST(lib_test_backtrace_str, 0);

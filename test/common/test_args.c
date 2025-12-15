// SPDX-License-Identifier: GPL-2.0+
/*
 * Test for unit test arguments
 *
 * Copyright 2025 Canonical Ltd
 * Written by Simon Glass <simon.glass@canonical.com>
 */

#include <string.h>
#include <test/common.h>
#include <test/test.h>
#include <test/ut.h>

/* Test that string arguments work correctly */
static int test_args_str_norun(struct unit_test_state *uts)
{
	ut_asserteq_str("hello", ut_str(0));

	return 0;
}
UNIT_TEST_ARGS(test_args_str_norun, UTF_CONSOLE | UTF_MANUAL, common,
	       { "strval", UT_ARG_STR });

/* Test that integer arguments work correctly */
static int test_args_int_norun(struct unit_test_state *uts)
{
	ut_asserteq(1234, ut_int(0));

	return 0;
}
UNIT_TEST_ARGS(test_args_int_norun, UTF_CONSOLE | UTF_MANUAL, common,
	       { "intval", UT_ARG_INT });

/* Test that boolean arguments work correctly */
static int test_args_bool_norun(struct unit_test_state *uts)
{
	ut_asserteq(true, ut_bool(0));

	return 0;
}
UNIT_TEST_ARGS(test_args_bool_norun, UTF_CONSOLE | UTF_MANUAL, common,
	       { "boolval", UT_ARG_BOOL });

/* Test multiple arguments of different types */
static int test_args_multi_norun(struct unit_test_state *uts)
{
	ut_asserteq_str("test", ut_str(0));
	ut_asserteq(42, ut_int(1));
	ut_asserteq(true, ut_bool(2));

	return 0;
}
UNIT_TEST_ARGS(test_args_multi_norun, UTF_CONSOLE | UTF_MANUAL, common,
	       { "str", UT_ARG_STR },
	       { "num", UT_ARG_INT },
	       { "flag", UT_ARG_BOOL });

/* Test optional arguments with defaults */
static int test_args_optional_norun(struct unit_test_state *uts)
{
	/* Required arg should match what was passed */
	ut_asserteq_str("required", ut_str(0));

	/* Optional args should have default values if not provided */
	ut_asserteq(99, ut_int(1));
	ut_asserteq(false, ut_bool(2));

	return 0;
}
UNIT_TEST_ARGS(test_args_optional_norun, UTF_CONSOLE | UTF_MANUAL, common,
	       { "req", UT_ARG_STR },
	       { "opt_int", UT_ARG_INT, UT_ARGF_OPTIONAL, { .vint = 99 } },
	       { "opt_bool", UT_ARG_BOOL, UT_ARGF_OPTIONAL, { .vbool = false } });

/*
 * Test requesting wrong type - ut_int() on a string arg should fail
 * This test deliberately causes a type mismatch to verify error handling
 */
static int test_args_wrongtype_norun(struct unit_test_state *uts)
{
	/* This should fail - asking for int but arg is string */
	ut_asserteq(0, ut_int(0));
	ut_asserteq(true, uts->arg_error);

	return 0;
}
UNIT_TEST_ARGS(test_args_wrongtype_norun, UTF_MANUAL, common,
	       { "strval", UT_ARG_STR });

/*
 * Test requesting invalid arg number - ut_str(1) when only arg 0 exists
 * This test deliberately causes an out-of-bounds access to verify error handling
 */
static int test_args_badnum_norun(struct unit_test_state *uts)
{
	/* This should fail - asking for arg 1 but only arg 0 exists */
	ut_asserteq_ptr(NULL, ut_str(1));
	ut_asserteq(true, uts->arg_error);

	return 0;
}
UNIT_TEST_ARGS(test_args_badnum_norun, UTF_MANUAL, common,
	       { "strval", UT_ARG_STR });

/* Wrapper test that runs the manual tests with proper arguments */
static int test_args(struct unit_test_state *uts)
{
	ut_assertok(run_command("ut -f common test_args_str_norun strval=hello",
				0));
	ut_assertok(run_command("ut -f common test_args_int_norun intval=1234",
				0));
	ut_assertok(run_command("ut -f common test_args_bool_norun boolval=1",
				0));
	ut_assertok(run_command("ut -f common test_args_multi_norun str=test num=42 flag=1",
				0));
	ut_assertok(run_command("ut -f common test_args_optional_norun req=required",
				0));

	return 0;
}
COMMON_TEST(test_args, UTF_CONSOLE);

/*
 * Test argument-parsing failure cases - these should all fail
 *
 * Note: Running 'ut' within a test is not normal practice since do_ut()
 * creates a new test state. But it works here for testing the argument
 * parsing itself.
 */
static int test_args_fail(struct unit_test_state *uts)
{
	/* Missing required argument - should fail */
	ut_asserteq(1, run_command("ut -f common test_args_str_norun", 0));
	ut_assert_nextline("Missing required argument 'strval' for test 'test_args_str_norun'");
	ut_assert_nextline_regex("Tests run: 1,.*failures: 1");
	ut_assert_console_end();

	/* Unknown argument name - should fail */
	ut_asserteq(1, run_command("ut -f common test_args_str_norun badarg=x",
				   0));
	ut_assert_nextline("Unknown argument 'badarg' for test 'test_args_str_norun'");
	ut_assert_nextline_regex("Tests run: 1,.*failures: 1");
	ut_assert_console_end();

	/* Invalid format (no = sign) - should fail */
	ut_asserteq(1, run_command("ut -f common test_args_str_norun strval",
				   0));
	ut_assert_nextline("Invalid argument 'strval' (expected key=value)");
	ut_assert_nextline_regex("Tests run: 1,.*failures: 1");
	ut_assert_console_end();

	return 0;
}
COMMON_TEST(test_args_fail, UTF_CONSOLE);

/* Test that requesting wrong type fails - ut_int() on string arg */
static int test_args_wrongtype(struct unit_test_state *uts)
{
	ut_asserteq(1,
		    run_command("ut -R -f common test_args_wrongtype_norun strval=hello",
				0));
	ut_assert_nextline("Test: test_args_wrongtype_norun: test_args.c");
	ut_assert_nextline_regex("test/common/test_args.c:.*, test_args_wrongtype_norun\\(\\): ut_int\\(\\) type check: arg 0 is not an int");
	ut_assert_nextline("Test 'test_args_wrongtype_norun' failed 1 times");
	ut_assert_nextline_regex("Tests run: 1,.*failures: 1");
	ut_assert_console_end();

	return 0;
}
COMMON_TEST(test_args_wrongtype, UTF_CONSOLE);

/* Test that requesting invalid arg number fails */
static int test_args_badnum(struct unit_test_state *uts)
{
	ut_asserteq(1,
		    run_command("ut -R -f common test_args_badnum_norun strval=hello",
				0));
	ut_assert_nextline("Test: test_args_badnum_norun: test_args.c");
	ut_assert_nextline_regex("test/common/test_args.c:.*, test_args_badnum_norun\\(\\): ut_str\\(\\) arg check: arg 1 is invalid \\(arg_count=1\\)");
	ut_assert_nextline("Test 'test_args_badnum_norun' failed 1 times");
	ut_assert_nextline_regex("Tests run: 1,.*failures: 1");
	ut_assert_console_end();

	return 0;
}
COMMON_TEST(test_args_badnum, UTF_CONSOLE);

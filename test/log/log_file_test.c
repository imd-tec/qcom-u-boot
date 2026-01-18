// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2026 Canonical Ltd
 * Written by Simon Glass <simon.glass@canonical.com>
 *
 * Test for log file driver
 */

#include <command.h>
#include <log.h>
#include <os.h>
#include <test/log.h>
#include <test/ut.h>

/* Test that the log_file driver can write to a file */
static int log_test_file_driver(struct unit_test_state *uts)
{
	const char *fname = "/tmp/log_test.log";
	void *buf;
	int size;

	ut_assertok(log_file_set_fname(fname));

	/* Generate some log messages using log rec command */
	run_command("log format Lfm", 0);
	run_command("log rec none warning test.c 123 my_func 'Test message'", 0);
	run_command("log rec none err error.c 456 err_func 'Error occurred'", 0);

	/* Close the file so we can read it */
	ut_assertok(log_file_set_fname(NULL));

	/* Read the file contents */
	ut_assertok(os_read_file(fname, &buf, &size));

	/* Check the contents */
	ut_asserteq_strn("123-my_func() Test message\n", buf);
	ut_assertnonnull(strstr(buf, "456-err_func() Error occurred\n"));

	os_free(buf);

	return 0;
}
LOG_TEST(log_test_file_driver);

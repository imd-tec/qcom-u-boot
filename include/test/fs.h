/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2025 Google LLC
 */

#ifndef __TEST_FS_H
#define __TEST_FS_H

#include <test/test.h>
#include <test/ut.h>

/**
 * FS_TEST() - Define a new filesystem test
 *
 * @name:	Name of test function
 * @flags:	Flags for the test (see enum ut_flags)
 */
#define FS_TEST(_name, _flags)	UNIT_TEST(_name, UTF_DM | (_flags), fs)

/**
 * FS_TEST_ARGS() - Define a filesystem test with inline arguments
 *
 * Like FS_TEST() but for tests that take arguments.
 * The test can access arguments via uts->args[].
 * The NULL terminator is added automatically.
 *
 * Example:
 *   FS_TEST_ARGS(my_test, UTF_MANUAL,
 *       { "fs_type", UT_ARG_STR },
 *       { "fs_image", UT_ARG_STR });
 *
 * @name:	Name of test function
 * @flags:	Flags for the test (see enum ut_flags)
 * @...:	Argument definitions (struct ut_arg_def initializers)
 */
#define FS_TEST_ARGS(_name, _flags, ...) \
	UNIT_TEST_ARGS(_name, UTF_DM | (_flags), fs, __VA_ARGS__)

#endif /* __TEST_FS_H */

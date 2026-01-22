// SPDX-License-Identifier: GPL-2.0+
/*
 * Test for expo environment editor
 *
 * Copyright 2025 Google LLC
 * Written by Simon Glass <sjg@chromium.org>
 */

#include <dm.h>
#include <env.h>
#include <expo.h>
#include <video.h>
#include <test/ut.h>
#include <test/video.h>
#include "bootstd_common.h"

/* Check expo_editenv() basic functionality */
static int editenv_test_base(struct unit_test_state *uts)
{
	char buf[256];
	int ret;

	/*
	 * Type "test" then press Enter to accept
	 * \x0d is Ctrl-M (Enter/carriage return)
	 */
	console_in_puts("test\x0d");
	ret = expo_editenv("myvar", NULL, buf, sizeof(buf));
	ut_assertok(ret);
	ut_asserteq_str("test", buf);

	return 0;
}
BOOTSTD_TEST(editenv_test_base, UTF_DM | UTF_SCAN_FDT | UTF_CONSOLE);

/* Check expo_editenv() with initial value - prepend text */
static int editenv_test_initial(struct unit_test_state *uts)
{
	char buf[256];
	int ret;

	/*
	 * Start with "world", go to start with Ctrl-A, type "hello ", then
	 * press Enter
	 */
	console_in_puts("\x01hello \x0d");
	ret = expo_editenv("myvar", "world", buf, sizeof(buf));
	ut_assertok(ret);
	ut_asserteq_str("hello world", buf);

	return 0;
}
BOOTSTD_TEST(editenv_test_initial, UTF_DM | UTF_SCAN_FDT | UTF_CONSOLE);

/* Check expo_editenv() escape closes editor (accepts current value) */
static int editenv_test_escape(struct unit_test_state *uts)
{
	char buf[256];
	int ret;

	/*
	 * Press Escape immediately - this closes the editor and accepts
	 * the current (initial) value
	 */
	console_in_puts("\x1b");
	ret = expo_editenv("myvar", "unchanged", buf, sizeof(buf));
	ut_assertok(ret);
	ut_asserteq_str("unchanged", buf);

	return 0;
}
BOOTSTD_TEST(editenv_test_escape, UTF_DM | UTF_SCAN_FDT | UTF_CONSOLE);

/* Check expo_editenv() renders correctly */
static int editenv_test_video(struct unit_test_state *uts)
{
	struct udevice *dev;
	char buf[256];
	int ret;

	ut_assertok(uclass_first_device_err(UCLASS_VIDEO, &dev));

	/* Type "abc" then press Enter */
	console_in_puts("abc\x0d");
	ret = expo_editenv("testvar", "initial", buf, sizeof(buf));
	ut_assertok(ret);
	ut_asserteq_str("initialabc", buf);

	/* Check the framebuffer has expected content */
	ut_asserteq(1029, video_compress_fb(uts, dev, false));

	return 0;
}
BOOTSTD_TEST(editenv_test_video, UTF_DM | UTF_SCAN_FDT | UTF_CONSOLE);

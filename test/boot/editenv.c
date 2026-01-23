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
#include <menu.h>
#include <video.h>
#include <video_console.h>
#include <test/ut.h>
#include <test/video.h>
#include "bootstd_common.h"

static const char initial[] =
	"This is a long string that will wrap to multiple lines "
	"when displayed in the textedit widget. It needs to be "
	"long enough to span several lines so that the up and down "
	"arrow keys can be tested properly.\n"
	"The arrow keys should "
	"move the cursor between lines in the multiline editor.";

/**
 * editenv_send() - Send a key to the editenv expo
 *
 * Arranges and renders the scene, sends the key, then checks for any
 * resulting action.
 *
 * @info: Editenv info
 * @key: Key to send (ASCII or BKEY_...)
 * Return: 0 if OK, 1 if editing is complete, -ECANCELED if user quit,
 *	other -ve on error
 */
static int editenv_send(struct editenv_info *info, int key)
{
	struct expo_action act;
	int ret;

	ret = expo_send_key(info->exp, key);
	if (ret)
		return ret;

	ret = scene_arrange(info->scn);
	if (ret)
		return ret;

	ret = expo_render(info->exp);
	if (ret)
		return ret;

	ret = expo_action_get(info->exp, &act);
	if (ret == -EAGAIN)
		return 0;
	if (ret)
		return ret;

	if (act.type == EXPOACT_QUIT)
		return -ECANCELED;

	if (act.type == EXPOACT_CLOSE)
		return 1;

	return 0;
}

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

/* Check expo_editenv() renders correctly with multiline text and navigation */
static int editenv_test_video(struct unit_test_state *uts)
{
	struct udevice *dev, *con;
	char buf[512];
	int ret;

	ut_assertok(uclass_first_device_err(UCLASS_VIDEO, &dev));
	ut_assertok(uclass_first_device_err(UCLASS_VIDEO_CONSOLE, &con));

	/* Set font size to 30 */
	ut_assertok(vidconsole_select_font(con, NULL, NULL, 30));

	/*
	 * Navigate with up arrow, insert text, then press Enter. The up arrow
	 * should be converted to Ctrl-P by scene_txtin_send_key().
	 * \x1b[A is the escape sequence for up arrow
	 */
	console_in_puts("\x1b[A!\x0d");
	ret = expo_editenv("testvar", initial, buf, sizeof(buf));
	ut_assertok(ret);

	/* The '!' should be inserted one visual line up from the end */
	ut_assert(strstr(buf, "tes!ted"));

	/* Check the framebuffer has expected content */
	ut_asserteq(16829, video_compress_fb(uts, dev, false));

	return 0;
}
BOOTSTD_TEST(editenv_test_video, UTF_DM | UTF_SCAN_FDT | UTF_CONSOLE);

/* Check the init/poll/uninit functions work correctly */
static int editenv_test_funcs(struct unit_test_state *uts)
{
	struct editenv_info info;
	struct udevice *dev, *con;

	ut_assertok(uclass_first_device_err(UCLASS_VIDEO, &dev));
	ut_assertok(uclass_first_device_err(UCLASS_VIDEO_CONSOLE, &con));

	/* Set font size to 30 */
	ut_assertok(vidconsole_select_font(con, NULL, NULL, 30));

	ut_assertok(expo_editenv_init("testvar", initial, &info));
	ut_asserteq(16611, ut_check_video(uts, "init"));

	/* Navigate up to previous line */
	ut_assertok(editenv_send(&info, BKEY_UP));
	ut_asserteq(16684, ut_check_video(uts, "up"));

	/* Navigate back down */
	ut_assertok(editenv_send(&info, BKEY_DOWN));
	ut_asserteq(16611, ut_check_video(uts, "down"));

	/* Type a character and press Enter to accept */
	ut_assertok(editenv_send(&info, '*'));
	ut_asserteq(16689, ut_check_video(uts, "insert"));

	ut_asserteq(1, editenv_send(&info, BKEY_SELECT));

	/* The '*' should be appended to the initial text */
	ut_assert(strstr(expo_editenv_result(&info), "editor.*"));
	ut_asserteq(16689, ut_check_video(uts, "save"));

	expo_editenv_uninit(&info);

	return 0;
}
BOOTSTD_TEST(editenv_test_funcs, UTF_DM | UTF_SCAN_FDT | UTF_CONSOLE);

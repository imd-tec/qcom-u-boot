// SPDX-License-Identifier: GPL-2.0+
/*
 * Tests for ext4l filesystem (Linux ext4 port)
 *
 * Copyright 2025 Canonical Ltd
 * Written by Simon Glass <simon.glass@canonical.com>
 */

#include <command.h>
#include <env.h>
#include <ext4l.h>
#include <fs.h>
#include <fs_legacy.h>
#include <u-boot/uuid.h>
#include <test/test.h>
#include <test/ut.h>
#include <test/fs.h>

#define EXT4L_ARG_IMAGE		0	/* fs_image: path to filesystem image */

/**
 * fs_test_ext4l_probe_norun() - Test probing an ext4l filesystem
 *
 * This test verifies that the ext4l driver can successfully probe and
 * mount an ext4 filesystem image.
 *
 * Arguments:
 *   fs_image: Path to the ext4 filesystem image
 */
static int fs_test_ext4l_probe_norun(struct unit_test_state *uts)
{
	const char *fs_image = ut_str(EXT4L_ARG_IMAGE);

	ut_assertnonnull(fs_image);
	ut_assertok(run_commandf("host bind 0 %s", fs_image));
	ut_assertok(fs_set_blk_dev("host", "0", FS_TYPE_ANY));

	return 0;
}
FS_TEST_ARGS(fs_test_ext4l_probe_norun, UTF_SCAN_FDT | UTF_CONSOLE | UTF_MANUAL,
	     { "fs_image", UT_ARG_STR });

/**
 * fs_test_ext4l_msgs_norun() - Test ext4l_msgs env var output
 *
 * This test verifies that setting ext4l_msgs=y causes mount messages
 * to be printed when probing an ext4 filesystem.
 *
 * Arguments:
 *   fs_image: Path to the ext4 filesystem image
 */
static int fs_test_ext4l_msgs_norun(struct unit_test_state *uts)
{
	const char *fs_image = ut_str(EXT4L_ARG_IMAGE);
	char uuid_str[UUID_STR_LEN + 1];
	u8 uuid[16];

	ut_assertnonnull(fs_image);
	ut_assertok(env_set("ext4l_msgs", "y"));
	console_record_reset_enable();
	ut_assertok(run_commandf("host bind 0 %s", fs_image));
	ut_assertok(fs_set_blk_dev("host", "0", FS_TYPE_ANY));

	/* Get the UUID and clear the env var now we have the output */
	ut_assertok(ext4l_get_uuid(uuid));
	uuid_bin_to_str(uuid, uuid_str, UUID_STR_FORMAT_STD);
	ut_assertok(env_set("ext4l_msgs", NULL));

	/*
	 * Check messages. The probe test runs first and doesn't unmount,
	 * so the journal needs recovery. Verify both messages.
	 */
	ut_assert_nextline("EXT4-fs (ext4l_mmc0): recovery complete");
	ut_assert_nextline("EXT4-fs (ext4l_mmc0): mounted filesystem %s r/w with ordered data mode. Quota mode: disabled.",
			   uuid_str);
	ut_assert_console_end();

	return 0;
}
FS_TEST_ARGS(fs_test_ext4l_msgs_norun, UTF_SCAN_FDT | UTF_CONSOLE | UTF_MANUAL,
	     { "fs_image", UT_ARG_STR });

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
#include <linux/sizes.h>
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
	 * so the journal needs recovery. The filesystem may be mounted
	 * multiple times during probe operations. Just verify we see the
	 * expected mount message at least once.
	 */
	ut_assert_skip_to_line("EXT4-fs (ext4l_mmc0): mounted filesystem %s r/w"
			       " with ordered data mode. Quota mode: disabled.",
			       uuid_str);
	/* Skip any remaining messages */

	return 0;
}
FS_TEST_ARGS(fs_test_ext4l_msgs_norun, UTF_SCAN_FDT | UTF_CONSOLE | UTF_MANUAL,
	     { "fs_image", UT_ARG_STR });

/**
 * fs_test_ext4l_ls_norun() - Test ext4l ls command
 *
 * This test verifies that the ext4l driver can list directory contents.
 *
 * Arguments:
 *   fs_image: Path to the ext4 filesystem image
 */
static int fs_test_ext4l_ls_norun(struct unit_test_state *uts)
{
	const char *fs_image = ut_str(EXT4L_ARG_IMAGE);

	ut_assertnonnull(fs_image);
	ut_assertok(run_commandf("host bind 0 %s", fs_image));
	console_record_reset_enable();
	ut_assertok(run_commandf("ls host 0"));
	/*
	 * The Python test adds testfile.txt (12 bytes) to the image.
	 * Directory entries appear in hash order which varies between runs.
	 * Verify the file entry appears with correct size (12 bytes).
	 * Other entries like ., .., subdir, lost+found may also appear.
	 */
	ut_assert_skip_to_line("       12   testfile.txt");
	/* Skip any remaining entries */

	return 0;
}
FS_TEST_ARGS(fs_test_ext4l_ls_norun, UTF_SCAN_FDT | UTF_CONSOLE | UTF_MANUAL,
	     { "fs_image", UT_ARG_STR });

/**
 * fs_test_ext4l_opendir_norun() - Test ext4l opendir/readdir/closedir
 *
 * Verifies that the ext4l driver can iterate through directory entries using
 * the opendir/readdir/closedir interface. It checks:
 * - Regular files (testfile.txt)
 * - Subdirectories (subdir)
 * - Symlinks (link.txt)
 * - Files in subdirectories (subdir/nested.txt)
 *
 * Arguments:
 *   fs_image: Path to the ext4 filesystem image
 */
static int fs_test_ext4l_opendir_norun(struct unit_test_state *uts)
{
	const char *fs_image = ut_str(EXT4L_ARG_IMAGE);
	struct fs_dir_stream *dirs;
	struct fs_dirent *dent;
	bool found_testfile = false;
	bool found_subdir = false;
	bool found_symlink = false;
	bool found_nested = false;
	int count = 0;

	ut_assertnonnull(fs_image);
	ut_assertok(run_commandf("host bind 0 %s", fs_image));
	ut_assertok(fs_set_blk_dev("host", "0", FS_TYPE_ANY));

	/* Open root directory */
	ut_assertok(ext4l_opendir("/", &dirs));
	ut_assertnonnull(dirs);

	/* Iterate through entries */
	while (!ext4l_readdir(dirs, &dent)) {
		ut_assertnonnull(dent);
		count++;
		if (!strcmp(dent->name, "testfile.txt")) {
			found_testfile = true;
			ut_asserteq(FS_DT_REG, dent->type);
			ut_asserteq(12, dent->size);
		} else if (!strcmp(dent->name, "subdir")) {
			found_subdir = true;
			ut_asserteq(FS_DT_DIR, dent->type);
		} else if (!strcmp(dent->name, "link.txt")) {
			found_symlink = true;
			ut_asserteq(FS_DT_LNK, dent->type);
		}
	}

	ext4l_closedir(dirs);

	/* Verify we found expected entries */
	ut_assert(found_testfile);
	ut_assert(found_subdir);
	ut_assert(found_symlink);
	/* At least ., .., testfile.txt, subdir, link.txt */
	ut_assert(count >= 5);

	/* Now test reading the subdirectory */
	ut_assertok(fs_set_blk_dev("host", "0", FS_TYPE_ANY));
	ut_assertok(ext4l_opendir("/subdir", &dirs));
	ut_assertnonnull(dirs);

	count = 0;
	while (!ext4l_readdir(dirs, &dent)) {
		ut_assertnonnull(dent);
		count++;
		if (!strcmp(dent->name, "nested.txt")) {
			found_nested = true;
			ut_asserteq(FS_DT_REG, dent->type);
			ut_asserteq(12, dent->size);
		}
	}

	ext4l_closedir(dirs);

	ut_assert(found_nested);
	/* At least ., .., nested.txt */
	ut_assert(count >= 3);

	return 0;
}
FS_TEST_ARGS(fs_test_ext4l_opendir_norun, UTF_SCAN_FDT | UTF_CONSOLE |
	     UTF_MANUAL, { "fs_image", UT_ARG_STR });

/**
 * fs_test_ext4l_exists_norun() - Test ext4l_exists function
 *
 * Verifies that ext4l_exists correctly reports file existence.
 *
 * Arguments:
 *   fs_image: Path to the ext4 filesystem image
 */
static int fs_test_ext4l_exists_norun(struct unit_test_state *uts)
{
	const char *fs_image = ut_str(EXT4L_ARG_IMAGE);

	ut_assertnonnull(fs_image);
	ut_assertok(run_commandf("host bind 0 %s", fs_image));
	ut_assertok(fs_set_blk_dev("host", "0", FS_TYPE_ANY));

	/* Test existing directory */
	ut_asserteq(1, ext4l_exists("/"));

	/* Test non-existent paths */
	ut_asserteq(0, ext4l_exists("/no/such/path"));

	return 0;
}
FS_TEST_ARGS(fs_test_ext4l_exists_norun, UTF_SCAN_FDT | UTF_CONSOLE |
	     UTF_MANUAL, { "fs_image", UT_ARG_STR });

/**
 * fs_test_ext4l_size_norun() - Test ext4l_size function
 *
 * Verifies that ext4l_size correctly reports file size.
 *
 * Arguments:
 *   fs_image: Path to the ext4 filesystem image
 */
static int fs_test_ext4l_size_norun(struct unit_test_state *uts)
{
	const char *fs_image = ut_str(EXT4L_ARG_IMAGE);
	loff_t size;

	ut_assertnonnull(fs_image);
	ut_assertok(run_commandf("host bind 0 %s", fs_image));
	ut_assertok(fs_set_blk_dev("host", "0", FS_TYPE_ANY));

	/* Test root directory size - one block on a 4K block filesystem */
	ut_assertok(ext4l_size("/", &size));
	ut_asserteq(SZ_4K, size);

	/* Test file size - testfile.txt contains "hello world\n" */
	ut_assertok(ext4l_size("/testfile.txt", &size));
	ut_asserteq(12, size);

	/* Test non-existent path returns -ENOENT */
	ut_asserteq(-ENOENT, ext4l_size("/no/such/path", &size));

	return 0;
}
FS_TEST_ARGS(fs_test_ext4l_size_norun, UTF_SCAN_FDT | UTF_CONSOLE | UTF_MANUAL,
	     { "fs_image", UT_ARG_STR });

/**
 * fs_test_ext4l_read_norun() - Test ext4l_read function
 *
 * Verifies that ext4l can read file contents.
 *
 * Arguments:
 *   fs_image: Path to the ext4 filesystem image
 */
static int fs_test_ext4l_read_norun(struct unit_test_state *uts)
{
	const char *fs_image = ut_str(EXT4L_ARG_IMAGE);
	loff_t actread;
	char buf[32];

	ut_assertnonnull(fs_image);
	ut_assertok(run_commandf("host bind 0 %s", fs_image));
	ut_assertok(fs_set_blk_dev("host", "0", FS_TYPE_ANY));

	/* Read the test file - contains "hello world\n" (12 bytes) */
	memset(buf, '\0', sizeof(buf));
	ut_assertok(ext4l_read("/testfile.txt", buf, 0, 0, &actread));
	ut_asserteq(12, actread);
	ut_asserteq_str("hello world\n", buf);

	/* Test partial read with offset */
	memset(buf, '\0', sizeof(buf));
	ut_assertok(ext4l_read("/testfile.txt", buf, 6, 5, &actread));
	ut_asserteq(5, actread);
	ut_asserteq_str("world", buf);

	/* Verify read returns error for non-existent path */
	ut_asserteq(-ENOENT, ext4l_read("/no/such/file", buf, 0, 10, &actread));

	return 0;
}
FS_TEST_ARGS(fs_test_ext4l_read_norun, UTF_SCAN_FDT | UTF_CONSOLE | UTF_MANUAL,
	     { "fs_image", UT_ARG_STR });

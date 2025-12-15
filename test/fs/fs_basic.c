// SPDX-License-Identifier: GPL-2.0+
/*
 * Basic filesystem tests - C implementation for Python wrapper
 *
 * These tests are marked UTF_MANUAL and are intended to be called from
 * test_basic.py which sets up the filesystem image and expected values.
 *
 * Copyright 2025 Google LLC
 */

#include <command.h>
#include <dm.h>
#include <env.h>
#include <fs.h>
#include <fs_legacy.h>
#include <hexdump.h>
#include <image.h>
#include <linux/sizes.h>
#include <mapmem.h>
#include <test/fs.h>
#include <test/test.h>
#include <test/ut.h>
#include <u-boot/md5.h>

/* Test constants matching fstest_defs.py */
#define ADDR	0x01000008

/*
 * Common argument indices. Each test declares only the arguments it needs,
 * so indices 2+ vary per test - see comments in each test.
 */
#define FS_ARG_TYPE	0	/* fs_type: ext4, fat, exfat, fs_generic */
#define FS_ARG_IMAGE	1	/* fs_image: path to filesystem image */

/* Common arguments for all filesystem tests (indices 0 and 1) */
#define COMMON_ARGS \
	{ "fs_type", UT_ARG_STR }, \
	{ "fs_image", UT_ARG_STR }

/**
 * get_fs_type(uts) - Get filesystem type enum from test argument
 *
 * Reads the fs_type argument and returns the appropriate FS_TYPE_* enum value.
 *
 * Return: filesystem type enum
 */
static int get_fs_type(struct unit_test_state *uts)
{
	const char *fs_type = ut_str(FS_ARG_TYPE);

	if (!fs_type)
		return FS_TYPE_ANY;

	if (!strcmp(fs_type, "ext4"))
		return FS_TYPE_EXT;
	if (!strcmp(fs_type, "fat"))
		return FS_TYPE_FAT;
	if (!strcmp(fs_type, "exfat"))
		return FS_TYPE_EXFAT;

	/* fs_generic uses FS_TYPE_ANY */
	return FS_TYPE_ANY;
}

/* Set up the host filesystem block device */
static int set_fs(struct unit_test_state *uts)
{
	return fs_set_blk_dev("host", "0:0", get_fs_type(uts));
}

/* Build a path by prepending "/" to the leaf filename, with optional suffix */
static const char *getpath(struct unit_test_state *uts, const char *leaf,
			   const char *suffix)
{
	snprintf(uts->priv, sizeof(uts->priv), "/%s%s", leaf, suffix ?: "");

	return uts->priv;
}

/**
 * prep_fs() - Prepare filesystem for testing
 *
 * Binds the fs_image argument as host device 0, sets up the block device,
 * and optionally returns a zeroed buffer.
 *
 * @uts: Unit test state
 * @len: Length of buffer to allocate and zero, or 0 for none
 * @bufp: Returns pointer to zeroed buffer, or NULL if @len is 0
 * Return: 0 on success, negative on error
 */
static int prep_fs(struct unit_test_state *uts, uint len, void **bufp)
{
	const char *fs_image = ut_str(FS_ARG_IMAGE);

	ut_assertnonnull(fs_image);
	ut_assertok(run_commandf("host bind 0 %s", fs_image));
	ut_assertok(set_fs(uts));

	if (len) {
		*bufp = map_sysmem(ADDR, len);
		memset(*bufp, '\0', len);
	}

	return 0;
}

/**
 * fs_write_supported(uts) - Check if write is supported for current fs type
 *
 * Reads the fs_type argument and checks if write support is enabled
 * for that filesystem type.
 *
 * Return: true if write is supported, false otherwise
 */
static bool fs_write_supported(struct unit_test_state *uts)
{
	const char *fs_type = ut_str(FS_ARG_TYPE);

	if (!fs_type)
		return false;

	if (!strcmp(fs_type, "ext4"))
		return IS_ENABLED(CONFIG_EXT4_WRITE);
	if (!strcmp(fs_type, "fat"))
		return IS_ENABLED(CONFIG_CMD_FAT_WRITE);

	/* fs_generic and exfat use generic write which is always available */
	return true;
}

/**
 * verify_md5() - Calculate MD5 of buffer and verify against expected
 *
 * Uses arg 3 (md5val) as the expected MD5 hex string.
 *
 * @uts: Unit test state
 * @buf: Buffer to calculate MD5 of
 * @len: Length of buffer
 *
 * Return: 0 if MD5 matches, -EINVAL otherwise
 */
static int verify_md5(struct unit_test_state *uts, const void *buf, size_t len)
{
	u8 digest[MD5_SUM_LEN], expected[MD5_SUM_LEN];
	const char *expected_hex = ut_str(3);

	ut_assertok(hex2bin(expected, expected_hex, MD5_SUM_LEN));

	md5_wd(buf, len, digest, CHUNKSZ_MD5);
	ut_asserteq_mem(expected, digest, MD5_SUM_LEN);

	return 0;
}

/**
 * Test Case 1 - ls command, listing root directory and invalid directory
 */
static int fs_test_ls_norun(struct unit_test_state *uts)
{
	const char *small = ut_str(2);
	const char *big = ut_str(3);
	struct fs_dir_stream *dirs;
	struct fs_dirent *dent;
	int found_big = 0, found_small = 0, found_subdir = 0;

	ut_assertok(prep_fs(uts, 0, NULL));

	/* Test listing root directory */
	dirs = fs_opendir("/");
	ut_assertnonnull(dirs);

	while ((dent = fs_readdir(dirs))) {
		if (!strcmp(dent->name, big)) {
			found_big = 1;
			ut_asserteq(FS_DT_REG, dent->type);
		} else if (!strcmp(dent->name, small)) {
			found_small = 1;
			ut_asserteq(FS_DT_REG, dent->type);
		} else if (!strcmp(dent->name, "SUBDIR")) {
			found_subdir = 1;
			ut_asserteq(FS_DT_DIR, dent->type);
		}
	}
	fs_closedir(dirs);

	ut_asserteq(1, found_big);
	ut_asserteq(1, found_small);
	ut_asserteq(1, found_subdir);

	/* Test invalid directory returns error */
	ut_assertok(set_fs(uts));
	dirs = fs_opendir("/invalid_d");
	ut_assertnull(dirs);

	/* Test file exists */
	ut_assertok(set_fs(uts));
	ut_asserteq(1, fs_exists(small));

	/* Test non-existent file */
	ut_assertok(set_fs(uts));
	ut_asserteq(0, fs_exists("nonexistent.file"));

	return 0;
}
FS_TEST_ARGS(fs_test_ls_norun, UTF_SCAN_FDT | UTF_CONSOLE | UTF_MANUAL,
	     COMMON_ARGS, { "small", UT_ARG_STR }, { "big", UT_ARG_STR });

/**
 * Test Case 2 - size command for small file (1MB)
 */
static int fs_test_size_small_norun(struct unit_test_state *uts)
{
	const char *small = ut_str(2);
	loff_t size;

	ut_assertok(prep_fs(uts, 0, NULL));
	ut_assertok(fs_size(getpath(uts, small, NULL), &size));
	ut_asserteq(SZ_1M, size);

	/* Test size via path with '..' */
	ut_assertok(set_fs(uts));
	snprintf(uts->priv, sizeof(uts->priv), "/SUBDIR/../%s", small);
	ut_assertok(fs_size(uts->priv, &size));
	ut_asserteq(SZ_1M, size);

	return 0;
}
FS_TEST_ARGS(fs_test_size_small_norun, UTF_SCAN_FDT | UTF_MANUAL,
	     COMMON_ARGS, { "small", UT_ARG_STR });

/**
 * Test Case 3 - size command for large file (2500 MiB)
 */
static int fs_test_size_big_norun(struct unit_test_state *uts)
{
	const char *big = ut_str(2);
	loff_t size;

	ut_assertok(prep_fs(uts, 0, NULL));
	ut_assertok(fs_size(getpath(uts, big, NULL), &size));
	ut_asserteq_64((loff_t)SZ_1M * 2500, size);  /* 2500 MiB = 0x9c400000 */

	return 0;
}
FS_TEST_ARGS(fs_test_size_big_norun, UTF_SCAN_FDT | UTF_MANUAL,
	     COMMON_ARGS, { "big", UT_ARG_STR });

/**
 * Test Case 4 - load small file, verify MD5
 */
static int fs_test_load_small_norun(struct unit_test_state *uts)
{
	const char *small = ut_str(2);
	loff_t actread;
	void *buf;

	ut_assertok(prep_fs(uts, SZ_1M, &buf));
	ut_assertok(fs_legacy_read(getpath(uts, small, NULL), ADDR, 0, 0,
				   &actread));
	ut_asserteq(SZ_1M, actread);
	ut_assertok(verify_md5(uts, buf, SZ_1M));

	return 0;
}
FS_TEST_ARGS(fs_test_load_small_norun, UTF_SCAN_FDT | UTF_MANUAL,
	     COMMON_ARGS, { "small", UT_ARG_STR }, { "md5val", UT_ARG_STR });

/**
 * Test Case 5 - load first 1MB of big file
 */
static int fs_test_load_big_first_norun(struct unit_test_state *uts)
{
	const char *big = ut_str(2);
	loff_t actread;
	void *buf;

	ut_assertok(prep_fs(uts, SZ_1M, &buf));
	ut_assertok(fs_legacy_read(getpath(uts, big, NULL), ADDR, 0, SZ_1M,
				   &actread));
	ut_asserteq(SZ_1M, actread);
	ut_assertok(verify_md5(uts, buf, SZ_1M));

	return 0;
}
FS_TEST_ARGS(fs_test_load_big_first_norun, UTF_SCAN_FDT | UTF_MANUAL,
	     COMMON_ARGS, { "big", UT_ARG_STR }, { "md5val", UT_ARG_STR });

/**
 * Test Case 6 - load last 1MB of big file (offset 0x9c300000)
 */
static int fs_test_load_big_last_norun(struct unit_test_state *uts)
{
	const char *big = ut_str(2);
	loff_t actread;
	void *buf;

	ut_assertok(prep_fs(uts, SZ_1M, &buf));
	ut_assertok(fs_legacy_read(getpath(uts, big, NULL), ADDR, 0x9c300000ULL,
				   SZ_1M, &actread));
	ut_asserteq(SZ_1M, actread);
	ut_assertok(verify_md5(uts, buf, SZ_1M));

	return 0;
}
FS_TEST_ARGS(fs_test_load_big_last_norun, UTF_SCAN_FDT | UTF_MANUAL,
	     COMMON_ARGS, { "big", UT_ARG_STR }, { "md5val", UT_ARG_STR });

/**
 * Test Case 7 - load 1MB from last 1MB chunk of 2GB (offset 0x7ff00000)
 */
static int fs_test_load_big_2g_last_norun(struct unit_test_state *uts)
{
	const char *big = ut_str(2);
	loff_t actread;
	void *buf;

	ut_assertok(prep_fs(uts, SZ_1M, &buf));
	ut_assertok(fs_legacy_read(getpath(uts, big, NULL), ADDR, 0x7ff00000ULL,
				   SZ_1M, &actread));
	ut_asserteq(SZ_1M, actread);
	ut_assertok(verify_md5(uts, buf, SZ_1M));

	return 0;
}
FS_TEST_ARGS(fs_test_load_big_2g_last_norun, UTF_SCAN_FDT | UTF_MANUAL,
	     COMMON_ARGS, { "big", UT_ARG_STR }, { "md5val", UT_ARG_STR });

/**
 * Test Case 8 - load first 1MB in 2GB region (offset 0x80000000)
 */
static int fs_test_load_big_2g_first_norun(struct unit_test_state *uts)
{
	const char *big = ut_str(2);
	loff_t actread;
	void *buf;

	ut_assertok(prep_fs(uts, SZ_1M, &buf));
	ut_assertok(fs_legacy_read(getpath(uts, big, NULL), ADDR, 0x80000000ULL,
				   SZ_1M, &actread));
	ut_asserteq(SZ_1M, actread);
	ut_assertok(verify_md5(uts, buf, SZ_1M));

	return 0;
}
FS_TEST_ARGS(fs_test_load_big_2g_first_norun, UTF_SCAN_FDT | UTF_MANUAL,
	     COMMON_ARGS, { "big", UT_ARG_STR }, { "md5val", UT_ARG_STR });

/**
 * Test Case 9 - load 1MB crossing 2GB boundary (offset 0x7ff80000)
 */
static int fs_test_load_big_2g_cross_norun(struct unit_test_state *uts)
{
	const char *big = ut_str(2);
	loff_t actread;
	void *buf;

	ut_assertok(prep_fs(uts, SZ_1M, &buf));
	ut_assertok(fs_legacy_read(getpath(uts, big, NULL), ADDR, 0x7ff80000ULL,
				   SZ_1M, &actread));
	ut_asserteq(SZ_1M, actread);
	ut_assertok(verify_md5(uts, buf, SZ_1M));

	return 0;
}
FS_TEST_ARGS(fs_test_load_big_2g_cross_norun, UTF_SCAN_FDT | UTF_MANUAL,
	     COMMON_ARGS, { "big", UT_ARG_STR }, { "md5val", UT_ARG_STR });

/**
 * Test Case 10 - load beyond file end (2MB from offset where only 1MB remains)
 */
static int fs_test_load_beyond_norun(struct unit_test_state *uts)
{
	const char *big = ut_str(2);
	loff_t actread;
	void *buf;

	ut_assertok(prep_fs(uts, SZ_2M, &buf));  /* 2MB buffer */

	/* Request 2MB starting at 1MB before EOF - should get 1MB */
	ut_assertok(fs_legacy_read(getpath(uts, big, NULL), ADDR, 0x9c300000ULL,
				   SZ_2M, &actread));
	ut_asserteq(SZ_1M, actread);  /* Only 1MB available */

	return 0;
}
FS_TEST_ARGS(fs_test_load_beyond_norun, UTF_SCAN_FDT | UTF_MANUAL,
	     COMMON_ARGS, { "big", UT_ARG_STR });

/**
 * Test Case 11 - write file
 */
static int fs_test_write_norun(struct unit_test_state *uts)
{
	const char *small = ut_str(2);

	loff_t actread, actwrite;
	void *buf;

	if (!fs_write_supported(uts))
		return -EAGAIN;

	ut_assertok(prep_fs(uts, SZ_1M, &buf));

	/* Read small file */
	ut_assertok(fs_legacy_read(getpath(uts, small, NULL), ADDR, 0, 0,
				   &actread));
	ut_asserteq(SZ_1M, actread);

	/* Write it back with new name */
	ut_assertok(set_fs(uts));
	ut_assertok(fs_write(getpath(uts, small, ".w"), ADDR, 0, SZ_1M,
			     &actwrite));
	ut_asserteq(SZ_1M, actwrite);

	/* Read back and verify MD5 */
	ut_assertok(set_fs(uts));
	memset(buf, '\0', SZ_1M);
	ut_assertok(fs_legacy_read(getpath(uts, small, ".w"), ADDR, 0, 0,
				   &actread));
	ut_asserteq(SZ_1M, actread);

	ut_assertok(verify_md5(uts, buf, SZ_1M));

	return 0;
}
FS_TEST_ARGS(fs_test_write_norun, UTF_SCAN_FDT | UTF_MANUAL,
	     COMMON_ARGS, { "small", UT_ARG_STR }, { "md5val", UT_ARG_STR });

/**
 * Test Case 12 - write to "." directory (should fail)
 */
static int fs_test_write_dot_norun(struct unit_test_state *uts)
{
	loff_t actwrite;

	if (!fs_write_supported(uts))
		return -EAGAIN;

	ut_assertok(prep_fs(uts, 0, NULL));

	/* Writing to "." should fail */
	ut_assert(fs_write("/.", ADDR, 0, 0x10, &actwrite));

	return 0;
}
FS_TEST_ARGS(fs_test_write_dot_norun, UTF_SCAN_FDT | UTF_MANUAL,
	     COMMON_ARGS);

/**
 * Test Case 13 - write via "./" path
 */
static int fs_test_write_dotpath_norun(struct unit_test_state *uts)
{
	const char *small = ut_str(2);
	loff_t actread, actwrite;
	void *buf;

	if (!fs_write_supported(uts))
		return -EAGAIN;

	ut_assertok(prep_fs(uts, SZ_1M, &buf));

	/* Read small file */
	ut_assertok(fs_legacy_read(getpath(uts, small, NULL), ADDR, 0, 0,
				   &actread));
	ut_asserteq(SZ_1M, actread);

	/* Write via "./" path */
	ut_assertok(set_fs(uts));
	snprintf(uts->priv, sizeof(uts->priv), "/./%s2", small);
	ut_assertok(fs_write(uts->priv, ADDR, 0, SZ_1M, &actwrite));
	ut_asserteq(SZ_1M, actwrite);

	/* Read back via "./" path and verify */
	ut_assertok(set_fs(uts));
	memset(buf, '\0', SZ_1M);
	ut_assertok(fs_legacy_read(uts->priv, ADDR, 0, 0, &actread));
	ut_asserteq(SZ_1M, actread);
	ut_assertok(verify_md5(uts, buf, SZ_1M));

	/* Also verify via normal path */
	ut_assertok(set_fs(uts));
	memset(buf, '\0', SZ_1M);
	ut_assertok(fs_legacy_read(getpath(uts, small, "2"), ADDR, 0, 0,
				   &actread));
	ut_asserteq(SZ_1M, actread);
	ut_assertok(verify_md5(uts, buf, SZ_1M));

	return 0;
}
FS_TEST_ARGS(fs_test_write_dotpath_norun, UTF_SCAN_FDT | UTF_MANUAL,
	     COMMON_ARGS, { "small", UT_ARG_STR }, { "md5val", UT_ARG_STR });

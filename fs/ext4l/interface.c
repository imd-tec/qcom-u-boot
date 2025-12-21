// SPDX-License-Identifier: GPL-2.0+
/*
 * U-Boot interface for ext4l filesystem (Linux port)
 *
 * Copyright 2025 Canonical Ltd
 * Written by Simon Glass <simon.glass@canonical.com>
 *
 * This provides the minimal interface between U-Boot and the ext4l driver.
 */

#include <blk.h>
#include <part.h>
#include <malloc.h>
#include <asm/byteorder.h>
#include <linux/errno.h>
#include <linux/jbd2.h>
#include <linux/types.h>

#include "ext4_uboot.h"
#include "ext4.h"

/* Global state */
static struct blk_desc *ext4l_dev_desc;
static struct disk_partition ext4l_part;

int ext4l_probe(struct blk_desc *fs_dev_desc,
		struct disk_partition *fs_partition)
{
	struct ext4_fs_context *ctx;
	struct super_block *sb;
	struct fs_context *fc;
	loff_t part_offset;
	__le16 *magic;
	u8 *buf;
	int ret;

	if (!fs_dev_desc)
		return -EINVAL;

	/* Initialise journal subsystem if enabled */
	if (IS_ENABLED(CONFIG_EXT4_JOURNAL)) {
		ret = jbd2_journal_init_global();
		if (ret)
			return ret;
	}

	/* Initialise multi-block allocator for write support */
	if (IS_ENABLED(CONFIG_EXT4_WRITE)) {
		ret = ext4_init_mballoc();
		if (ret)
			return ret;
	}

	/* Initialise extent status cache */
	ret = ext4_init_es();
	if (ret)
		return ret;

	/* Allocate super_block */
	sb = kzalloc(sizeof(struct super_block), GFP_KERNEL);
	if (!sb) {
		ret = -ENOMEM;
		goto err_exit_es;
	}

	/* Allocate block_device */
	sb->s_bdev = kzalloc(sizeof(struct block_device), GFP_KERNEL);
	if (!sb->s_bdev) {
		ret = -ENOMEM;
		goto err_free_sb;
	}

	sb->s_bdev->bd_mapping = kzalloc(sizeof(struct address_space), GFP_KERNEL);
	if (!sb->s_bdev->bd_mapping) {
		ret = -ENOMEM;
		goto err_free_bdev;
	}

	/* Allocate fs_context */
	fc = kzalloc(sizeof(struct fs_context), GFP_KERNEL);
	if (!fc) {
		ret = -ENOMEM;
		goto err_free_mapping;
	}

	/* Allocate ext4_fs_context */
	ctx = kzalloc(sizeof(struct ext4_fs_context), GFP_KERNEL);
	if (!ctx) {
		ret = -ENOMEM;
		goto err_free_fc;
	}

	/* Initialise fs_context fields */
	fc->fs_private = ctx;
	fc->sb_flags |= SB_I_VERSION;

	buf = malloc(BLOCK_SIZE + 512);
	if (!buf) {
		ret = -ENOMEM;
		goto err_free_ctx;
	}

	/* Calculate partition offset in bytes */
	part_offset = fs_partition ? (loff_t)fs_partition->start * fs_dev_desc->blksz : 0;

	/* Read sectors containing the superblock */
	if (blk_dread(fs_dev_desc,
		      (part_offset + BLOCK_SIZE) / fs_dev_desc->blksz,
		      2, buf) != 2) {
		ret = -EIO;
		goto err_free_buf;
	}

	/* Check magic number within superblock */
	magic = (__le16 *)(buf + (BLOCK_SIZE % fs_dev_desc->blksz) +
			   offsetof(struct ext4_super_block, s_magic));
	if (le16_to_cpu(*magic) != EXT4_SUPER_MAGIC) {
		ret = -EINVAL;
		goto err_free_buf;
	}

	/* Save device info for later operations */
	ext4l_dev_desc = fs_dev_desc;
	if (fs_partition)
		memcpy(&ext4l_part, fs_partition, sizeof(ext4l_part));

	free(buf);
	kfree(ctx);
	kfree(fc);
	kfree(sb->s_bdev->bd_mapping);
	kfree(sb->s_bdev);
	kfree(sb);

	return 0;

err_free_buf:
	free(buf);
err_free_ctx:
	kfree(ctx);
err_free_fc:
	kfree(fc);
err_free_mapping:
	kfree(sb->s_bdev->bd_mapping);
err_free_bdev:
	kfree(sb->s_bdev);
err_free_sb:
	kfree(sb);
err_exit_es:
	ext4_exit_es();
	return ret;
}

void ext4l_close(void)
{
	ext4l_dev_desc = NULL;
}

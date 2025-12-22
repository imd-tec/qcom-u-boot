// SPDX-License-Identifier: GPL-2.0+
/*
 * U-Boot interface for ext4l filesystem (Linux port)
 *
 * Copyright 2025 Canonical Ltd
 * Written by Simon Glass <simon.glass@canonical.com>
 *
 * This provides the interface between U-Boot's filesystem layer and
 * the ext4l driver.
 */

#include <blk.h>
#include <env.h>
#include <membuf.h>
#include <part.h>
#include <malloc.h>
#include <linux/errno.h>
#include <linux/jbd2.h>
#include <linux/types.h>

#include "ext4_uboot.h"
#include "ext4.h"

/* Message buffer size */
#define EXT4L_MSG_BUF_SIZE	4096

/* Global state */
static struct blk_desc *ext4l_dev_desc;
static struct disk_partition ext4l_part;

/* Global block device tracking for buffer I/O */
static struct blk_desc *ext4l_blk_dev;
static struct disk_partition ext4l_partition;
static int ext4l_mounted;

/* Global super_block pointer for filesystem operations */
static struct super_block *ext4l_sb;

/* Message recording buffer */
static struct membuf ext4l_msg_buf;
static char ext4l_msg_data[EXT4L_MSG_BUF_SIZE];

/**
 * ext4l_get_blk_dev() - Get the current block device
 * Return: Block device descriptor or NULL if not mounted
 */
struct blk_desc *ext4l_get_blk_dev(void)
{
	if (!ext4l_mounted)
		return NULL;
	return ext4l_blk_dev;
}

/**
 * ext4l_get_partition() - Get the current partition info
 * Return: Partition info pointer
 */
struct disk_partition *ext4l_get_partition(void)
{
	return &ext4l_partition;
}

/**
 * ext4l_get_uuid() - Get the filesystem UUID
 * @uuid: Buffer to receive the 16-byte UUID
 * Return: 0 on success, -ENODEV if not mounted
 */
int ext4l_get_uuid(u8 *uuid)
{
	if (!ext4l_sb)
		return -ENODEV;
	memcpy(uuid, ext4l_sb->s_uuid.b, 16);
	return 0;
}

/**
 * ext4l_set_blk_dev() - Set the block device for ext4l operations
 * @blk_dev: Block device descriptor
 * @partition: Partition info (can be NULL for whole disk)
 */
void ext4l_set_blk_dev(struct blk_desc *blk_dev, struct disk_partition *partition)
{
	ext4l_blk_dev = blk_dev;
	if (partition)
		memcpy(&ext4l_partition, partition, sizeof(struct disk_partition));
	else
		memset(&ext4l_partition, 0, sizeof(struct disk_partition));
	ext4l_mounted = 1;
}

/**
 * ext4l_clear_blk_dev() - Clear block device (unmount)
 */
void ext4l_clear_blk_dev(void)
{
	/* Clear buffer cache before unmounting */
	bh_cache_clear();

	ext4l_blk_dev = NULL;
	ext4l_mounted = 0;
}

/**
 * ext4l_msg_init() - Initialize the message buffer
 */
static void ext4l_msg_init(void)
{
	membuf_init(&ext4l_msg_buf, ext4l_msg_data, EXT4L_MSG_BUF_SIZE);
}

/**
 * ext4l_record_msg() - Record a message in the buffer
 * @msg: Message string to record
 * @len: Length of message
 */
void ext4l_record_msg(const char *msg, int len)
{
	membuf_put(&ext4l_msg_buf, msg, len);
}

/**
 * ext4l_get_msg_buf() - Get the message buffer
 *
 * Return: Pointer to the message buffer
 */
struct membuf *ext4l_get_msg_buf(void)
{
	return &ext4l_msg_buf;
}

/**
 * ext4l_print_msgs() - Print all recorded messages
 *
 * Prints the contents of the message buffer to the console.
 */
static void ext4l_print_msgs(void)
{
	char *data;
	int len;

	while ((len = membuf_getraw(&ext4l_msg_buf, 80, true, &data)) > 0)
		printf("%.*s", len, data);
}

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

	/* Initialise message buffer for recording ext4 messages */
	ext4l_msg_init();

	/* Initialise CRC32C table for checksum verification */
	ext4l_crc32c_init();

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

	/* Initialise system zone for block validity checking */
	ret = ext4_init_system_zone();
	if (ret)
		goto err_exit_es;

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

	/* Initialise super_block fields */
	sb->s_bdev->bd_super = sb;
	sb->s_blocksize = 1024;
	sb->s_blocksize_bits = 10;
	snprintf(sb->s_id, sizeof(sb->s_id), "ext4l_mmc%d",
		 fs_dev_desc->devnum);
	sb->s_flags = 0;
	sb->s_fs_info = NULL;

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
	fc->root = (struct dentry *)sb;	/* Hack: store sb for ext4_fill_super */

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

	free(buf);

	/* Save device info for later operations */
	ext4l_dev_desc = fs_dev_desc;
	if (fs_partition)
		memcpy(&ext4l_part, fs_partition, sizeof(ext4l_part));

	/* Set block device for buffer I/O */
	ext4l_set_blk_dev(fs_dev_desc, fs_partition);

	/* Mount the filesystem */
	ret = ext4_fill_super(sb, fc);
	if (ret) {
		printf("ext4l: ext4_fill_super failed: %d\n", ret);
		goto err_free_ctx;
	}

	/* Store super_block for later operations */
	ext4l_sb = sb;

	/* Print messages if ext4l_msgs environment variable is set */
	if (env_get_yesno("ext4l_msgs") == 1)
		ext4l_print_msgs();

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
	ext4l_sb = NULL;
	ext4l_clear_blk_dev();
}

/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * ext4l filesystem interface
 *
 * Copyright 2025 Canonical Ltd
 * Written by Simon Glass <simon.glass@canonical.com>
 */

#ifndef __EXT4L_H__
#define __EXT4L_H__

struct blk_desc;
struct disk_partition;

/**
 * ext4l_probe() - Probe a block device for an ext4 filesystem
 *
 * @fs_dev_desc: Block device descriptor
 * @fs_partition: Partition information
 * Return: 0 on success, -EINVAL if no device or invalid magic,
 *	   -ENOMEM on allocation failure, -EIO on read error
 */
int ext4l_probe(struct blk_desc *fs_dev_desc,
		struct disk_partition *fs_partition);

/**
 * ext4l_close() - Close the ext4 filesystem
 */
void ext4l_close(void);

/**
 * ext4l_get_uuid() - Get the filesystem UUID
 * @uuid: Buffer to receive the 16-byte UUID
 * Return: 0 on success, -ENODEV if not mounted
 */
int ext4l_get_uuid(u8 *uuid);

#endif /* __EXT4L_H__ */

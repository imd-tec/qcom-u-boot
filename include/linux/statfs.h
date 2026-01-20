/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Filesystem statistics definitions for U-Boot
 *
 * Based on Linux statfs.h
 */
#ifndef _LINUX_STATFS_H
#define _LINUX_STATFS_H

#include <linux/types.h>

/**
 * struct kstatfs - kernel filesystem statistics
 * @f_type: filesystem type
 * @f_bsize: optimal transfer block size
 * @f_blocks: total data blocks in filesystem
 * @f_bfree: free blocks in filesystem
 * @f_bavail: free blocks available to unprivileged user
 * @f_files: total file nodes in filesystem
 * @f_ffree: free file nodes in filesystem
 * @f_fsid: filesystem ID
 * @f_namelen: maximum length of filenames
 * @f_frsize: fragment size
 * @f_flags: mount flags
 * @f_spare: spare for later
 */
struct kstatfs {
	long f_type;
	long f_bsize;
	u64 f_blocks;
	u64 f_bfree;
	u64 f_bavail;
	u64 f_files;
	u64 f_ffree;
	__kernel_fsid_t f_fsid;
	long f_namelen;
	long f_frsize;
	long f_flags;
	long f_spare[4];
};

/**
 * uuid_to_fsid - convert UUID to filesystem ID
 * @uuid: UUID to convert (at least 8 bytes)
 *
 * Converts the first 8 bytes of a UUID to a filesystem ID.
 *
 * Return: the filesystem ID
 */
static inline __kernel_fsid_t uuid_to_fsid(const u8 *uuid)
{
	__kernel_fsid_t fsid;

	fsid.val[0] = (uuid[0] << 24) | (uuid[1] << 16) |
		      (uuid[2] << 8) | uuid[3];
	fsid.val[1] = (uuid[4] << 24) | (uuid[5] << 16) |
		      (uuid[6] << 8) | uuid[7];
	return fsid;
}

#endif /* _LINUX_STATFS_H */

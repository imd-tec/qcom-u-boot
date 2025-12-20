/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Filesystem definitions
 *
 * Minimal version for U-Boot ext4l - based on Linux 6.18
 */
#ifndef _LINUX_FS_H
#define _LINUX_FS_H

#include <linux/types.h>
#include <linux/list.h>
#include <linux/mutex.h>

/* Forward declarations */
struct inode;
struct super_block;
struct buffer_head;
struct address_space_operations;

/* errseq_t - error sequence type */
typedef u32 errseq_t;

/* fmode_t - file mode type */
typedef unsigned int fmode_t;

/* File mode flags */
#define FMODE_READ		((__force fmode_t)(1 << 0))
#define FMODE_WRITE		((__force fmode_t)(1 << 1))
#define FMODE_LSEEK		((__force fmode_t)(1 << 2))

/* Buffer operations are in buffer_head.h */

/* address_space - extended for inode.c */
struct address_space {
	struct inode *host;
	errseq_t wb_err;
	unsigned long nrpages;
	unsigned long writeback_index;
	struct list_head i_private_list;
	const struct address_space_operations *a_ops;
};

/* block_device - minimal stub */
struct block_device {
	struct address_space *bd_mapping;
	void *bd_disk;
	struct super_block *bd_super;
};

/* errseq functions - stubs */
static inline int errseq_check(errseq_t *eseq, errseq_t since)
{
	return 0;
}

static inline int errseq_check_and_advance(errseq_t *eseq, errseq_t *since)
{
	return 0;
}

/* File readahead state - stub */
struct file_ra_state {
	unsigned long start;
	unsigned int size;
	unsigned int async_size;
	unsigned int ra_pages;
	unsigned int mmap_miss;
	long long prev_pos;
};

/* file - minimal stub */
struct file {
	fmode_t f_mode;
	struct inode *f_inode;
	unsigned int f_flags;
	struct address_space *f_mapping;
	void *private_data;
	struct file_ra_state f_ra;
};

/* Get inode from file */
static inline struct inode *file_inode(struct file *f)
{
	return f->f_inode;
}

/* iattr - inode attributes for setattr */
struct iattr {
	unsigned int ia_valid;
	umode_t ia_mode;
	uid_t ia_uid;
	gid_t ia_gid;
	loff_t ia_size;
};

/* writeback_control - defined in linux/compat.h */

/* fsnotify - stub */
#define fsnotify_change(d, m)	do { } while (0)

/* inode_init_once - stub */
static inline void inode_init_once(struct inode *inode)
{
}

/* S_ISDIR, etc. - already in linux/stat.h */
#include <linux/stat.h>

#endif /* _LINUX_FS_H */

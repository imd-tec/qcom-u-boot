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
struct file;
struct folio;
struct readahead_control;
struct kiocb;
struct writeback_control;
struct swap_info_struct;

/* errseq_t - error sequence type */
typedef u32 errseq_t;

/* fmode_t - file mode type */
typedef unsigned int fmode_t;

/* File mode flags */
#define FMODE_READ		((__force fmode_t)(1 << 0))
#define FMODE_WRITE		((__force fmode_t)(1 << 1))
#define FMODE_LSEEK		((__force fmode_t)(1 << 2))
#define FMODE_NOWAIT		((__force fmode_t)(1 << 20))
#define FMODE_CAN_ODIRECT	((__force fmode_t)(1 << 21))
#define FMODE_CAN_ATOMIC_WRITE	((__force fmode_t)(1 << 22))

/* Directory file mode flags - use low bits for hash mode */
#define FMODE_32BITHASH		((__force fmode_t)0x00000001)
#define FMODE_64BITHASH		((__force fmode_t)0x00000002)

/* Seek constants */
#ifndef SEEK_HOLE
#define SEEK_HOLE	4
#define SEEK_DATA	3
#endif

/* vfsmount - mount point */
struct vfsmount {
	struct dentry *mnt_root;
};

/* path - pathname components */
struct path {
	struct vfsmount *mnt;
	struct dentry *dentry;
};

/* Buffer operations are in buffer_head.h */

#ifdef __UBOOT__
/* Maximum number of cached folios per address_space */
#define FOLIO_CACHE_MAX 64
#endif

/* address_space_operations - forward declare for address_space */
struct address_space_operations;

/* address_space - extended for inode.c */
struct address_space {
	struct inode *host;
	errseq_t wb_err;
	unsigned long nrpages;
	unsigned long writeback_index;
	struct list_head i_private_list;
	const struct address_space_operations *a_ops;
#ifdef __UBOOT__
	/* Simple folio cache for U-Boot (no XA/radix tree) */
	struct folio *folio_cache[FOLIO_CACHE_MAX];
	int folio_cache_count;
#endif
};

/* address_space_operations - filesystem address space methods */
struct address_space_operations {
	int (*read_folio)(struct file *, struct folio *);
	void (*readahead)(struct readahead_control *);
	sector_t (*bmap)(struct address_space *, sector_t);
	void (*invalidate_folio)(struct folio *, size_t, size_t);
	bool (*release_folio)(struct folio *, gfp_t);
	int (*write_begin)(const struct kiocb *, struct address_space *,
			   loff_t, unsigned, struct folio **, void **);
	int (*write_end)(const struct kiocb *, struct address_space *,
			 loff_t, unsigned, unsigned, struct folio *, void *);
	int (*writepages)(struct address_space *, struct writeback_control *);
	bool (*dirty_folio)(struct address_space *, struct folio *);
	bool (*is_partially_uptodate)(struct folio *, size_t, size_t);
	int (*error_remove_folio)(struct address_space *, struct folio *);
	int (*migrate_folio)(struct address_space *, struct folio *,
			     struct folio *, int);
	int (*swap_activate)(struct swap_info_struct *, struct file *,
			     sector_t *);
};

/* block_device - minimal stub */
struct block_device {
	struct address_space *bd_mapping;
	void *bd_disk;
	struct super_block *bd_super;
	dev_t bd_dev;
	bool read_only;
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
	struct path f_path;
	loff_t f_pos;
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

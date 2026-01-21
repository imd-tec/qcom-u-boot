/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LINUX_IOMAP_H
#define LINUX_IOMAP_H

#include <linux/types.h>

struct bio;
struct dax_device;
struct inode;
struct iomap_iter;
struct kiocb;
struct iov_iter;
struct vm_fault;

/* iomap type values */
#define IOMAP_HOLE	0
#define IOMAP_DELALLOC	1
#define IOMAP_MAPPED	2
#define IOMAP_UNWRITTEN	3
#define IOMAP_INLINE	4

/* iomap flags */
#define IOMAP_F_NEW		(1U << 0)
#define IOMAP_F_DIRTY		(1U << 1)
#define IOMAP_F_SHARED		(1U << 2)
#define IOMAP_F_MERGED		(1U << 3)
#define IOMAP_F_BUFFER_HEAD	(1U << 4)
#define IOMAP_F_ZONE_APPEND	(1U << 5)
#define IOMAP_F_PRIVATE		(1U << 12)

/* Flags for iomap_begin */
#define IOMAP_WRITE		(1 << 0)
#define IOMAP_ZERO		(1 << 1)
#define IOMAP_REPORT		(1 << 2)
#define IOMAP_FAULT		(1 << 3)
#define IOMAP_DIRECT		(1 << 4)
#define IOMAP_NOWAIT		(1 << 5)
#define IOMAP_OVERWRITE_ONLY	(1 << 6)
#define IOMAP_UNSHARE		(1 << 7)
#define IOMAP_DAX		(1 << 8)
#define IOMAP_ATOMIC		(1 << 9)

/* IOMAP_NULL_ADDR indicates a hole/unwritten block address */
#define IOMAP_NULL_ADDR		((u64)-1)

/* Additional iomap flags */
#define IOMAP_F_ATOMIC_BIO	(1U << 6)

/* iomap DIO end_io flags */
#define IOMAP_DIO_UNWRITTEN	(1 << 0)
#define IOMAP_DIO_COW		(1 << 1)

/* iomap_dio_rw flags */
#define IOMAP_DIO_FORCE_WAIT	(1 << 0)
#define IOMAP_DIO_OVERWRITE_ONLY (1 << 1)

struct iomap {
	u64			addr;
	loff_t			offset;
	u64			length;
	u16			type;
	u16			flags;
	struct block_device	*bdev;
	struct dax_device	*dax_dev;
	void			*inline_data;
};

struct iomap_ops {
	int (*iomap_begin)(struct inode *inode, loff_t pos, loff_t length,
			   unsigned int flags, struct iomap *iomap,
			   struct iomap *srcmap);
	int (*iomap_end)(struct inode *inode, loff_t pos, loff_t length,
			 ssize_t written, unsigned int flags,
			 struct iomap *iomap);
};

struct iomap_dio_ops {
	int (*end_io)(struct kiocb *iocb, ssize_t size, int error,
		      unsigned int flags);
	void (*submit_io)(const struct iomap_iter *iter, struct bio *bio,
			  loff_t file_offset);
	struct bio_set *bio_set;
};

struct iomap_iter;

/* Stubs for U-Boot - these are not actually used in read-only mode */
static inline ssize_t
iomap_dio_rw(struct kiocb *iocb, struct iov_iter *iter,
	     const struct iomap_ops *ops, const struct iomap_dio_ops *dops,
	     unsigned int dio_flags, void *private, size_t done_before)
{
	return -EOPNOTSUPP;
}

static inline loff_t
iomap_seek_hole(struct inode *inode, loff_t pos, const struct iomap_ops *ops)
{
	return -EOPNOTSUPP;
}

static inline loff_t
iomap_seek_data(struct inode *inode, loff_t pos, const struct iomap_ops *ops)
{
	return -EOPNOTSUPP;
}

#define iomap_bmap(m, b, o)	({ (void)(m); (void)(b); (void)(o); 0UL; })
#define iomap_swapfile_activate(s, f, sp, o) \
	({ (void)(s); (void)(f); (void)(sp); (void)(o); -EOPNOTSUPP; })

#endif /* LINUX_IOMAP_H */

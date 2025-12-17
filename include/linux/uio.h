/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 *	Berkeley style UIO structures	-	Alan Cox 1994.
 */
#ifndef __LINUX_UIO_H
#define __LINUX_UIO_H

#include <linux/types.h>

struct kvec {
	void *iov_base;
	size_t iov_len;
};

struct iovec {
	void __user *iov_base;
	size_t iov_len;
};

enum iter_type {
	ITER_UBUF,
	ITER_IOVEC,
	ITER_BVEC,
	ITER_KVEC,
	ITER_XARRAY,
	ITER_DISCARD,
};

struct iov_iter {
	u8 iter_type;
	bool nofault;
	bool data_source;
	size_t iov_offset;
	union {
		size_t count;
	};
	union {
		const struct iovec *__iov;
		const struct kvec *kvec;
		const struct bio_vec *bvec;
		struct xarray *xarray;
		void __user *ubuf;
	};
	union {
		unsigned long nr_segs;
		loff_t xarray_start;
	};
};

static inline size_t iov_iter_count(const struct iov_iter *i)
{
	return i->count;
}

static inline void iov_iter_truncate(struct iov_iter *i, size_t count)
{
	if (i->count > count)
		i->count = count;
}

static inline size_t iov_iter_alignment(const struct iov_iter *i)
{
	return 0;  /* Stub - assume aligned */
}

#endif /* __LINUX_UIO_H */

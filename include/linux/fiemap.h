/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Fiemap definitions for U-Boot
 *
 * Based on Linux fiemap.h - extent mapping interface.
 */
#ifndef _LINUX_FIEMAP_H
#define _LINUX_FIEMAP_H

#include <linux/types.h>

/* FIEMAP extent flags */
#define FIEMAP_EXTENT_LAST		0x00000001
#define FIEMAP_EXTENT_UNKNOWN		0x00000002
#define FIEMAP_EXTENT_DELALLOC		0x00000004
#define FIEMAP_EXTENT_UNWRITTEN		0x00000800

/* FIEMAP flags */
#define FIEMAP_FLAG_SYNC		0x00000001
#define FIEMAP_FLAG_XATTR		0x00000002
#define FIEMAP_FLAG_CACHE		0x00000004

/**
 * struct fiemap_extent_info - fiemap request to a filesystem
 * @fi_flags: flags as passed from user
 * @fi_extents_mapped: number of mapped extents
 * @fi_extents_max: size of fiemap_extent array
 * @fi_extents_start: start of fiemap_extent array
 */
struct fiemap_extent_info {
	unsigned int fi_flags;
	unsigned int fi_extents_mapped;
	unsigned int fi_extents_max;
	void *fi_extents_start;
};

/* Fiemap stubs - fiemap not supported in U-Boot */
#define fiemap_prep(i, fi, s, l, f) \
	({ (void)(i); (void)(fi); (void)(s); (void)(l); (void)(f); 0; })
#define fiemap_fill_next_extent(fi, l, p, sz, f) \
	({ (void)(fi); (void)(l); (void)(p); (void)(sz); (void)(f); 0; })
#define iomap_fiemap(i, fi, s, l, o) \
	({ (void)(i); (void)(fi); (void)(s); (void)(l); (void)(o); 0; })

#endif /* _LINUX_FIEMAP_H */

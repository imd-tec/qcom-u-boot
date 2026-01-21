/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Pathname lookup definitions for U-Boot
 *
 * Based on Linux namei.h - pathname resolution flags.
 */
#ifndef _LINUX_NAMEI_H
#define _LINUX_NAMEI_H

/* Pathname lookup flags */
#define LOOKUP_FOLLOW		0x0001	/* Follow links at end */
#define LOOKUP_DIRECTORY	0x0002	/* Require a directory */
#define LOOKUP_AUTOMOUNT	0x0004	/* Force terminal automount */
#define LOOKUP_EMPTY		0x0008	/* Accept empty path */
#define LOOKUP_DOWN		0x0020	/* Follow mounts at start */
#define LOOKUP_MOUNTPOINT	0x0040	/* Follow mounts at end */
#define LOOKUP_REVAL		0x0080	/* Revalidate cache */
#define LOOKUP_RCU		0x0100	/* RCU mode */
#define LOOKUP_CACHED		0x0200	/* Cached lookup only */
#define LOOKUP_PARENT		0x0400	/* Looking up parent */
#define LOOKUP_OPEN		0x10000	/* Opening file */
#define LOOKUP_CREATE		0x20000	/* Creating file */
#define LOOKUP_EXCL		0x40000	/* Exclusive create */
#define LOOKUP_RENAME_TARGET	0x80000	/* Rename target */

#include <linux/minmax.h>

/* nd_terminate_link - terminate symlink string */
static inline void nd_terminate_link(void *name, loff_t len, int maxlen)
{
	((char *)name)[min_t(loff_t, len, maxlen)] = '\0';
}

#endif /* _LINUX_NAMEI_H */

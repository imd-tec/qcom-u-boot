/* SPDX-License-Identifier: GPL-2.0 */
/*
 * include/linux/backing-dev.h
 *
 * low-level device information and state which is propagated up through
 * to high-level code.
 */

#ifndef _LINUX_BACKING_DEV_H
#define _LINUX_BACKING_DEV_H

#include <linux/types.h>

struct backing_dev_info {
	unsigned long ra_pages;
	unsigned long io_pages;
};

/* Stub for inode_to_bdi - returns NULL since we don't use backing dev */
static inline struct backing_dev_info *inode_to_bdi(struct inode *inode)
{
	return NULL;
}

#endif /* _LINUX_BACKING_DEV_H */

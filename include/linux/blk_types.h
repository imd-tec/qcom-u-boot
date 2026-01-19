/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Block I/O types
 *
 * Copyright 2025 Canonical Ltd
 * Written by Simon Glass <simon.glass@canonical.com>
 *
 * Minimal version for U-Boot - based on Linux
 */
#ifndef _LINUX_BLK_TYPES_H
#define _LINUX_BLK_TYPES_H

#include <linux/types.h>

/* Sector size definitions */
#ifndef SECTOR_SHIFT
#define SECTOR_SHIFT		9
#endif
#ifndef SECTOR_SIZE
#define SECTOR_SIZE		(1 << SECTOR_SHIFT)
#endif

/* Block I/O operation flags */
typedef __u32 __bitwise blk_opf_t;

/* Block operation codes (bits 0-7) */
#define REQ_OP_READ		0
#define REQ_OP_WRITE		1
#define REQ_OP_MASK		0xff

/* Block request flags (bits 8+) */
#define REQ_SYNC		(1 << 8)	/* Synchronous I/O */
#define REQ_FUA			(1 << 9)	/* Forced unit access */
#define REQ_PREFLUSH		(1 << 10)	/* Request cache flush */
#define REQ_IDLE		(1 << 11)	/* Anticipate more I/O */
#define REQ_META		(1 << 12)	/* Metadata I/O request */
#define REQ_PRIO		(1 << 13)	/* Boost priority */
#define REQ_RAHEAD		(1 << 14)	/* Read ahead */

#endif /* _LINUX_BLK_TYPES_H */

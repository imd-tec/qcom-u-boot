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

/* Block I/O operation flags */
typedef __u32 __bitwise blk_opf_t;

/* Block operation codes (bits 0-7) */
#define REQ_OP_READ		0
#define REQ_OP_WRITE		1
#define REQ_OP_MASK		0xff

/* Block request flags (bits 8+) */
#define REQ_SYNC		(1 << 8)	/* Synchronous I/O */
#define REQ_FUA			(1 << 9)	/* Forced unit access */

#endif /* _LINUX_BLK_TYPES_H */

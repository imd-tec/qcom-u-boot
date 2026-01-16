/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Device number macros for U-Boot
 *
 * Based on Linux kdev_t.h
 */
#ifndef _LINUX_KDEV_T_H
#define _LINUX_KDEV_T_H

#include <linux/types.h>

/* Number of minor bits */
#ifndef MINORBITS
#define MINORBITS	20
#endif

/* Minor number mask */
#ifndef MINORMASK
#define MINORMASK	((1U << MINORBITS) - 1)
#endif

/**
 * MAJOR - extract major number from dev_t
 * @dev: device number
 */
#ifndef MAJOR
#define MAJOR(dev)	((unsigned int)((dev) >> MINORBITS))
#endif

/**
 * MINOR - extract minor number from dev_t
 * @dev: device number
 */
#ifndef MINOR
#define MINOR(dev)	((unsigned int)((dev) & MINORMASK))
#endif

/**
 * MKDEV - create dev_t from major and minor numbers
 * @ma: major number
 * @mi: minor number
 */
#ifndef MKDEV
#define MKDEV(ma, mi)	(((ma) << MINORBITS) | (mi))
#endif

/* Old-style device number encoding (8:8) */
#define old_valid_dev(dev)	(MAJOR(dev) < 256 && MINOR(dev) < 256)
#define old_encode_dev(dev)	((MAJOR(dev) << 8) | MINOR(dev))
#define old_decode_dev(dev)	MKDEV((dev) >> 8, (dev) & 0xff)

/* New-style device number encoding (pass-through) */
#define new_encode_dev(dev)	((unsigned int)(dev))
#define new_decode_dev(dev)	((dev_t)(dev))

#endif /* _LINUX_KDEV_T_H */

/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Stub definitions for partition statistics.
 * U-Boot doesn't track I/O statistics.
 */
#ifndef _LINUX_PART_STAT_H
#define _LINUX_PART_STAT_H

#define STAT_READ	0
#define STAT_WRITE	1

#define part_stat_read(bdev, field)	0
#define part_stat_inc(bdev, field)	do { } while (0)
#define part_stat_add(bdev, field, val)	do { } while (0)

#endif /* _LINUX_PART_STAT_H */

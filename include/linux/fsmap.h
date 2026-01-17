/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Filesystem map definitions for U-Boot
 *
 * Based on Linux fsmap.h - for FS_IOC_GETFSMAP ioctl.
 */
#ifndef _LINUX_FSMAP_H
#define _LINUX_FSMAP_H

#include <linux/types.h>

/**
 * struct fsmap - filesystem extent mapping
 * @fmr_device: device identifier
 * @fmr_flags: mapping flags
 * @fmr_physical: physical offset on device
 * @fmr_owner: owner identifier
 * @fmr_offset: logical offset in file
 * @fmr_length: length of the extent
 * @fmr_reserved: reserved (must be zero)
 */
struct fsmap {
	__u32 fmr_device;
	__u32 fmr_flags;
	__u64 fmr_physical;
	__u64 fmr_owner;
	__u64 fmr_offset;
	__u64 fmr_length;
	__u64 fmr_reserved[3];
};

/* Special owner values */
#define FMR_OWN_FREE		(-1ULL)
#define FMR_OWN_UNKNOWN		(-2ULL)

/* Construct owner value from type and code */
#define FMR_OWNER(type, code)	(((__u64)(type) << 32) | (__u64)(code))

/* fsmap flags */
#define FMR_OF_SPECIAL_OWNER	(1 << 0)

/* fsmap head flags */
#define FMH_IF_VALID		0
#define FMH_OF_DEV_T		(1 << 0)

#endif /* _LINUX_FSMAP_H */

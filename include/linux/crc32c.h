/* SPDX-License-Identifier: GPL-2.0 */
/*
 * CRC32C definitions
 *
 * Minimal version for U-Boot ext4l - based on Linux 6.18
 */
#ifndef _LINUX_CRC32C_H
#define _LINUX_CRC32C_H

#include <linux/types.h>
#include <u-boot/crc.h>

/* Use U-Boot's CRC32 implementation */
static inline u32 crc32c(u32 crc, const void *address, unsigned int length)
{
	return crc32(crc, address, length);
}

#define crc32c_le(crc, p, len)	crc32c(crc, p, len)

#endif /* _LINUX_CRC32C_H */

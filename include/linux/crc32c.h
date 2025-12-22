/* SPDX-License-Identifier: GPL-2.0 */
/*
 * CRC32C definitions for ext4l
 *
 * CRC32C (Castagnoli) uses polynomial 0x1EDC6F41 (bit-reflected: 0x82F63B78)
 * This is different from standard CRC32 (IEEE 802.3) which uses 0x04C11DB7.
 *
 * ext4l provides its own implementation to avoid conflicts with other
 * filesystems (e.g., btrfs) that have their own crc32c().
 */
#ifndef _LINUX_CRC32C_H
#define _LINUX_CRC32C_H

#include <asm/byteorder.h>
#include <linux/types.h>

u32 ext4l_crc32c(u32 crc, const void *address, unsigned int length);

#define crc32c(crc, p, len)	ext4l_crc32c(crc, p, len)
#define crc32c_le(crc, p, len)	ext4l_crc32c(crc, p, len)

#endif /* _LINUX_CRC32C_H */

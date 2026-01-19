/* SPDX-License-Identifier: GPL-2.0 */
/*
 * High UID/GID to low UID/GID conversion helpers
 *
 * Based on Linux highuid.h
 */
#ifndef _LINUX_HIGHUID_H
#define _LINUX_HIGHUID_H

/*
 * U-Boot doesn't support 16-bit UIDs/GIDs overflow handling,
 * so these are simplified versions.
 */
#define low_16_bits(x)		((x) & 0xFFFF)
#define high_16_bits(x)		(((x) >> 16) & 0xFFFF)
#define fs_high2lowuid(uid)	((uid) & 0xFFFF)
#define fs_high2lowgid(gid)	((gid) & 0xFFFF)

#endif /* _LINUX_HIGHUID_H */

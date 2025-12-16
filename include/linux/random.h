/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Stub definitions for random number generation.
 */
#ifndef _LINUX_RANDOM_H
#define _LINUX_RANDOM_H

#include <linux/types.h>

#define get_random_bytes(buf, len)	do { } while (0)
#define prandom_u32()			0
#define get_random_u32()		0
#define get_random_u64()		0ULL

#endif /* _LINUX_RANDOM_H */

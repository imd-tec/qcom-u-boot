/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Jiffies and time conversion
 *
 * Copyright 2025 Canonical Ltd
 * Written by Simon Glass <simon.glass@canonical.com>
 *
 * Minimal version for U-Boot - based on Linux
 */
#ifndef _LINUX_JIFFIES_H
#define _LINUX_JIFFIES_H

#include <linux/types.h>
#include <limits.h>

#define MAX_JIFFY_OFFSET	((LONG_MAX >> 1) - 1)

/* HZ - timer frequency (simplified for U-Boot) */
#define HZ			1000

/* jiffies - always 0 in U-Boot (no timer tick counter) */
#define jiffies			0UL

/* Time comparison macros are in include/time.h */

/* Jiffies conversion */
#define msecs_to_jiffies(m)	((m) * HZ / 1000)
#define jiffies_to_msecs(j)	((j) * 1000 / HZ)
#define nsecs_to_jiffies(ns)	((ns) / (1000000000L / HZ))
#define round_jiffies_up(j)	(j)

/* Time comparison - stub for U-Boot (jiffies is always 0) */
#define time_is_before_jiffies(a)	({ (void)(a); 0; })

#endif /* _LINUX_JIFFIES_H */

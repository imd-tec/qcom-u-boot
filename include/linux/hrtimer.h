/* SPDX-License-Identifier: GPL-2.0 */
/*
 * High-resolution timer definitions for U-Boot
 *
 * Based on Linux hrtimer.h - high resolution timers.
 * U-Boot stub - high-resolution timers not supported.
 */
#ifndef _LINUX_HRTIMER_H
#define _LINUX_HRTIMER_H

/* High-resolution timer modes */
enum hrtimer_mode {
	HRTIMER_MODE_ABS	= 0x00,
	HRTIMER_MODE_REL	= 0x01,
	HRTIMER_MODE_PINNED	= 0x02,
	HRTIMER_MODE_SOFT	= 0x04,
	HRTIMER_MODE_HARD	= 0x08,

	HRTIMER_MODE_ABS_PINNED = HRTIMER_MODE_ABS | HRTIMER_MODE_PINNED,
	HRTIMER_MODE_REL_PINNED = HRTIMER_MODE_REL | HRTIMER_MODE_PINNED,
	HRTIMER_MODE_ABS_SOFT	= HRTIMER_MODE_ABS | HRTIMER_MODE_SOFT,
	HRTIMER_MODE_REL_SOFT	= HRTIMER_MODE_REL | HRTIMER_MODE_SOFT,
	HRTIMER_MODE_ABS_HARD	= HRTIMER_MODE_ABS | HRTIMER_MODE_HARD,
	HRTIMER_MODE_REL_HARD	= HRTIMER_MODE_REL | HRTIMER_MODE_HARD,
};

/**
 * schedule_hrtimeout() - sleep until timeout with high-resolution timer
 * @expires: timeout value (ktime_t)
 * @mode: timer mode
 *
 * U-Boot stub - returns immediately.
 *
 * Return: 0
 */
#define schedule_hrtimeout(expires, mode) \
	({ (void)(expires); (void)(mode); 0; })

#endif /* _LINUX_HRTIMER_H */

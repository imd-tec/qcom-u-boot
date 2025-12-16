/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Stub definitions for Linux kernel timer support.
 * U-Boot doesn't use kernel timers.
 */
#ifndef _LINUX_TIMER_H
#define _LINUX_TIMER_H

struct timer_list {
	unsigned long expires;
	void (*function)(struct timer_list *);
	unsigned long data;
};

#define DEFINE_TIMER(name, func)	\
	struct timer_list name = { .function = func }

#define setup_timer(timer, func, data)		do { } while (0)
#define timer_setup(timer, func, flags)		do { } while (0)
#define init_timer(timer)			do { } while (0)
#define add_timer(timer)			do { } while (0)
#define del_timer(timer)			0
#define del_timer_sync(timer)			do { } while (0)
#define mod_timer(timer, expires)		0
#define timer_pending(timer)			0

#define from_timer(var, callback_timer, timer_fieldname)	\
	container_of(callback_timer, typeof(*var), timer_fieldname)

#endif /* _LINUX_TIMER_H */

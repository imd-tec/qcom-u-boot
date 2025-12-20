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

/* Use macros for functions taking callback pointers to avoid requiring
 * the callback to be declared (some callers have them in #ifdef blocks)
 */
#define setup_timer(timer, func, data)		do { } while (0)
#define timer_setup(timer, func, flags)		do { } while (0)

static inline void init_timer(struct timer_list *timer)
{
}

static inline void add_timer(struct timer_list *timer)
{
}

static inline int del_timer(struct timer_list *timer)
{
	return 0;
}

#define del_timer_sync(timer)			do { } while (0)

static inline int mod_timer(struct timer_list *timer, unsigned long expires)
{
	return 0;
}

static inline int timer_pending(struct timer_list *timer)
{
	return 0;
}

#define from_timer(var, callback_timer, timer_fieldname)	\
	container_of(callback_timer, typeof(*var), timer_fieldname)

#define timer_container_of(var, callback_timer, timer_fieldname)	\
	container_of(callback_timer, typeof(*var), timer_fieldname)

static inline void timer_shutdown_sync(struct timer_list *timer)
{
}

static inline void timer_delete_sync(struct timer_list *timer)
{
}

#endif /* _LINUX_TIMER_H */

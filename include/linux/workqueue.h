/* SPDX-License-Identifier: GPL-2.0 */
/*
 * workqueue.h --- work queue handling for Linux.
 *
 * Stub definitions for Linux kernel workqueue support.
 * U-Boot doesn't use workqueues.
 */
#ifndef _LINUX_WORKQUEUE_H
#define _LINUX_WORKQUEUE_H

struct work_struct {
	void (*func)(struct work_struct *);
};

struct delayed_work {
	struct work_struct work;
};

struct workqueue_struct;

/* Use macros for functions taking callback pointers to avoid requiring
 * the callback to be declared (some callers have them in #ifdef blocks)
 */
#define INIT_WORK(work, func)			do { } while (0)
#define INIT_DELAYED_WORK(work, func)		do { } while (0)

static inline void schedule_work(struct work_struct *work)
{
}

static inline int schedule_delayed_work(struct delayed_work *work,
					unsigned long delay)
{
	return 0;
}

static inline int cancel_work_sync(struct work_struct *work)
{
	return 0;
}

static inline int cancel_delayed_work(struct delayed_work *work)
{
	return 0;
}

static inline int cancel_delayed_work_sync(struct delayed_work *work)
{
	return 0;
}

static inline int flush_work(struct work_struct *work)
{
	return 0;
}

static inline int flush_delayed_work(struct delayed_work *work)
{
	return 0;
}

static inline int queue_work(struct workqueue_struct *wq,
			     struct work_struct *work)
{
	return 0;
}

static inline int queue_delayed_work(struct workqueue_struct *wq,
				     struct delayed_work *work,
				     unsigned long delay)
{
	return 0;
}

#define alloc_workqueue(fmt, flags, max, ...)	((struct workqueue_struct *)1)
#define create_singlethread_workqueue(name)	((struct workqueue_struct *)1)

static inline void destroy_workqueue(struct workqueue_struct *wq)
{
}

/* System workqueues - all stubs in U-Boot */
#define system_dfl_wq		((struct workqueue_struct *)1)
#define system_wq		((struct workqueue_struct *)1)

#endif /* _LINUX_WORKQUEUE_H */

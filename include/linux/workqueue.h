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

#define INIT_WORK(work, func)			do { } while (0)
#define INIT_DELAYED_WORK(work, func)		do { } while (0)
#define schedule_work(work)			do { } while (0)
#define schedule_delayed_work(work, delay)	0
#define cancel_work_sync(work)			0
#define cancel_delayed_work(work)		0
#define cancel_delayed_work_sync(work)		0
#define flush_work(work)			0
#define flush_delayed_work(work)		0
#define queue_work(wq, work)			0
#define queue_delayed_work(wq, work, delay)	0

#define alloc_workqueue(fmt, flags, max, ...)	((struct workqueue_struct *)1)
#define create_singlethread_workqueue(name)	((struct workqueue_struct *)1)
#define destroy_workqueue(wq)			do { } while (0)

struct workqueue_struct;

#endif /* _LINUX_WORKQUEUE_H */

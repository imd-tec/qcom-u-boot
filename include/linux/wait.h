/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Linux wait queue related types and methods
 *
 * Stub definitions for Linux kernel wait queues.
 * U-Boot doesn't use wait queues.
 */
#ifndef _LINUX_WAIT_H
#define _LINUX_WAIT_H

typedef int wait_queue_head_t;

struct wait_queue_entry {
	int dummy;
};

#define DECLARE_WAITQUEUE(name, task)	do { } while (0)
#define DECLARE_WAIT_QUEUE_HEAD(name)	do { } while (0)

#define init_waitqueue_head(wq)		do { } while (0)
#define add_wait_queue(wq, entry)	do { } while (0)
#define remove_wait_queue(wq, entry)	do { } while (0)
#define wake_up(wq)			do { } while (0)
#define wake_up_all(wq)			do { } while (0)
#define wake_up_interruptible(wq)	do { } while (0)
#define wake_up_interruptible_all(wq)	do { } while (0)

#define wait_event(wq, condition)	do { } while (0)
#define wait_event_interruptible(wq, condition)	0

#endif /* _LINUX_WAIT_H */

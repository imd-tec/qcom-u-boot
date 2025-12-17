/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Define 'struct task_struct' and provide the main scheduler
 * APIs (schedule(), wakeup variants, etc.)
 *
 * Stub definitions for Linux kernel scheduler.
 * U-Boot is single-threaded.
 */
#ifndef _LINUX_SCHED_H
#define _LINUX_SCHED_H

#include <linux/types.h>

struct task_struct {
	int pid;
	char comm[16];
};

extern struct task_struct *current;

#define TASK_RUNNING		0
#define TASK_INTERRUPTIBLE	1
#define TASK_UNINTERRUPTIBLE	2

#define cond_resched()		do { } while (0)
#define yield()			do { } while (0)
/* Note: schedule() is implemented in common/cyclic.c */

#define in_interrupt()		0
#define in_atomic()		0
#define in_task()		1

#define signal_pending(task)	0
#define fatal_signal_pending(task)	0

#endif /* _LINUX_SCHED_H */

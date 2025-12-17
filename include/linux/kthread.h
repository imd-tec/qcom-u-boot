/* SPDX-License-Identifier: GPL-2.0 */
/* Simple interface for creating and stopping kernel threads without mess.
 *
 * Stub definitions for Linux kernel thread support.
 * U-Boot doesn't have multi-threading.
 */
#ifndef _LINUX_KTHREAD_H
#define _LINUX_KTHREAD_H

struct task_struct;

#define kthread_create(fn, data, fmt, ...)	\
	((struct task_struct *)__builtin_return_address(0))
#define kthread_run(fn, data, fmt, ...)		\
	((struct task_struct *)__builtin_return_address(0))
#define kthread_stop(task)		do { } while (0)
#define kthread_should_stop()		0
#define kthread_should_park()		0
#define kthread_park(task)		0
#define kthread_unpark(task)		do { } while (0)
#define kthread_parkme()		do { } while (0)

#define wake_up_process(task)		do { } while (0)
#define set_current_state(state)	do { } while (0)

#define task_pid_nr(task)		0

#endif /* _LINUX_KTHREAD_H */

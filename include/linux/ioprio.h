/* SPDX-License-Identifier: GPL-2.0 */
/*
 * I/O priority definitions for U-Boot
 *
 * Based on Linux ioprio.h - I/O scheduling priority.
 * U-Boot stub - I/O priority not supported.
 */
#ifndef IOPRIO_H
#define IOPRIO_H

/* I/O priority classes */
#define IOPRIO_CLASS_NONE	0
#define IOPRIO_CLASS_RT		1
#define IOPRIO_CLASS_BE		2
#define IOPRIO_CLASS_IDLE	3

/* I/O priority levels (0-7, lower is higher priority) */
#define IOPRIO_NR_LEVELS	8
#define IOPRIO_BE_NR		IOPRIO_NR_LEVELS

/**
 * IOPRIO_PRIO_VALUE() - create I/O priority value from class and level
 * @class: I/O priority class
 * @data: priority level within class
 *
 * Return: encoded priority value
 */
#define IOPRIO_PRIO_VALUE(class, data)	(((class) << 13) | (data))

/**
 * IOPRIO_PRIO_CLASS() - extract class from priority value
 * @ioprio: encoded priority
 *
 * Return: I/O priority class
 */
#define IOPRIO_PRIO_CLASS(ioprio)	(((ioprio) >> 13) & 0x3)

/**
 * IOPRIO_PRIO_DATA() - extract data/level from priority value
 * @ioprio: encoded priority
 *
 * Return: priority level
 */
#define IOPRIO_PRIO_DATA(ioprio)	((ioprio) & 0x1fff)

/**
 * get_current_ioprio() - get I/O priority of current task
 *
 * U-Boot stub - always returns 0.
 *
 * Return: I/O priority value
 */
#define get_current_ioprio()		(0)

/**
 * set_task_ioprio() - set I/O priority of a task
 * @task: task to modify (ignored)
 * @ioprio: priority to set (ignored)
 *
 * U-Boot stub - no-op.
 */
#define set_task_ioprio(task, ioprio)	do { (void)(task); (void)(ioprio); } while (0)

#endif /* IOPRIO_H */

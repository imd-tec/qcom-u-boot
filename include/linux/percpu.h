/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Per-CPU variable and operation stubs for U-Boot
 *
 * U-Boot is single-threaded, so per-CPU variables are just regular
 * variables and per-CPU operations are simple direct accesses.
 */
#ifndef _LINUX_PERCPU_H
#define _LINUX_PERCPU_H

#include <linux/types.h>
#include <malloc.h>

/*
 * Per-CPU variable definitions - just regular variables in U-Boot
 */
#define DEFINE_PER_CPU(type, name)	type name
#define per_cpu(var, cpu)		(var)
#define per_cpu_ptr(ptr, cpu)		(ptr)
#define this_cpu_inc(var)		((var)++)
#define this_cpu_read(var)		(var)

/* CPU iteration - only one CPU in U-Boot */
#define for_each_possible_cpu(cpu)	for ((cpu) = 0; (cpu) < 1; (cpu)++)
#define smp_processor_id()		0
#define num_possible_cpus()		1

/* Per-CPU allocation - just regular allocation in U-Boot */
#define alloc_percpu(type)		((type *)kzalloc(sizeof(type), GFP_KERNEL))
#define free_percpu(ptr)		kfree(ptr)

/*
 * Per-CPU read-write semaphore stubs
 * U-Boot is single-threaded, so these are no-ops
 */
struct percpu_rw_semaphore {
	int dummy;
};

#define percpu_down_read(sem)		do { } while (0)
#define percpu_up_read(sem)		do { } while (0)
#define percpu_down_write(sem)		do { } while (0)
#define percpu_up_write(sem)		do { } while (0)

static inline int percpu_init_rwsem(struct percpu_rw_semaphore *sem)
{
	return 0;
}

static inline void percpu_free_rwsem(struct percpu_rw_semaphore *sem)
{
}

#endif /* _LINUX_PERCPU_H */

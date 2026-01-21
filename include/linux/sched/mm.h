/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Memory allocation context helpers for U-Boot
 *
 * Based on Linux include/linux/sched/mm.h
 */
#ifndef _LINUX_SCHED_MM_H
#define _LINUX_SCHED_MM_H

/**
 * memalloc_nofs_save() - Mark implicit GFP_NOFS allocation scope
 *
 * U-Boot stub - no filesystem allocation context tracking needed.
 *
 * Return: 0 (no flags to restore)
 */
static inline unsigned int memalloc_nofs_save(void)
{
	return 0;
}

/**
 * memalloc_nofs_restore() - End implicit GFP_NOFS scope
 * @flags: flags returned by memalloc_nofs_save()
 *
 * U-Boot stub - no filesystem allocation context tracking needed.
 */
static inline void memalloc_nofs_restore(unsigned int flags)
{
}

/* Memory allocation retry wait - stub for U-Boot */
#define memalloc_retry_wait(g)		do { } while (0)

#endif /* _LINUX_SCHED_MM_H */

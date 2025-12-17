/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Sorting functions - use stdlib qsort.
 */
#ifndef _LINUX_SORT_H
#define _LINUX_SORT_H

#include <linux/types.h>
#include <stdlib.h>

typedef int (*cmp_func_t)(const void *, const void *);

static inline void sort(void *base, size_t num, size_t size,
			cmp_func_t cmp, void *swap)
{
	qsort(base, num, size, cmp);
}

#endif /* _LINUX_SORT_H */

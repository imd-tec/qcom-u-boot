/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_PFN_T_H
#define _LINUX_PFN_T_H

#include <linux/types.h>

/*
 * pfn_t is a type that encapsulates a page frame number along with
 * flags about how it should be used. For U-Boot, we just need a
 * minimal definition.
 */
typedef struct {
	u64 val;
} pfn_t;

#define PFN_DEV		(1ULL << 56)
#define PFN_MAP		(1ULL << 57)

static inline pfn_t pfn_to_pfn_t(unsigned long pfn)
{
	pfn_t pfn_t = { .val = pfn };

	return pfn_t;
}

static inline unsigned long pfn_t_to_pfn(pfn_t pfn)
{
	return pfn.val & ~(PFN_DEV | PFN_MAP);
}

#endif /* _LINUX_PFN_T_H */

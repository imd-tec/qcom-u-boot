/* SPDX-License-Identifier: GPL-2.0 */
/*
 * include/linux/pagevec.h
 *
 * In many places it is efficient to batch an operation up against multiple
 * folios.  A folio_batch is a container which is used for that.
 */

#ifndef _LINUX_PAGEVEC_H
#define _LINUX_PAGEVEC_H

#include <linux/types.h>

/* Minimal stub - pagevec is used for batching page operations */

#define PAGEVEC_SIZE	16

struct folio;

struct folio_batch {
	unsigned char nr;
	unsigned char i;
	bool percpu_pvec_drained;
	struct folio *folios[PAGEVEC_SIZE];
};

static inline void folio_batch_init(struct folio_batch *fbatch)
{
	fbatch->nr = 0;
	fbatch->i = 0;
	fbatch->percpu_pvec_drained = false;
}

static inline unsigned int folio_batch_count(struct folio_batch *fbatch)
{
	return fbatch->nr;
}

static inline unsigned int folio_batch_add(struct folio_batch *fbatch,
					   struct folio *folio)
{
	fbatch->folios[fbatch->nr++] = folio;
	return PAGEVEC_SIZE - fbatch->nr;
}

#endif /* _LINUX_PAGEVEC_H */

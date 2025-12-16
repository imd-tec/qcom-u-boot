/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Stub definitions for swap/memory management.
 * U-Boot doesn't use swap.
 */
#ifndef _LINUX_SWAP_H
#define _LINUX_SWAP_H

#define mark_page_accessed(page)	do { } while (0)

struct address_space;
struct folio;

static inline void folio_mark_accessed(struct folio *folio)
{
}

#endif /* _LINUX_SWAP_H */

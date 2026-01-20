/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_DAX_H
#define _LINUX_DAX_H

#include <linux/types.h>
#include <linux/pfn_t.h>
#include <linux/mm_types.h>

struct address_space;
struct dax_device;
struct vm_area_struct;
struct iomap_ops;
struct kiocb;
struct iov_iter;
struct vm_fault;
struct file;
struct inode;

/* DAX is not supported in U-Boot - provide stubs */
static inline ssize_t
dax_iomap_rw(struct kiocb *iocb, struct iov_iter *iter,
	     const struct iomap_ops *ops)
{
	return -EOPNOTSUPP;
}

static inline vm_fault_t
dax_iomap_fault(struct vm_fault *vmf, unsigned int order, pfn_t *pfnp,
		int *errp, const struct iomap_ops *ops)
{
	return VM_FAULT_SIGBUS;
}

static inline vm_fault_t
dax_finish_sync_fault(struct vm_fault *vmf, unsigned int order, pfn_t pfn)
{
	return VM_FAULT_SIGBUS;
}

static inline bool dax_mapping(struct address_space *mapping)
{
	return false;
}

/* 3-arg version used by ext4 */
#define daxdev_mapping_supported(f, i, d) ({ (void)(f); (void)(i); (void)(d); 1; })

/* DAX stubs */
#define IS_DAX(inode)				(0)
#define dax_break_layout_final(inode)		do { } while (0)
#define dax_writeback_mapping_range(m, bd, wb)	({ (void)(m); (void)(bd); (void)(wb); 0; })
#define dax_zero_range(i, p, l, d, op) \
	({ (void)(i); (void)(p); (void)(l); (void)(d); (void)(op); -EOPNOTSUPP; })
#define dax_break_layout_inode(i, m)		({ (void)(i); (void)(m); 0; })

#endif /* _LINUX_DAX_H */

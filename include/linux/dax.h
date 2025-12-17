/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_DAX_H
#define _LINUX_DAX_H

#include <linux/types.h>
#include <linux/pfn_t.h>

struct address_space;
struct dax_device;
struct vm_area_struct;
struct iomap_ops;
struct kiocb;
struct iov_iter;
struct vm_fault;

typedef unsigned int vm_fault_t;

#define VM_FAULT_SIGBUS		0x0002
#define VM_FAULT_NOPAGE		0x0100

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

static inline bool daxdev_mapping_supported(struct vm_area_struct *vma,
					    struct dax_device *dax_dev)
{
	return false;
}

#endif /* _LINUX_DAX_H */

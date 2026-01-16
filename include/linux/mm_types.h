/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Memory management types stub for U-Boot
 *
 * U-Boot doesn't have virtual memory management, so these are stubs.
 */
#ifndef _LINUX_MM_TYPES_H
#define _LINUX_MM_TYPES_H

#include <linux/types.h>

/* Forward declarations */
struct file;
struct folio;
struct address_space;

/**
 * struct page - minimal stub for page structure
 * @flags: page flags
 *
 * U-Boot stub - only the flags field is provided.
 */
struct page {
	unsigned long flags;
};

/**
 * typedef vm_fault_t - return type for page fault handlers
 *
 * Encodes the result of a page fault.
 */
typedef unsigned int vm_fault_t;

/* VM flags for vm_area_struct */
#define VM_SHARED		0x00000008
#define VM_WRITE		0x00000002
#define VM_HUGEPAGE		0x01000000

/* Fault flags */
#define FAULT_FLAG_WRITE	0x01

/* VM fault return values */
#define VM_FAULT_SIGBUS		0x0002
#define VM_FAULT_NOPAGE		0x0010
#define VM_FAULT_LOCKED		0x0200

/* Maximum order for page cache allocations */
#define MAX_PAGECACHE_ORDER	12

struct vm_operations_struct;

/**
 * struct vm_area_struct - virtual memory area
 * @vm_start: start address
 * @vm_end: end address
 * @vm_file: file this vma is associated with
 * @vm_flags: VM flags
 *
 * U-Boot stub.
 */
struct vm_area_struct {
	unsigned long vm_start;
	unsigned long vm_end;
	struct file *vm_file;
	unsigned long vm_flags;
};

/**
 * struct vm_fault - virtual memory fault info
 * @vma: virtual memory area
 * @address: faulting address
 * @flags: fault flags
 * @pgoff: page offset
 * @folio: folio being faulted
 * @page: page being faulted
 *
 * U-Boot stub.
 */
struct vm_fault {
	struct vm_area_struct *vma;
	unsigned long address;
	unsigned int flags;
	pgoff_t pgoff;
	struct folio *folio;
	struct page *page;
};

/**
 * struct vm_operations_struct - virtual memory area operations
 *
 * Callbacks for VM operations. U-Boot stub.
 */
struct vm_operations_struct {
	vm_fault_t (*fault)(struct vm_fault *vmf);
	vm_fault_t (*huge_fault)(struct vm_fault *vmf, unsigned int order);
	vm_fault_t (*page_mkwrite)(struct vm_fault *vmf);
	vm_fault_t (*pfn_mkwrite)(struct vm_fault *vmf);
	vm_fault_t (*map_pages)(struct vm_fault *vmf, pgoff_t start,
				pgoff_t end);
};

/**
 * struct vm_area_desc - for mmap_prepare
 * @file: associated file
 * @vm_flags: VM flags
 * @vm_ops: VM operations
 *
 * U-Boot stub.
 */
struct vm_area_desc {
	struct file *file;
	unsigned long vm_flags;
	const struct vm_operations_struct *vm_ops;
};

#endif /* _LINUX_MM_TYPES_H */

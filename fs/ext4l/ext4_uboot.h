/* SPDX-License-Identifier: GPL-2.0 */
/*
 * U-Boot compatibility header for ext4l filesystem
 *
 * Copyright 2025 Canonical Ltd
 * Written by Simon Glass <simon.glass@canonical.com>
 *
 * This provides minimal definitions to allow Linux ext4 code to compile
 * in U-Boot.
 */

#ifndef __EXT4_UBOOT_H__
#define __EXT4_UBOOT_H__

/*
 * Suppress warnings for unused static functions and variables in Linux ext4
 * source files. These are used in code paths that are stubbed out in U-Boot.
 */
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"

#include <div64.h>
#include <linux/types.h>
#include <linux/bitops.h>
#include <vsprintf.h>		/* For panic() */
#include <linux/string.h>
#include <linux/stat.h>
#include <asm/byteorder.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/log2.h>
#include <linux/init.h>
#include <linux/math64.h>
#include <linux/workqueue.h>
#include <linux/cred.h>
#include <linux/fs.h>
#include <linux/iomap.h>
#include <linux/seq_file.h>
#include <linux/rbtree.h>	/* Real rbtree implementation */
#include <linux/time.h>		/* For timespec64, time64_t */
#include <linux/build_bug.h>	/* For BUILD_BUG_ON */
#include <linux/bug.h>		/* For WARN_ON, WARN_ONCE */
#include <u-boot/crc.h>		/* For crc32() used by crc32_be */
#include "ext4_trace.h"		/* Trace event stubs */
#include "ext4_fscrypt.h"	/* fscrypt stubs */

/*
 * __CHAR_UNSIGNED__ - directory hash algorithm selection
 *
 * The ext4 filesystem uses different hash algorithms for directory indexing
 * depending on whether the platform's 'char' type is signed or unsigned.
 * GCC automatically defines __CHAR_UNSIGNED__ on platforms where char is
 * unsigned (e.g., ARM), and leaves it undefined where char is signed
 * (e.g., x86/sandbox).
 *
 * The filesystem stores EXT2_FLAGS_UNSIGNED_HASH or EXT2_FLAGS_SIGNED_HASH
 * in the superblock to record which hash variant was used when the filesystem
 * was created, ensuring correct behavior regardless of the mounting platform.
 *
 * See super.c:5123 and ioctl.c:1489 for the hash algorithm selection code.
 */

/*
 * Override no_printk to avoid format warnings in disabled debug prints.
 * The Linux kernel uses sector_t as u64, but U-Boot uses unsigned long.
 * This causes format mismatches with %llu that we want to ignore.
 */
#undef no_printk
#define no_printk(fmt, ...)	({ 0; })

/* rol32 and ror32 are now in linux/bitops.h */
/* Time types - timespec64 and time64_t are now in linux/time.h */

/*
 * ktime_t, sector_t are now in linux/types.h
 * atomic_t, atomic64_t are now in asm-generic/atomic.h
 * MAX_JIFFY_OFFSET is now in linux/jiffies.h
 * BDEVNAME_SIZE is now in linux/blkdev.h
 */
#include <asm-generic/atomic.h>
#include <linux/jiffies.h>
#include <linux/blkdev.h>

/* atomic_dec_if_positive, atomic_add_unless, etc. are now in asm-generic/atomic.h */

/* SMP stubs - U-Boot is single-threaded */
#define raw_smp_processor_id()	0

/* cmpxchg - compare and exchange, single-threaded version */
#define cmpxchg(ptr, old, new) ({		\
	typeof(*(ptr)) __cmpxchg_old = (old);	\
	typeof(*(ptr)) __cmpxchg_new = (new);	\
	typeof(*(ptr)) __cmpxchg_ret = *(ptr);	\
	if (__cmpxchg_ret == __cmpxchg_old)	\
		*(ptr) = __cmpxchg_new;		\
	__cmpxchg_ret;				\
})

/* Reference count type */
typedef struct { atomic_t refs; } refcount_t;

/* rwlock_t and read_lock/read_unlock are now in linux/spinlock.h */
#include <linux/spinlock.h>

/* RB tree types - from <linux/rbtree.h> included above */

/* percpu - use Linux headers */
#include <linux/percpu_counter.h>
#include <linux/percpu.h>

/* Project ID type */
typedef struct { unsigned int val; } kprojid_t;

#define make_kprojid(ns, id)	((kprojid_t){ .val = (id) })
#define from_kprojid(ns, kprojid)	((kprojid).val)
#define projid_eq(a, b)		((a).val == (b).val)

/* kobject - stub */
struct kobject {
	const char *name;
};

/* lockdep stubs - needed before jbd2.h is included */
struct lockdep_map { int dummy; };
struct lock_class_key { int dummy; };
#define rwsem_acquire(l, s, t, i)	do { } while (0)
#define rwsem_acquire_read(l, s, t, i)	do { } while (0)
#define rwsem_release(l, i)		do { } while (0)
#define _THIS_IP_			((unsigned long)0)

/* completion - use Linux header */
#include <linux/completion.h>

/* Cache alignment - stub */
#define ____cacheline_aligned_in_smp

/* Pointer check macros */
#define ZERO_OR_NULL_PTR(x)		((unsigned long)(x) <= PAGE_SIZE)
#define data_race(expr)			(expr)

/* Block I/O request flags - stubs */
#define REQ_META	0
#define REQ_PRIO	0
#define REQ_RAHEAD	0

/* GFP flags - stubs */
#define __GFP_MOVABLE	0
#define __GFP_FS	0

/* FIEMAP extent flags */
#define FIEMAP_EXTENT_LAST		0x00000001
#define FIEMAP_EXTENT_UNKNOWN		0x00000002
#define FIEMAP_EXTENT_DELALLOC		0x00000004
#define FIEMAP_EXTENT_UNWRITTEN		0x00000800
#define EXT4_FIEMAP_EXTENT_HOLE		0x08000000

/* FALLOC flags */
#define FALLOC_FL_KEEP_SIZE		0x01
#define FALLOC_FL_PUNCH_HOLE		0x02
#define FALLOC_FL_COLLAPSE_RANGE	0x08
#define FALLOC_FL_ZERO_RANGE		0x10
#define FALLOC_FL_INSERT_RANGE		0x20
#define FALLOC_FL_WRITE_ZEROES		0x40
#define FALLOC_FL_ALLOCATE_RANGE	0x80
#define FALLOC_FL_MODE_MASK		0xff

/* File flags */
#define O_SYNC		0

/* Forward declarations (struct inode, struct address_space) are in linux/fs.h */

/* Page types */
typedef unsigned long pgoff_t;
#ifndef PAGE_SHIFT
#define PAGE_SHIFT	12
#endif

/* File mode flags */
#define FMODE_32BITHASH		0x00000001
#define FMODE_64BITHASH		0x00000002

/* struct file is defined in linux/fs.h */

/* kiocb - kernel I/O control block */
struct iov_iter;

struct kiocb {
	int ki_flags;
	struct file *ki_filp;
	loff_t ki_pos;
};

#define IOCB_DIRECT		0x0001
#define IOCB_NOWAIT		0x0002
#define IOCB_ATOMIC		0x0004

/* iov_iter stubs */
#define iov_iter_truncate(i, count)	do { } while (0)
#define iov_iter_count(i)		0
#define iov_iter_alignment(iter)	0

/* __counted_by attribute - not available in U-Boot */
#define __counted_by(x)

/* dir_context for directory iteration */
struct dir_context;
typedef int (*filldir_t)(struct dir_context *, const char *, int, loff_t, u64, unsigned);

struct dir_context {
	filldir_t actor;
	loff_t pos;
};

/* iomap types and structs are in linux/iomap.h */

/* fiemap types */
#define FIEMAP_FLAG_SYNC	0x00000001
#define FIEMAP_FLAG_XATTR	0x00000002
#define FIEMAP_FLAG_CACHE	0x00000004

struct fiemap_extent_info {
	unsigned int fi_flags;
	unsigned int fi_extents_mapped;
	unsigned int fi_extents_max;
	void *fi_extents_start;
};

/* Capabilities - stubs (always allow) */
#define CAP_SYS_ADMIN		0
#define CAP_SYS_RESOURCE	0
#define capable(cap)		(1)

/* fscrypt_str, qstr are now in ext4_fscrypt.h */

/* percpu rw semaphore is in linux/percpu.h */

/* Memory allocation context - stubs */
static inline unsigned int memalloc_nofs_save(void) { return 0; }
static inline void memalloc_nofs_restore(unsigned int flags) { }

/* Inode flags - stubs */
#define IS_CASEFOLDED(inode)	(0)
/* IS_ENCRYPTED and FSCRYPT_SET_CONTEXT_MAX_SIZE are in ext4_fscrypt.h */
#define S_NOQUOTA		0

/* User namespace - stub */
struct user_namespace {
	int dummy;
};
extern struct user_namespace init_user_ns;

/*
 * BUG_ON / BUG - stubs (not using linux/bug.h which panics)
 * In Linux, these indicate kernel bugs. In ext4l, some BUG_ON conditions
 * that check for race conditions can trigger in single-threaded U-Boot,
 * so we stub them out as no-ops.
 */
#undef BUG_ON
#undef BUG
#define BUG_ON(cond)	do { (void)(cond); } while (0)
#define BUG()		do { } while (0)

/* might_sleep - stub */
#define might_sleep()	do { } while (0)

/* sb_rdonly - check if filesystem is mounted read-only */
#define sb_rdonly(sb)	((sb)->s_flags & SB_RDONLY)

/* Trace stubs are now in ext4_trace.h */

/* Buffer operations - wait_on_buffer, lock_buffer, unlock_buffer etc are in linux/buffer_head.h */
#define mark_buffer_dirty_inode(bh, i)	sync_dirty_buffer(bh)
#define mark_buffer_dirty(bh)		sync_dirty_buffer(bh)
struct buffer_head *sb_getblk(struct super_block *sb, sector_t block);
#define wait_on_bit_io(addr, bit, mode)	do { (void)(addr); (void)(bit); (void)(mode); } while (0)

/* inode_needs_sync - stub */
#define inode_needs_sync(inode)		(0)

/* Memory barriers - stubs for single-threaded */
#define smp_rmb()	do { } while (0)
#define smp_wmb()	do { } while (0)
#define smp_mb()	do { } while (0)

/*
 * set_bit/clear_bit are declared extern in asm/bitops.h but not implemented.
 * We implement them in interface.c for sandbox.
 */

/* Little-endian bit operations - use arch-provided find_next_zero_bit */
#define find_next_zero_bit_le(addr, size, offset) \
	find_next_zero_bit((void *)addr, size, offset)
#define __set_bit_le(nr, addr)		set_bit(nr, addr)
#define test_bit_le(nr, addr)		test_bit(nr, addr)
#define __test_and_clear_bit_le(nr, addr) \
	({ int __old = test_bit(nr, addr); clear_bit(nr, addr); __old; })
#define __test_and_set_bit_le(nr, addr) \
	({ int __old = test_bit(nr, addr); set_bit(nr, addr); __old; })

/* KUNIT stub */
#define KUNIT_STATIC_STUB_REDIRECT(...)	do { } while (0)

/* percpu_counter operations are in linux/percpu_counter.h */

/* Group permission - stub */
#define in_group_p(gid)			(0)

/* Quota operations - stubs */
#define dquot_alloc_block_nofail(inode, nr)	\
	({ (inode)->i_blocks += (nr) << ((inode)->i_blkbits - 9); 0; })
#define dquot_initialize(inode)			({ (void)(inode); 0; })
#define dquot_free_inode(inode)			do { (void)(inode); } while (0)
#define dquot_alloc_inode(inode)		({ (void)(inode); 0; })
#define dquot_drop(inode)			do { (void)(inode); } while (0)

/* icount - inode reference count */
#define icount_read(inode)			(1)

/* d_inode - get inode from dentry */
#define d_inode(dentry)				((dentry) ? (dentry)->d_inode : NULL)

/* Random number functions */
#define get_random_u32_below(max)		(0)

/* Buffer cache operations */
#define sb_find_get_block(sb, block)		((struct buffer_head *)NULL)
#define sync_dirty_buffer(bh)			submit_bh(REQ_OP_WRITE, bh)

/* Time functions - use boot-relative time for timestamps */
#define ktime_get_real_seconds()		(get_timer(0) / 1000)
#define time_before32(a, b)			(0)

/* Inode operations - iget_locked and new_inode are in interface.c */
extern struct inode *new_inode(struct super_block *sb);
#define i_uid_write(inode, uid)			do { } while (0)
#define i_gid_write(inode, gid)			do { } while (0)
#define inode_fsuid_set(inode, idmap)		do { } while (0)
#define inode_init_owner(idmap, i, dir, mode)	do { (i)->i_mode = (mode); } while (0)
#define insert_inode_locked(inode)		(0)
#define unlock_new_inode(inode)			do { } while (0)
#define clear_nlink(inode)			do { } while (0)
#define IS_DIRSYNC(inode)			({ (void)(inode); 0; })

/* fscrypt_prepare_new_inode, fscrypt_set_context are in ext4_fscrypt.h */

/* ext4_init_acl is provided by acl.h */
/* xattr stubs for files that don't include xattr.h */
struct super_block;
struct buffer_head;
struct qstr;

#ifdef CONFIG_EXT4_XATTR
int __ext4_xattr_set_credits(struct super_block *sb, struct inode *inode,
			     struct buffer_head *block_bh, size_t value_len,
			     bool is_create);
#endif
/* ext4_init_security is provided by xattr.h */

/* inode state stubs */
#define is_bad_inode(inode)			(0)

/* Block device operations - stubs */
#define sb_issue_zeroout(sb, blk, num, gfp)	({ (void)(sb); (void)(blk); (void)(num); (void)(gfp); 0; })
#define blkdev_issue_flush(bdev)		({ (void)(bdev); 0; })

/* Inode locking - stubs */
#define inode_is_locked(i)	(1)
#define i_size_write(i, s)	do { (i)->i_size = (s); } while (0)
#define i_size_read(i)		((i)->i_size)

/* spin_trylock is defined in linux/spinlock.h */

/* atomic_add_unless is now in asm-generic/atomic.h */

/* Block group lock - stub */
#define bgl_lock_ptr(lock, group)	NULL

/* RCU stubs */
#define rcu_read_lock()			do { } while (0)
#define rcu_read_unlock()		do { } while (0)
#define rcu_dereference(p)		(p)
#define rcu_dereference_protected(p, c)	(p)
#define rcu_assign_pointer(p, v)	((p) = (v))
#define call_rcu(head, func)		do { func(head); } while (0)
#define synchronize_rcu()		do { } while (0)

/* RCU head for callbacks - defined in linux/compat.h as callback_head */

/* lockdep stubs */
#define lockdep_is_held(lock)		(1)

/* Memory allocation - use linux/slab.h which is already available */
#include <linux/slab.h>

/* KMEM_CACHE macro - use kmem_cache_create */
#define KMEM_CACHE(s, flags)		kmem_cache_create(#s, sizeof(struct s), 0, flags, NULL)

/*
 * RB tree operations - use real rbtree implementation from lib/rbtree.c
 * and include/linux/rbtree.h. rb_entry, rb_first, rb_next, rb_prev,
 * rb_insert_color, rb_erase, rb_link_node, RB_EMPTY_ROOT, and
 * rbtree_postorder_for_each_entry_safe are all provided by the real
 * implementation - do not stub them!
 */

/* RCU barrier - stub */
#define rcu_barrier()		do { } while (0)

/* inode/dentry operations */
void iput(struct inode *inode);

/* current task - from linux/sched.h */
#include <linux/sched.h>

/* _RET_IP_ - return instruction pointer */
#define _RET_IP_	((unsigned long)__builtin_return_address(0))

/* SB_FREEZE constants */
#define SB_FREEZE_WRITE		1
#define SB_FREEZE_PAGEFAULT	2
#define SB_FREEZE_FS		3
#define SB_FREEZE_COMPLETE	4

/* sb_writers stub */
struct sb_writers {
	int frozen;
};

/* mapping_large_folio_support stub */
#define mapping_large_folio_support(m)	(0)

/* sector_t is now in linux/types.h */

/* Buffer head - from linux/buffer_head.h */
#include <linux/buffer_head.h>
#include <linux/jbd2.h>

/*
 * U-Boot buffer head private bits.
 *
 * Start at BH_JBDPrivateStart + 1 because ext4.h uses BH_JBDPrivateStart
 * for BH_BITMAP_UPTODATE.
 */
#define BH_OwnsData		(BH_JBDPrivateStart + 1)
BUFFER_FNS(OwnsData, ownsdata)

/*
 * U-Boot: marks buffer is in the buffer cache.
 * Cached buffers are freed by bh_cache_clear(), not brelse().
 */
#define BH_Cached		(BH_JBDPrivateStart + 2)
BUFFER_FNS(Cached, cached)

/* Forward declare for get_block_t */
struct inode;
struct buffer_head;

/* get_block_t - block mapping callback */
typedef int (get_block_t)(struct inode *inode, sector_t iblock,
			  struct buffer_head *bh_result, int create);

/* crc32c - from linux/crc32c.h */
#include <linux/crc32c.h>

/* ratelimit_state - stub */
struct ratelimit_state {
	int dummy;
};

/* fscrypt_dummy_policy and qstr are now in ext4_fscrypt.h */

/* errseq_t is defined in linux/fs.h */
/* time64_t is now in linux/time.h */

/* IS_NOQUOTA - stub */
#define IS_NOQUOTA(inode)	(0)

/* dentry - stub */
struct dentry {
	struct qstr d_name;
	struct inode *d_inode;
	struct super_block *d_sb;
	struct dentry *d_parent;
};

/* name_snapshot - for dentry name snapshots */
struct name_snapshot {
	struct qstr name;
};

/* vm_fault_t - stub */
typedef unsigned int vm_fault_t;

/* VM flags */
#define VM_SHARED		0x00000008
#define VM_WRITE		0x00000002
#define VM_HUGEPAGE		0x01000000
#define FAULT_FLAG_WRITE	0x01

/* pipe_inode_info - forward declaration */
struct pipe_inode_info;

/* vm_area_desc - for mmap_prepare */
struct vm_area_desc {
	struct file *file;
	unsigned long vm_flags;
	const struct vm_operations_struct *vm_ops;
};

/* Forward declarations for function prototypes */
struct kstat;
struct path;
struct vm_fault;
struct file_kattr;
struct dir_context;
struct readahead_control;
struct fiemap_extent_info;
struct folio;

/* qsize_t - quota size type */
typedef long long qsize_t;

/* blk_opf_t - block operation flags */
typedef unsigned int blk_opf_t;

/* Forward declare buffer_head for bh_end_io_t */
struct buffer_head;

/* bh_end_io_t - buffer head end io callback */
typedef void bh_end_io_t(struct buffer_head *bh, int uptodate);

/* Directory entry types */
#define DT_UNKNOWN	0
#define DT_FIFO		1
#define DT_CHR		2
#define DT_DIR		4
#define DT_BLK		6
#define DT_REG		8
#define DT_LNK		10
#define DT_SOCK		12
#define DT_WHT		14

/* mnt_idmap - stub */
struct mnt_idmap {
	int dummy;
};
extern struct mnt_idmap nop_mnt_idmap;

/* fstrim_range - stub */
struct fstrim_range {
	u64 start;
	u64 len;
	u64 minlen;
};

/* rw_semaphore - defined in linux/rwsem.h, include it */
#include <linux/rwsem.h>

/* block_device is defined in linux/fs.h */

/* Superblock flags */
#define SB_RDONLY		(1 << 0)
#define SB_I_VERSION		(1 << 26)	/* Update inode version */

/* UUID type */
typedef struct {
	__u8 b[16];
} uuid_t;

/* Forward declarations for super_block */
struct super_operations;
struct export_operations;
struct xattr_handler;

/* super_block - minimal stub */
struct super_block {
	void *s_fs_info;
	unsigned long s_blocksize;
	unsigned char s_blocksize_bits;
	unsigned long s_magic;
	loff_t s_maxbytes;
	unsigned long s_flags;
	unsigned long s_iflags;		/* Internal flags */
	struct rw_semaphore s_umount;
	struct sb_writers s_writers;
	struct block_device *s_bdev;
	char s_id[32];
	struct dentry *s_root;
	uuid_t s_uuid;
	struct file_system_type *s_type;
	s32 s_time_gran;		/* Time granularity (ns) */
	time64_t s_time_min;		/* Min supported time */
	time64_t s_time_max;		/* Max supported time */
	const struct super_operations *s_op;
	const struct export_operations *s_export_op;
	const struct xattr_handler * const *s_xattr;
	struct dentry *d_sb;		/* Parent dentry - stub */

	/* U-Boot: list of all inodes, for freeing on unmount */
	struct list_head s_inodes;
};

/* Block device read-only check */
static inline int bdev_read_only(struct block_device *bdev)
{
	return bdev ? bdev->read_only : 0;
}

/* kuid_t and kgid_t - from linux/cred.h */
#include <linux/cred.h>

/* Inode state bits */
#define I_NEW			(1 << 0)
#define I_FREEING		(1 << 1)
#define I_DIRTY_DATASYNC	(1 << 2)

/* Inode flags for i_flags */
#define S_SYNC		1
#define S_NOATIME	2
#define S_APPEND	4
#define S_IMMUTABLE	8
#define S_DAX		16
#define S_DIRSYNC	32
#define S_ENCRYPTED	64
#define S_CASEFOLD	128
#define S_VERITY	256

/* Permission mode constants */
#define S_IRWXUGO	(S_IRWXU | S_IRWXG | S_IRWXO)

/* Whiteout mode for overlayfs */
#define WHITEOUT_DEV	0
#define WHITEOUT_MODE	0

/* Rename flags */
#define RENAME_NOREPLACE	(1 << 0)
#define RENAME_EXCHANGE		(1 << 1)
#define RENAME_WHITEOUT		(1 << 2)

/* Inode dirty state flags */
#define I_DIRTY_TIME		(1 << 3)

/* Superblock flags */
#define SB_LAZYTIME		(1 << 25)

/* iattr valid flags */
#define ATTR_MODE		(1 << 0)
#define ATTR_UID		(1 << 1)
#define ATTR_GID		(1 << 2)
#define ATTR_SIZE		(1 << 3)
#define ATTR_ATIME		(1 << 4)
#define ATTR_MTIME		(1 << 5)
#define ATTR_CTIME		(1 << 6)
#define ATTR_ATIME_SET		(1 << 7)
#define ATTR_MTIME_SET		(1 << 8)
#define ATTR_FORCE		(1 << 9)
#define ATTR_KILL_SUID		(1 << 11)
#define ATTR_KILL_SGID		(1 << 12)
#define ATTR_TIMES_SET		((1 << 7) | (1 << 8))

/* STATX flags and attributes */
#define STATX_BTIME		0x00000800U
#define STATX_DIOALIGN		0x00002000U
#define STATX_WRITE_ATOMIC	0x00004000U
#define STATX_ATTR_COMPRESSED	0x00000004
#define STATX_ATTR_IMMUTABLE	0x00000010
#define STATX_ATTR_APPEND	0x00000020
#define STATX_ATTR_NODUMP	0x00000040
#define STATX_ATTR_ENCRYPTED	0x00000800
#define STATX_ATTR_VERITY	0x00100000

/* VM fault return values */
#define VM_FAULT_SIGBUS		0x0002
#define VM_FAULT_NOPAGE		0x0010
#define VM_FAULT_LOCKED		0x0200

/* struct path is defined in linux/fs.h */

/* struct kstat - stat buffer */
struct kstat {
	u64 ino;
	dev_t dev;
	umode_t mode;
	unsigned int nlink;
	uid_t uid;
	gid_t gid;
	dev_t rdev;
	loff_t size;
	struct timespec64 atime;
	struct timespec64 mtime;
	struct timespec64 ctime;
	struct timespec64 btime;
	u64 blocks;
	u32 blksize;
	u64 attributes;
	u64 attributes_mask;
	u32 result_mask;
	u32 dio_mem_align;
	u32 dio_offset_align;
	u32 atomic_write_unit_min;
	u32 atomic_write_unit_max;
	u32 atomic_write_segments_max;
};

/* struct vm_area_struct - virtual memory area */
struct vm_area_struct {
	unsigned long vm_start;
	unsigned long vm_end;
	struct file *vm_file;
	unsigned long vm_flags;
};

/* struct page - minimal stub */
struct page {
	unsigned long flags;
};

/* struct vm_fault - virtual memory fault info */
struct vm_fault {
	struct vm_area_struct *vma;
	unsigned long address;
	unsigned int flags;
	pgoff_t pgoff;
	struct folio *folio;
	struct page *page;
};

/* vm_operations_struct - virtual memory area operations */
struct vm_operations_struct {
	vm_fault_t (*fault)(struct vm_fault *vmf);
	vm_fault_t (*huge_fault)(struct vm_fault *vmf, unsigned int order);
	vm_fault_t (*page_mkwrite)(struct vm_fault *vmf);
	vm_fault_t (*pfn_mkwrite)(struct vm_fault *vmf);
	vm_fault_t (*map_pages)(struct vm_fault *vmf, pgoff_t start, pgoff_t end);
};

/* Forward declaration for swap */
struct swap_info_struct;

/* MAX_PAGECACHE_ORDER - maximum order for page cache allocations */
#define MAX_PAGECACHE_ORDER	12

/* Process flags */
#define PF_MEMALLOC		0x00000800

/* Forward declarations for inode operations */
struct inode_operations;
struct file_operations;

/* inode - extended for inode.c */
struct inode {
	struct super_block *i_sb;
	unsigned long i_ino;
	umode_t i_mode;
	unsigned int i_nlink;
	loff_t i_size;
	struct address_space *i_mapping;
	struct address_space i_data;
	kuid_t i_uid;
	kgid_t i_gid;
	unsigned long i_blocks;
	unsigned int i_generation;
	unsigned int i_flags;
	unsigned int i_blkbits;
	unsigned long i_state;
	struct timespec64 i_atime;
	struct timespec64 i_mtime;
	struct timespec64 i_ctime;
	struct list_head i_io_list;
	dev_t i_rdev;
	const struct inode_operations *i_op;
	const struct file_operations *i_fop;
	atomic_t i_writecount;		/* Count of writers */
	atomic_t i_count;		/* Reference count */
	struct rw_semaphore i_rwsem;	/* inode lock */
	const char *i_link;		/* Symlink target for fast symlinks */
	unsigned short i_write_hint;	/* Write life time hint */

	/* U-Boot: linkage into super_block s_inodes list */
	struct list_head i_sb_list;
};

/* Inode time accessors */
static inline struct timespec64 inode_get_atime(const struct inode *inode)
{
	return inode->i_atime;
}

static inline struct timespec64 inode_get_mtime(const struct inode *inode)
{
	return inode->i_mtime;
}

static inline struct timespec64 inode_get_ctime(const struct inode *inode)
{
	return inode->i_ctime;
}

static inline time_t inode_get_atime_sec(const struct inode *inode)
{
	return inode->i_atime.tv_sec;
}

static inline time_t inode_get_ctime_sec(const struct inode *inode)
{
	return inode->i_ctime.tv_sec;
}

static inline time_t inode_get_mtime_sec(const struct inode *inode)
{
	return inode->i_mtime.tv_sec;
}

static inline void inode_set_ctime(struct inode *inode, time_t sec, long nsec)
{
	inode->i_ctime.tv_sec = sec;
	inode->i_ctime.tv_nsec = nsec;
}

static inline void inode_set_atime(struct inode *inode, time_t sec, long nsec)
{
	inode->i_atime.tv_sec = sec;
	inode->i_atime.tv_nsec = nsec;
}

static inline void inode_set_mtime(struct inode *inode, time_t sec, long nsec)
{
	inode->i_mtime.tv_sec = sec;
	inode->i_mtime.tv_nsec = nsec;
}

static inline void simple_inode_init_ts(struct inode *inode)
{
	struct timespec64 ts = { .tv_sec = 0, .tv_nsec = 0 };

	inode->i_atime = ts;
	inode->i_mtime = ts;
	inode->i_ctime = ts;
}

/*
 * Inode state accessors - simplified for single-threaded U-Boot.
 * Linux uses READ_ONCE/WRITE_ONCE and lockdep assertions; we use direct access.
 */
static inline unsigned long inode_state_read_once(struct inode *inode)
{
	return inode->i_state;
}

static inline unsigned long inode_state_read(struct inode *inode)
{
	return inode->i_state;
}

static inline void inode_state_set_raw(struct inode *inode, unsigned long flags)
{
	inode->i_state |= flags;
}

static inline void inode_state_set(struct inode *inode, unsigned long flags)
{
	inode->i_state |= flags;
}

static inline void inode_state_clear_raw(struct inode *inode,
					 unsigned long flags)
{
	inode->i_state &= ~flags;
}

static inline void inode_state_clear(struct inode *inode, unsigned long flags)
{
	inode->i_state &= ~flags;
}

static inline void inode_state_assign_raw(struct inode *inode,
					  unsigned long flags)
{
	inode->i_state = flags;
}

static inline void inode_state_assign(struct inode *inode, unsigned long flags)
{
	inode->i_state = flags;
}

#define QSTR_INIT(n, l) { .name = (const unsigned char *)(n), .len = (l) }

/* dotdot_name for ".." lookups */
static const struct qstr dotdot_name = QSTR_INIT("..", 2);

/*
 * Hash info structure - defined in ext4.h.
 * Only defined here for files that don't include ext4.h (like hash.c)
 * This is wrapped in EXT4_UBOOT_NO_EXT4_H which hash.c defines.
 */
#ifdef EXT4_UBOOT_NO_EXT4_H
struct dx_hash_info {
	u32 hash;
	u32 minor_hash;
	int hash_version;
	u32 *seed;
};
#endif

/* Hash algorithm types */
#define DX_HASH_LEGACY			0
#define DX_HASH_HALF_MD4		1
#define DX_HASH_TEA			2
#define DX_HASH_LEGACY_UNSIGNED		3
#define DX_HASH_HALF_MD4_UNSIGNED	4
#define DX_HASH_TEA_UNSIGNED		5
#define DX_HASH_SIPHASH			6
#define DX_HASH_LAST			DX_HASH_SIPHASH

/* EOF markers for htree */
#define EXT4_HTREE_EOF_32BIT   ((1UL  << (32 - 1)) - 1)
#define EXT4_HTREE_EOF_64BIT   ((1ULL << (64 - 1)) - 1)

/* jbd2_buffer_trigger_type is defined in jbd2.h */

/* seq_file - forward declaration */
struct seq_file;

/* fscrypt_has_encryption_key, fscrypt_fname_siphash are in ext4_fscrypt.h */

/* ext4 warning macros - stubs (only when ext4.h is not included) */
#ifdef EXT4_UBOOT_NO_EXT4_H
#define ext4_warning(sb, fmt, ...) \
	do { } while (0)

#define ext4_warning_inode(inode, fmt, ...) \
	do { } while (0)
#endif

/* fallthrough annotation */
#ifndef fallthrough
#define fallthrough __attribute__((__fallthrough__))
#endif

/* BUILD_BUG_ON is in linux/build_bug.h */
/* WARN_ON, WARN_ON_ONCE, WARN_ONCE are in linux/bug.h */
#define pr_warn_once(fmt, ...) do { } while (0)

/* lockdep stubs */
#define lockdep_assert_held_read(l)	do { (void)(l); } while (0)

/* strtomem_pad - copy string to fixed-size buffer with padding */
#define strtomem_pad(dest, src, pad) do { \
	size_t _len = strlen(src); \
	if (_len >= sizeof(dest)) \
		_len = sizeof(dest); \
	memcpy(dest, src, _len); \
	if (_len < sizeof(dest)) \
		memset((char *)(dest) + _len, (pad), sizeof(dest) - _len); \
} while (0)

/* Memory weight - count set bits */
static inline unsigned long memweight(const void *ptr, size_t bytes)
{
	unsigned long ret = 0;
	const unsigned char *p = ptr;
	size_t i;

	for (i = 0; i < bytes; i++)
		ret += hweight8(p[i]);
	return ret;
}

/* BITS_PER_BYTE */
#ifndef BITS_PER_BYTE
#define BITS_PER_BYTE 8
#endif

/* extents.c stubs */

/* rwsem is_locked stub */
#define rwsem_is_locked(sem)		(1)

/* Buffer operations */
#define sb_getblk_gfp(sb, blk, gfp)	sb_getblk((sb), (blk))
#define bh_uptodate_or_lock(bh)		(1)
/* ext4_read_bh is stubbed in interface.c */

/* Inode locking */
#define inode_lock(inode)		do { } while (0)
#define inode_unlock(inode)		do { } while (0)
#define inode_lock_shared(inode)	do { } while (0)
#define inode_unlock_shared(inode)	do { } while (0)
#define inode_trylock(inode)		(1)
#define inode_trylock_shared(inode)	(1)
#define inode_dio_wait(inode)		do { } while (0)

/* Lock debugging - no-ops in U-Boot */
#define lockdep_assert_held_write(l)	do { } while (0)
#define lockdep_assert_held(l)		do { } while (0)

/* File operations */
#define file_modified(file)		({ (void)(file); 0; })
#define file_accessed(file)		do { (void)(file); } while (0)

/* Generic file operations - stubs for file.c */
#define generic_file_read_iter(iocb, to)	({ (void)(iocb); (void)(to); 0L; })
#define generic_write_checks(iocb, from)	({ (void)(iocb); (void)(from); 0L; })
#define generic_perform_write(iocb, from)	({ (void)(iocb); (void)(from); 0L; })
#define generic_write_sync(iocb, count)		({ (void)(iocb); (count); })
#define generic_atomic_write_valid(iocb, from)	({ (void)(iocb); (void)(from); 0; })
#define vfs_setpos(file, offset, maxsize)	({ (void)(file); (void)(maxsize); (offset); })

/* Security checks - no security in U-Boot */
#define IS_NOSEC(inode)			(1)

/* Filemap operations */
#define filemap_invalidate_lock(m)	do { } while (0)
#define filemap_invalidate_unlock(m)	do { } while (0)
#define filemap_invalidate_lock_shared(m) do { } while (0)
#define filemap_invalidate_unlock_shared(m) do { } while (0)
#define filemap_write_and_wait_range(m, s, e) ({ (void)(m); (void)(s); (void)(e); 0; })
#define filemap_fdatawrite_range(m, s, e) ({ (void)(m); (void)(s); (void)(e); 0; })
#define truncate_pagecache(i, s)	do { } while (0)
#define pagecache_isize_extended(i, f, t) do { } while (0)
#define invalidate_mapping_pages(m, s, e) do { (void)(m); (void)(s); (void)(e); } while (0)

/* Filemap fault handlers - stubs */
static inline vm_fault_t filemap_fault(struct vm_fault *vmf)
{
	return 0;
}

static inline vm_fault_t filemap_map_pages(struct vm_fault *vmf,
					   pgoff_t start, pgoff_t end)
{
	return 0;
}

/* DAX device mapping check - always false in U-Boot */
#define daxdev_mapping_supported(f, i, d) ({ (void)(f); (void)(i); (void)(d); 1; })

/* Inode time/size operations */
#define inode_newsize_ok(i, s)		({ (void)(i); (void)(s); 0; })
#define inode_set_ctime_current(i)	({ (void)(i); (struct timespec64){}; })
#define inode_set_mtime_to_ts(i, ts)	({ (void)(i); (ts); })
#define i_blocksize(i)			(1U << (i)->i_blkbits)

/* IS_SYNC macro */
#define IS_SYNC(inode)			(0)

/* Case-folding stubs - not supported in U-Boot */
#define sb_no_casefold_compat_fallback(sb)	({ (void)(sb); 1; })
#define generic_ci_validate_strict_name(d, n)	({ (void)(d); (void)(n); 1; })

/* in_range helper - check if value is in range [start, start+len) */
static inline int in_range(unsigned long val, unsigned long start,
			   unsigned long len)
{
	return val >= start && val < start + len;
}

/* Quota stub */
#define dquot_reclaim_block(i, n)	do { } while (0)

/* fiemap stubs */
#define fiemap_prep(i, fi, s, l, f)	({ (void)(i); (void)(fi); (void)(s); (void)(l); (void)(f); 0; })
#define fiemap_fill_next_extent(fi, l, p, sz, f) ({ (void)(fi); (void)(l); (void)(p); (void)(sz); (void)(f); 0; })
#define iomap_fiemap(i, fi, s, l, o)	({ (void)(i); (void)(fi); (void)(s); (void)(l); (void)(o); 0; })

/* Memory retry wait */
#define memalloc_retry_wait(g)		do { } while (0)

/* bdev operations */
#define bdev_write_zeroes_unmap_sectors(b) ({ (void)(b); 0; })

/* indirect.c stubs */

/* umin - unsigned min (Linux 6.x) */
#define umin(x, y)	((x) < (y) ? (x) : (y))

/* truncate_inode_pages - stub */
#define truncate_inode_pages(m, s)	do { } while (0)

/* ext4_sb_bread_nofail is stubbed in interface.c */

/* extents_status.c stubs */

/* shrinker - memory reclaim infrastructure (stub for U-Boot) */
struct shrink_control {
	gfp_t gfp_mask;
	int nid;
	unsigned long nr_to_scan;
	unsigned long nr_scanned;
};

struct shrinker {
	unsigned long (*count_objects)(struct shrinker *, struct shrink_control *);
	unsigned long (*scan_objects)(struct shrinker *, struct shrink_control *);
	void *private_data;
};

static inline struct shrinker *shrinker_alloc(unsigned int flags,
					      const char *fmt, ...)
{
	/* Return static dummy - U-Boot doesn't need memory reclamation */
	static struct shrinker dummy_shrinker;

	return &dummy_shrinker;
}

static inline void shrinker_register(struct shrinker *s)
{
}

static inline void shrinker_free(struct shrinker *s)
{
}

/* ktime functions */
static inline ktime_t ktime_get(void)
{
	return 0;
}

static inline s64 ktime_to_ns(ktime_t kt)
{
	return kt;
}

static inline ktime_t ktime_sub(ktime_t a, ktime_t b)
{
	return a - b;
}

static inline ktime_t ktime_add_ns(ktime_t kt, s64 ns)
{
	return kt + ns;
}

/* hrtimer stubs */
#define HRTIMER_MODE_ABS		0
#define schedule_hrtimeout(exp, mode)	({ (void)(exp); (void)(mode); 0; })

/* write lock variants */
#define write_trylock(lock)		({ (void)(lock); 1; })

/* percpu_counter_init/destroy are in linux/percpu_counter.h */

/* ratelimit macros */
#define DEFAULT_RATELIMIT_INTERVAL	(5 * 1000)
#define DEFAULT_RATELIMIT_BURST		10
#define DEFINE_RATELIMIT_STATE(name, interval, burst) \
	int name __attribute__((unused)) = 0
#define __ratelimit(state)		({ (void)(state); 1; })

/* seq_file tokens */
#define SEQ_START_TOKEN			((void *)1)

/* folio - memory page container stub */
struct folio {
	struct page *page;
	unsigned long index;
	struct address_space *mapping;
	unsigned long flags;
	void *data;
	struct buffer_head *private;
	int _refcount;
};

/* folio_batch - batch of folios */
struct folio_batch {
	unsigned int nr;
	struct folio *folios[16];
};

/* folio operations - stubs */
#define folio_mark_dirty(f)			do { (void)(f); } while (0)
/*
 * offset_in_folio - calculate offset of pointer within folio's data
 * In Linux this uses page alignment, but in U-Boot we use the folio's
 * actual data pointer since our buffers are malloc'd.
 */
#define offset_in_folio(f, p)			((f) ? (unsigned int)((uintptr_t)(p) - (uintptr_t)(f)->data) : 0U)
#define folio_buffers(f)			({ (void)(f); (struct buffer_head *)NULL; })
#define virt_to_folio(p)			({ (void)(p); (struct folio *)NULL; })
#define folio_set_bh(bh, f, off)		do { if ((bh) && (f)) { (bh)->b_folio = (f); (bh)->b_data = (char *)(f)->data + (off); } } while (0)
#define memcpy_from_folio(dst, f, off, len)	do { (void)(dst); (void)(f); (void)(off); (void)(len); } while (0)
#define folio_test_uptodate(f)			({ (void)(f); 1; })
#define folio_pos(f)				({ (void)(f); 0LL; })
#define folio_size(f)				({ (void)(f); PAGE_SIZE; })
#define folio_unlock(f)				do { (void)(f); } while (0)
/* folio_put and folio_get are implemented in support.c */
#define folio_lock(f)				do { (void)(f); } while (0)
#define folio_batch_init(fb)			do { (fb)->nr = 0; } while (0)
#define filemap_get_folios(m, i, e, fb)		({ (void)(m); (void)(i); (void)(e); (void)(fb); 0U; })

/* xa_mark_t - xarray mark type */
typedef unsigned int xa_mark_t;

/* Page cache tags */
#define PAGECACHE_TAG_DIRTY	0
#define PAGECACHE_TAG_TOWRITE	1
#define PAGECACHE_TAG_WRITEBACK	2

static inline xa_mark_t wbc_to_tag(struct writeback_control *wbc)
{
	if (wbc->sync_mode == WB_SYNC_ALL || wbc->tagged_writepages)
		return PAGECACHE_TAG_TOWRITE;
	return PAGECACHE_TAG_DIRTY;
}

/* blk_plug - block I/O plugging stub */
struct blk_plug {
	int dummy;
};
#define blk_start_plug(p)		do { (void)(p); } while (0)
#define blk_finish_plug(p)		do { (void)(p); } while (0)

/* Writeback reasons */
#define WB_REASON_FS_FREE_SPACE	0

/* readahead_control stub */
struct readahead_control {
	struct address_space *mapping;
	struct file *file;
	unsigned long _index;
	unsigned int _batch_count;
};

#define readahead_pos(rac)		({ (void)(rac); 0LL; })
#define readahead_length(rac)		({ (void)(rac); 0UL; })

/* address_space_operations is in linux/fs.h */

/* Stub for buffer_migrate_folio */
static inline int buffer_migrate_folio(struct address_space *mapping,
				       struct folio *dst, struct folio *src, int mode)
{
	return -EOPNOTSUPP;
}

/* Stub for buffer_migrate_folio_norefs */
static inline int buffer_migrate_folio_norefs(struct address_space *mapping,
					      struct folio *dst, struct folio *src, int mode)
{
	return -EOPNOTSUPP;
}

/* Stub for noop_dirty_folio */
static inline bool noop_dirty_folio(struct address_space *mapping,
				    struct folio *folio)
{
	return false;
}

/* Stub implementations for address_space_operations callbacks */
static inline bool block_is_partially_uptodate(struct folio *folio,
					       size_t from, size_t count)
{
	return false;
}

static inline int generic_error_remove_folio(struct address_space *mapping,
					     struct folio *folio)
{
	return 0;
}

/* FGP flags for folio_grab_cache */
#define FGP_ACCESSED	0x00000001
#define FGP_LOCK	0x00000002
#define FGP_CREAT	0x00000004
#define FGP_WRITE	0x00000008
#define FGP_NOFS	0x00000010
#define FGP_NOWAIT	0x00000020
#define FGP_FOR_MMAP	0x00000040
#define FGP_STABLE	0x00000080
#define FGP_WRITEBEGIN	(FGP_LOCK | FGP_WRITE | FGP_CREAT | FGP_STABLE)

/* kmap/kunmap stubs for inline.c */
#define kmap_local_folio(folio, off)	((folio) ? (char *)(folio)->data + (off) : NULL)
#define kunmap_local(addr)		do { (void)(addr); } while (0)

/* Folio zeroing stubs for inline.c */
#define folio_zero_tail(f, off, kaddr)	({ (void)(f); (void)(off); (void)(kaddr); (void *)NULL; })
#define folio_zero_segment(f, s, e)	do { (void)(f); (void)(s); (void)(e); } while (0)

/* mapping_gfp_mask stub */
#define mapping_gfp_mask(m)		({ (void)(m); GFP_KERNEL; })

/* Folio operations - implemented in support.c */
struct folio *__filemap_get_folio(struct address_space *mapping,
				  pgoff_t index, unsigned int fgp_flags,
				  gfp_t gfp);
void folio_put(struct folio *folio);
void folio_get(struct folio *folio);
void mapping_clear_folio_cache(struct address_space *mapping);

/* projid_t - project ID type */
typedef unsigned int projid_t;

/*
 * Additional stubs for inode.c
 */

/* try_cmpxchg - compare and exchange with return value */
#define try_cmpxchg(ptr, old, new) ({		\
	typeof(*(old)) __old = *(old);		\
	typeof(*(ptr)) __ret = cmpxchg(ptr, __old, (new));	\
	if (__ret != __old)			\
		*(old) = __ret;			\
	__ret == __old;				\
})

/* hash_64 - simple 64-bit hash */
#define hash_64(val, bits)	((unsigned long)((val) >> (64 - (bits))))

/* Dentry operations - stubs */
#define d_find_any_alias(i)			({ (void)(i); (struct dentry *)NULL; })
#define dget_parent(d)				({ (void)(d); (struct dentry *)NULL; })
#define dput(d)					do { (void)(d); } while (0)
#define d_splice_alias(i, d)			({ (d)->d_inode = (i); (d); })
#define d_obtain_alias(i)			({ (void)(i); (struct dentry *)NULL; })
#define d_instantiate_new(d, i)			((void)((d)->d_inode = (i)))
#define d_instantiate(d, i)			((void)((d)->d_inode = (i)))
#define d_tmpfile(f, i)				do { (void)(f); (void)(i); } while (0)
#define d_invalidate(d)				do { (void)(d); } while (0)
#define finish_open_simple(f, e)		(e)
#define ihold(i)				do { (void)(i); } while (0)

/* Sync operations - stubs */
#define sync_mapping_buffers(m)			({ (void)(m); 0; })
#define sync_inode_metadata(i, w)		({ (void)(i); (void)(w); 0; })
#define generic_buffers_fsync_noflush(f, s, e, d) ({ (void)(f); (void)(s); (void)(e); (void)(d); 0; })
#define file_write_and_wait_range(f, s, e)	({ (void)(f); (void)(s); (void)(e); 0; })
#define file_check_and_advance_wb_err(f)	({ (void)(f); 0; })

/* DAX stubs - DAX not supported in U-Boot */
#define IS_DAX(inode)				(0)
#define dax_break_layout_final(inode)		do { } while (0)
#define dax_writeback_mapping_range(m, bd, wb)	({ (void)(m); (void)(bd); (void)(wb); 0; })
#define dax_zero_range(i, p, l, d, op)		({ (void)(i); (void)(p); (void)(l); (void)(d); (void)(op); -EOPNOTSUPP; })
#define dax_break_layout_inode(i, m)		({ (void)(i); (void)(m); 0; })

/* Superblock freezing stubs */
#define sb_start_intwrite(sb)			do { (void)(sb); } while (0)
#define sb_end_intwrite(sb)			do { (void)(sb); } while (0)
#define sb_start_intwrite_trylock(sb)		({ (void)(sb); 1; })
#define sb_start_pagefault(sb)			do { (void)(sb); } while (0)
#define sb_end_pagefault(sb)			do { (void)(sb); } while (0)

/* d_path - get pathname - stub returns empty path */
static inline char *d_path(const struct path *path, char *buf, int buflen)
{
	if (buflen > 0)
		buf[0] = '\0';
	return buf;
}

/* fscrypt_file_open is in ext4_fscrypt.h */
#define fsverity_file_open(i, f)		({ (void)(i); (void)(f); 0; })

/* Quota file open - stub */
#define dquot_file_open(i, f)			({ (void)(i); (void)(f); 0; })

/* Inode I/O list management */
#define inode_io_list_del(inode)		do { } while (0)
#define inode_is_open_for_write(i)		(0)
#define inode_is_dirtytime_only(i)		(0)

/* Writeback stubs for super.c */
#define writeback_iter(mapping, wbc, folio, error) \
	({ (void)(mapping); (void)(wbc); (void)(error); (struct folio *)NULL; })
#define folio_redirty_for_writepage(wbc, folio) \
	({ (void)(wbc); (void)(folio); false; })

/* Folio operations - additional stubs */
#define folio_zero_segments(f, s1, e1, s2, e2)	do { } while (0)
#define folio_zero_new_buffers(f, f2, t)	do { } while (0)
#define folio_wait_stable(f)			do { } while (0)
#define folio_zero_range(f, s, l)		do { } while (0)
#define folio_mark_uptodate(f)			do { } while (0)
#define folio_next_index(f)			((f)->index + 1)
#define folio_next_pos(f)			((loff_t)folio_next_index(f) << PAGE_SHIFT)
#define folio_mapped(f)				(0)

/*
 * fgf_set_order - Set the order (size) for folio allocation
 * U-Boot doesn't support large folios, so this is a no-op stub.
 */
#define fgf_set_order(size)			(0)
#define folio_clear_dirty_for_io(f)		({ (void)(f); 1; })
#define folio_clear_uptodate(f)			do { } while (0)
#define folio_batch_release(fb)			do { } while (0)
#define folio_nr_pages(f)			(1UL)
#define folio_contains(f, idx)			({ (void)(f); (void)(idx); 1; })
#define folio_clear_checked(f)			do { } while (0)
#define folio_test_dirty(f)			(0)
#define folio_test_writeback(f)			(0)
#define folio_wait_writeback(f)			do { } while (0)
#define folio_clear_dirty(f)			do { } while (0)
#define folio_test_checked(f)			(0)
#define folio_maybe_dma_pinned(f)		(0)
#define folio_set_checked(f)			do { } while (0)
#define folio_test_locked(f)			(0)
#define folio_mkclean(f)			(0)
#define page_folio(page)			((struct folio *)(page))

/* Quota stubs - additional */
#define dquot_claim_block(i, n)			({ (void)(i); (void)(n); 0; })
#define dquot_reserve_block(i, n)		({ (void)(i); (void)(n); 0; })
#define dquot_release_reservation_block(i, n)	do { } while (0)
#define dquot_initialize_needed(i)		(0)
#define dquot_transfer(m, i, a)			({ (void)(m); (void)(i); (void)(a); 0; })
#define is_quota_modification(m, i, a)		({ (void)(m); (void)(i); (void)(a); 0; })

/* percpu_counter_sub is in linux/percpu_counter.h */

/* Filemap operations - additional */
#define filemap_get_folio(m, i)			((struct folio *)NULL)
#define filemap_get_folios_tag(m, s, e, t, fb)	({ (void)(m); (void)(s); (void)(e); (void)(t); (void)(fb); 0U; })
#define filemap_flush(m)			({ (void)(m); 0; })
#define filemap_write_and_wait(m)		({ (void)(m); 0; })
#define filemap_dirty_folio(m, f)		({ (void)(m); (void)(f); false; })
#define filemap_lock_folio(m, i)		((struct folio *)NULL)
/* filemap_invalidate_lock_shared defined earlier */
#define mapping_tagged(m, t)			(0)
#define tag_pages_for_writeback(m, s, e)	do { } while (0)
#define try_to_writeback_inodes_sb(sb, r)	do { } while (0)
#define mapping_gfp_constraint(m, g)		(g)
#define mapping_set_folio_order_range(m, l, h)	do { } while (0)
#define filemap_splice_read(i, p, pi, l, f)	({ (void)(i); (void)(p); (void)(pi); (void)(l); (void)(f); 0L; })

/* Buffer operations - additional */
#define getblk_unmovable(bdev, block, size)	sb_getblk(bdev->bd_super, block)
#define create_empty_buffers(f, s, flags)	({ (void)(f); (void)(s); (void)(flags); (struct buffer_head *)NULL; })
/* bh_offset returns offset of b_data within the folio */
#define bh_offset(bh)				((bh)->b_folio ? \
	(unsigned long)((char *)(bh)->b_data - (char *)(bh)->b_folio->data) : 0UL)
#define block_invalidate_folio(f, o, l)		do { } while (0)
#define block_write_end(pos, len, copied, folio) ({ (void)(pos); (void)(len); (void)(folio); (copied); })
#define block_dirty_folio(m, f)			({ (void)(m); (void)(f); false; })
#define try_to_free_buffers(f)			({ (void)(f); true; })
#define block_commit_write(f, f2, t)		do { } while (0)
#define block_page_mkwrite(v, f, g)		((vm_fault_t)0)
#define map_bh(bh, sb, block)			do { } while (0)
#define write_begin_get_folio(iocb, m, idx, l)	({ (void)(iocb); (void)(m); (void)(idx); (void)(l); (struct folio *)NULL; })

/* fscrypt_name, fscrypt_match_name, and fscrypt stubs are in ext4_fscrypt.h */

/* fsverity stubs */
#define fsverity_prepare_setattr(d, a)		({ (void)(d); (void)(a); 0; })
#define fsverity_active(i)			(0)

/* Inode time setters - needed for ext4.h */
static inline struct timespec64 inode_set_atime_to_ts(struct inode *inode,
						      struct timespec64 ts)
{
	inode->i_atime = ts;
	return ts;
}

static inline struct timespec64 inode_set_ctime_to_ts(struct inode *inode,
						      struct timespec64 ts)
{
	inode->i_ctime = ts;
	return ts;
}

/* Inode version operations */
#define inode_peek_iversion_raw(i)		(0ULL)
#define inode_peek_iversion(i)			(0ULL)
#define inode_set_flags(i, f, m)		do { } while (0)
#define inode_set_iversion_raw(i, v)		do { } while (0)
#define inode_set_iversion_queried(i, v)	do { } while (0)
#define inode_inc_iversion(i)			do { } while (0)

/* Inode credential helpers */
static inline unsigned int i_uid_read(const struct inode *inode)
{
	return inode->i_uid.val;
}

static inline unsigned int i_gid_read(const struct inode *inode)
{
	return inode->i_gid.val;
}

#define i_uid_needs_update(m, a, i)		({ (void)(m); (void)(a); (void)(i); 0; })
#define i_gid_needs_update(m, a, i)		({ (void)(m); (void)(a); (void)(i); 0; })
#define i_uid_update(m, a, i)			do { } while (0)
#define i_gid_update(m, a, i)			do { } while (0)

/* Device encoding helpers */
#ifndef MINORBITS
#define MINORBITS	20
#endif
#ifndef MINORMASK
#define MINORMASK	((1U << MINORBITS) - 1)
#endif
#ifndef MAJOR
#define MAJOR(dev)	((unsigned int)((dev) >> MINORBITS))
#endif
#ifndef MINOR
#define MINOR(dev)	((unsigned int)((dev) & MINORMASK))
#endif
#ifndef MKDEV
#define MKDEV(ma, mi)	(((ma) << MINORBITS) | (mi))
#endif

#define old_valid_dev(dev)	(MAJOR(dev) < 256 && MINOR(dev) < 256)
#define old_encode_dev(dev)	((MAJOR(dev) << 8) | MINOR(dev))
#define old_decode_dev(dev)	MKDEV((dev) >> 8, (dev) & 0xff)
#define new_encode_dev(dev)	((unsigned int)(dev))
#define new_decode_dev(dev)	((dev_t)(dev))

/* UID/GID bit helpers */
#define low_16_bits(x)		((x) & 0xFFFF)
#define high_16_bits(x)		(((x) >> 16) & 0xFFFF)
#define fs_high2lowuid(uid)	((uid) & 0xFFFF)
#define fs_high2lowgid(gid)	((gid) & 0xFFFF)

/* Inode allocation/state operations */
extern struct inode *iget_locked(struct super_block *sb, unsigned long ino);
#define set_nlink(i, n)			do { (i)->i_nlink = (n); } while (0)
#define inc_nlink(i)			do { (i)->i_nlink++; } while (0)
#define drop_nlink(i)			do { (i)->i_nlink--; } while (0)
#define inode_set_cached_link(i, l, len) do { } while (0)
#define init_special_inode(i, m, d)	do { } while (0)
#define make_bad_inode(i)		do { } while (0)
#define iget_failed(i)			do { } while (0)
#define find_inode_by_ino_rcu(sb, ino)	((struct inode *)NULL)
#define mark_inode_dirty(i)		do { } while (0)

/* Attribute operations */
#define setattr_prepare(m, d, a)	({ (void)(m); (void)(d); (void)(a); 0; })
#define setattr_copy(m, i, a)		do { } while (0)
#define posix_acl_chmod(m, i, mo)	({ (void)(m); (void)(i); (void)(mo); 0; })
#define generic_fillattr(m, req, i, s)	do { } while (0)
#define generic_fill_statx_atomic_writes(s, u_m, u_M, g) do { } while (0)

/* Inode flag macros */
#define IS_APPEND(inode)	((inode)->i_flags & S_APPEND)
#define IS_IMMUTABLE(inode)	((inode)->i_flags & S_IMMUTABLE)

/* File operations */
#define file_update_time(f)		do { } while (0)
#define vmf_fs_error(e)			((vm_fault_t)VM_FAULT_SIGBUS)

/* iomap stubs */
#define iomap_bmap(m, b, o)		({ (void)(m); (void)(b); (void)(o); 0UL; })
#define iomap_swapfile_activate(s, f, sp, o) ({ (void)(s); (void)(f); (void)(sp); (void)(o); -EOPNOTSUPP; })

/* Block device alignment */
#define bdev_dma_alignment(bd)		(0)

/* Truncation */
#define truncate_inode_pages_final(m)	do { } while (0)
#define truncate_pagecache_range(i, s, e) do { } while (0)

/*
 * Additional stubs for dir.c
 */

/* FSTR_INIT - fscrypt_str initializer (fscrypt_str defined in ext4_fscrypt.h) */
#define FSTR_INIT(n, l)		{ .name = (n), .len = (l) }

/* fscrypt directory operations are in ext4_fscrypt.h */

/* Readahead operations */
#define ra_has_index(ra, idx)			({ (void)(ra); (void)(idx); 0; })
#define page_cache_sync_readahead(m, ra, f, i, n) do { } while (0)

/* Inode version operations */
#define inode_eq_iversion(i, v)			({ (void)(i); (void)(v); 1; })
#define inode_query_iversion(i)			({ (void)(i); 0ULL; })

/* Directory context operations - call the actor callback */
static inline bool dir_emit(struct dir_context *ctx, const char *name, int len,
			    u64 ino, unsigned int type)
{
	return ctx->actor(ctx, name, len, ctx->pos, ino, type) == 0;
}
#define dir_relax_shared(i)			({ (void)(i); 1; })

/* File llseek */
#define generic_file_llseek_size(f, o, w, m, e)	({ (void)(f); (void)(o); (void)(w); (void)(m); (void)(e); 0LL; })

/* generic_read_dir - stub function (needs to be a real function for struct init) */
ssize_t generic_read_dir(struct file *f, char __user *buf, size_t count,
			 loff_t *ppos);

/* struct_size helper */
#define struct_size(p, member, count)		(sizeof(*(p)) + sizeof((p)->member[0]) * (count))

/* file_operations - extended for dir.c */
struct file_operations {
	int (*open)(struct inode *, struct file *);
	loff_t (*llseek)(struct file *, loff_t, int);
	ssize_t (*read)(struct file *, char *, size_t, loff_t *);
	int (*iterate_shared)(struct file *, struct dir_context *);
	long (*unlocked_ioctl)(struct file *, unsigned int, unsigned long);
	int (*fsync)(struct file *, loff_t, loff_t, int);
	int (*release)(struct inode *, struct file *);
};

/* delayed_call - for delayed freeing of symlink data */
typedef void (*delayed_call_func_t)(const void *);
struct delayed_call {
	delayed_call_func_t fn;
	const void *arg;
};

#define set_delayed_call(dc, func, data) do { \
	(dc)->fn = (func); \
	(dc)->arg = (data); \
} while (0)

#define kfree_link		kfree

/* nd_terminate_link - terminate symlink string */
static inline void nd_terminate_link(void *name, loff_t len, int maxlen)
{
	((char *)name)[min_t(loff_t, len, maxlen)] = '\0';
}

/* inode_operations - for file and directory operations */
struct inode_operations {
	/* Symlink operations */
	const char *(*get_link)(struct dentry *, struct inode *,
				struct delayed_call *);
	/* Common operations */
	int (*getattr)(struct mnt_idmap *, const struct path *,
		       struct kstat *, u32, unsigned int);
	ssize_t (*listxattr)(struct dentry *, char *, size_t);
	int (*fiemap)(struct inode *, struct fiemap_extent_info *, u64, u64);
	int (*setattr)(struct mnt_idmap *, struct dentry *, struct iattr *);
	struct posix_acl *(*get_inode_acl)(struct inode *, int, bool);
	int (*set_acl)(struct mnt_idmap *, struct dentry *,
		       struct posix_acl *, int);
	int (*fileattr_get)(struct dentry *, struct file_kattr *);
	int (*fileattr_set)(struct mnt_idmap *, struct dentry *,
			    struct file_kattr *);
	/* Directory operations */
	struct dentry *(*lookup)(struct inode *, struct dentry *, unsigned int);
	int (*create)(struct mnt_idmap *, struct inode *, struct dentry *,
		      umode_t, bool);
	int (*link)(struct dentry *, struct inode *, struct dentry *);
	int (*unlink)(struct inode *, struct dentry *);
	int (*symlink)(struct mnt_idmap *, struct inode *, struct dentry *,
		       const char *);
	struct dentry *(*mkdir)(struct mnt_idmap *, struct inode *,
				struct dentry *, umode_t);
	int (*rmdir)(struct inode *, struct dentry *);
	int (*mknod)(struct mnt_idmap *, struct inode *, struct dentry *,
		     umode_t, dev_t);
	int (*rename)(struct mnt_idmap *, struct inode *, struct dentry *,
		      struct inode *, struct dentry *, unsigned int);
	int (*tmpfile)(struct mnt_idmap *, struct inode *, struct file *,
		       umode_t);
};

/* file open helper */
#define simple_open(i, f)		({ (void)(i); (void)(f); 0; })

/* simple_get_link - for fast symlinks stored in inode */
static inline const char *simple_get_link(struct dentry *dentry,
					  struct inode *inode,
					  struct delayed_call *callback)
{
	return inode->i_link;
}

/* fscrypt symlink stubs are in ext4_fscrypt.h */

/*
 * Additional stubs for super.c
 */

/* fs_context and fs_parser stubs */
struct constant_table {
	const char *name;
	int value;
};

struct fs_parameter_spec {
	const char *name;
	int opt;
	unsigned short type;
	const struct constant_table *data;
};

/* fs_parameter spec types */
#define fs_param_is_flag	0
#define fs_param_is_u32		1
#define fs_param_is_s32		2
#define fs_param_is_u64		3
#define fs_param_is_enum	4
#define fs_param_is_string	5
#define fs_param_is_blob	6
#define fs_param_is_fd		7
#define fs_param_is_uid		8
#define fs_param_is_gid		9
#define fs_param_is_blockdev	10

/* fsparam_* macros for mount option parsing - use literal values */
#define fsparam_flag(name, opt) \
	{(name), (opt), 0, NULL}
#define fsparam_u32(name, opt) \
	{(name), (opt), 1, NULL}
#define fsparam_s32(name, opt) \
	{(name), (opt), 2, NULL}
#define fsparam_u64(name, opt) \
	{(name), (opt), 3, NULL}
#define fsparam_string(name, opt) \
	{(name), (opt), 5, NULL}
#define fsparam_string_empty(name, opt) \
	{(name), (opt), 5, NULL}
#define fsparam_enum(name, opt, array) \
	{(name), (opt), 4, (array)}
#define fsparam_bdev(name, opt) \
	{(name), (opt), 10, NULL}
#define fsparam_uid(name, opt) \
	{(name), (opt), 8, NULL}
#define fsparam_gid(name, opt) \
	{(name), (opt), 9, NULL}
#define __fsparam(type, name, opt, flags, data) \
	{(name), (opt), (type), (data)}

/* Quota format constants */
#define QFMT_VFS_OLD		1
#define QFMT_VFS_V0		2
#define QFMT_VFS_V1		4

struct fs_context;
struct fs_parameter;

struct fs_context_operations {
	int (*parse_param)(struct fs_context *, struct fs_parameter *);
	int (*get_tree)(struct fs_context *);
	int (*reconfigure)(struct fs_context *);
	void (*free)(struct fs_context *);
};

struct file_system_type {
	struct module *owner;
	const char *name;
	int (*init_fs_context)(struct fs_context *);
	const struct fs_parameter_spec *parameters;
	void (*kill_sb)(struct super_block *);
	int fs_flags;
	struct list_head fs_supers;
};

#define FS_REQUIRES_DEV		1
#define FS_BINARY_MOUNTDATA	2
#define FS_HAS_SUBTYPE		4
#define FS_USERNS_MOUNT		8
#define FS_DISALLOW_NOTIFY_PERM	16
#define FS_ALLOW_IDMAP		32

/* Buffer read sync */
static inline void end_buffer_read_sync(struct buffer_head *bh, int uptodate)
{
	if (uptodate)
		set_buffer_uptodate(bh);
	else
		clear_buffer_uptodate(bh);
	unlock_buffer(bh);
}
#define REQ_OP_READ		0

/* Superblock flags */
#define SB_ACTIVE		(1 << 30)

/* Part stat - not used in U-Boot. Note: sectors[X] is passed as second arg */
#define STAT_WRITE		0
#define STAT_READ		0
static u64 __attribute__((unused)) __ext4_sectors[2];
#define sectors			__ext4_sectors
#define part_stat_read(p, f)	({ (void)(p); (void)(f); 0ULL; })

/* System state - U-Boot is always running */
#define system_state		0
#define SYSTEM_HALT		1
#define SYSTEM_POWER_OFF	2
#define SYSTEM_RESTART		3

/* Hex dump */
#define DUMP_PREFIX_ADDRESS	0
#define print_hex_dump(l, p, pt, rg, gc, b, len, a) do { } while (0)

/* Slab flags */
#define SLAB_RECLAIM_ACCOUNT	0
#define SLAB_ACCOUNT		0

/* Forward declarations for super_operations and export_operations */
struct kstatfs;
struct fid;

/* super_operations - for VFS */
struct super_operations {
	struct inode *(*alloc_inode)(struct super_block *);
	void (*free_inode)(struct inode *);
	void (*destroy_inode)(struct inode *);
	int (*write_inode)(struct inode *, struct writeback_control *);
	void (*dirty_inode)(struct inode *, int);
	int (*drop_inode)(struct inode *);
	void (*evict_inode)(struct inode *);
	void (*put_super)(struct super_block *);
	int (*sync_fs)(struct super_block *, int);
	int (*freeze_fs)(struct super_block *);
	int (*unfreeze_fs)(struct super_block *);
	int (*statfs)(struct dentry *, struct kstatfs *);
	int (*show_options)(struct seq_file *, struct dentry *);
	void (*shutdown)(struct super_block *);
	ssize_t (*quota_read)(struct super_block *, int, char *, size_t, loff_t);
	ssize_t (*quota_write)(struct super_block *, int, const char *, size_t, loff_t);
	struct dentry *(*get_dquots)(struct inode *);
};

/* export_operations for NFS */
struct export_operations {
	int (*encode_fh)(struct inode *, __u32 *, int *, struct inode *);
	struct dentry *(*fh_to_dentry)(struct super_block *, struct fid *, int, int);
	struct dentry *(*fh_to_parent)(struct super_block *, struct fid *, int, int);
	struct dentry *(*get_parent)(struct dentry *);
	int (*commit_metadata)(struct inode *);
};

/* Generic file handle encoder for NFS exports - stub */
static inline int generic_encode_ino32_fh(struct inode *inode, __u32 *fh,
					  int *max_len, struct inode *parent)
{
	return 0;
}

/* fid for export_operations */
struct fid {
	union {
		struct {
			u32 ino;
			u32 gen;
			u32 parent_ino;
			u32 parent_gen;
		} i32;
		__u32 raw[0];
	};
};

/* uuid_to_fsid - convert UUID to fsid */
static inline __kernel_fsid_t uuid_to_fsid(const u8 *uuid)
{
	__kernel_fsid_t fsid;

	fsid.val[0] = (uuid[0] << 24) | (uuid[1] << 16) |
		      (uuid[2] << 8) | uuid[3];
	fsid.val[1] = (uuid[4] << 24) | (uuid[5] << 16) |
		      (uuid[6] << 8) | uuid[7];
	return fsid;
}

/* kstatfs for statfs */
struct kstatfs {
	long f_type;
	long f_bsize;
	u64 f_blocks;
	u64 f_bfree;
	u64 f_bavail;
	u64 f_files;
	u64 f_ffree;
	__kernel_fsid_t f_fsid;
	long f_namelen;
	long f_frsize;
	long f_flags;
	long f_spare[4];
};

/* seq_file stubs */
struct seq_file;

/* Module stubs */
struct module;
#ifndef THIS_MODULE
#define THIS_MODULE			NULL
#endif
#define MODULE_ALIAS_FS(name)

/* register/unregister filesystem */
#define register_filesystem(fs)		({ (void)(fs); 0; })
#define unregister_filesystem(fs)	({ (void)(fs); 0; })

/* EXT4_GOING flags */
#define EXT4_GOING_FLAGS_DEFAULT	0
#define EXT4_GOING_FLAGS_LOGFLUSH	1
#define EXT4_GOING_FLAGS_NOLOGFLUSH	2

/* fs_context stubs */
/* fs_context_purpose - what the context is for */
enum fs_context_purpose {
	FS_CONTEXT_FOR_MOUNT,
	FS_CONTEXT_FOR_SUBMOUNT,
	FS_CONTEXT_FOR_RECONFIGURE,
};

struct fs_context {
	const struct fs_context_operations *ops;
	struct file_system_type *fs_type;
	void *fs_private;
	struct dentry *root;
	struct user_namespace *user_ns;
	void *s_fs_info;		/* Filesystem specific info */
	unsigned int sb_flags;
	unsigned int sb_flags_mask;
	unsigned int lsm_flags;
	enum fs_context_purpose purpose;
	bool sloppy;
	bool silent;
};

/* ext4 superblock initialisation and commit */
int ext4_fill_super(struct super_block *sb, struct fs_context *fc);
int ext4_commit_super(struct super_block *sb);
void ext4_unregister_li_request(struct super_block *sb);

/* fs_parameter stubs */
struct fs_parameter {
	const char *key;
	int type;
	size_t size;
	int dirfd;
	union {
		char *string;
		int boolean;
		int integer;
	};
};

/* fs_value types - result type from parsing */
enum fs_value_type {
	fs_value_is_undefined,
	fs_value_is_flag,
	fs_value_is_string,
	fs_value_is_blob,
	fs_value_is_filename,
	fs_value_is_file,
};

/* fs_parse_result - result of parsing a parameter */
struct fs_parse_result {
	bool negated;
	union {
		bool boolean;
		int int_32;
		unsigned int uint_32;
		u64 uint_64;
		kuid_t uid;
		kgid_t gid;
	};
};

/* fs_parse stubs */
#define fs_parse(fc, desc, param, result) ({ (void)(fc); (void)(desc); (void)(param); (void)(result); -ENOPARAM; })
#define ENOPARAM			519
#define fs_lookup_param(fc, p, bdev, fl, path) ({ (void)(fc); (void)(p); (void)(bdev); (void)(fl); (void)(path); -EINVAL; })

/* get_tree helpers */
#define get_tree_bdev(fc, fill_super)	({ (void)(fc); (void)(fill_super); -ENODEV; })
#define get_tree_nodev(fc, fill_super)	({ (void)(fc); (void)(fill_super); -ENODEV; })

/* kill_sb helpers */
#define kill_block_super(sb)		do { } while (0)

/* prandom */
#define get_random_u32()		0
#define prandom_u32_max(max)		0

/* ctype */
#include <linux/ctype.h>

/* crc16 - use U-Boot's implementation */
#include <linux/crc16.h>

/* Timer and timing stubs */
#define HZ				1000
#define jiffies				0UL
#ifndef time_before
#define time_before(a, b)		((long)((a) - (b)) < 0)
#endif
#ifndef time_after
#define time_after(a, b)		time_before(b, a)
#endif
#define msecs_to_jiffies(m)		((m) * HZ / 1000)
#define jiffies_to_msecs(j)		((j) * 1000 / HZ)
#define round_jiffies_up(j)		(j)

/* Path lookup flags */
#define LOOKUP_FOLLOW			0x0001

/* I/O priority classes */
#define IOPRIO_CLASS_BE			2

/* Superblock flags */
#define SB_INLINECRYPT			(1 << 27)
#define SB_SILENT			(1 << 15)
#define SB_POSIXACL			(1 << 16)
#define SB_I_CGROUPWB			0
#define SB_I_ALLOW_HSM			0

/* Block open flags */
#define BLK_OPEN_READ			(1 << 0)
#define BLK_OPEN_WRITE			(1 << 1)
#define BLK_OPEN_RESTRICT_WRITES	(1 << 2)

/* Request operation (bits 0-7) and flags (bits 8+) */
#define REQ_OP_WRITE			1
#define REQ_OP_MASK			0xff

/* ensure these values are outside the operations mask */
#define REQ_SYNC			(1 << 8)
#define REQ_FUA				(1 << 9)

/* blk_holder_ops for block device */
struct blk_holder_ops {
	void (*mark_dead)(struct block_device *, bool);
};
static const struct blk_holder_ops fs_holder_ops;

/* end_buffer_write_sync - implemented in support.c */
void end_buffer_write_sync(struct buffer_head *bh, int uptodate);

/* File system management time flag */
#define FS_MGTIME			0

/* Block size */
#define BLOCK_SIZE			1024

/* Time constants */
#define NSEC_PER_SEC			1000000000L

/* EXT4 magic number */
#define EXT4_SUPER_MAGIC		0xEF53

/* Max file size for large files */
#define MAX_LFS_FILESIZE		((loff_t)LLONG_MAX)

/* blockgroup_lock for per-group locking */
struct blockgroup_lock {
	int num_locks;	/* U-Boot doesn't need real locking */
};

/* Buffer submission stubs - declarations for stub.c implementations */
int submit_bh(int op_flags, struct buffer_head *bh);
struct buffer_head *bdev_getblk(struct block_device *bdev, sector_t block,
				unsigned int size, gfp_t gfp);
int trylock_buffer(struct buffer_head *bh);

/* Trace stubs for super.c - declaration for stub.c implementation */
void trace_ext4_error(struct super_block *sb, const char *func, unsigned int line);

/* Ratelimiting - declaration for stub.c */
int ___ratelimit(struct ratelimit_state *rs, const char *func);

/* Filesystem notification - declaration for stub.c */
void fsnotify_sb_error(struct super_block *sb, struct inode *inode, int error);

/* File path operations - declaration for stub.c */
char *file_path(struct file *file, char *buf, int buflen);
struct block_device *file_bdev(struct file *file);

/* percpu_init_rwsem/percpu_free_rwsem are in linux/percpu.h */

/* Block device sync - declarations for stub.c */
int sync_blockdev(struct block_device *bdev);
void invalidate_bdev(struct block_device *bdev);

/* Kobject - declarations for stub.c */
void kobject_put(struct kobject *kobj);
/* wait_for_completion is now a macro in linux/completion.h */

/* DAX - declaration for stub.c */
void fs_put_dax(void *dax, void *holder);

/* fscrypt declarations are in ext4_fscrypt.h */

/* Inode allocation - declaration for stub.c */
void *alloc_inode_sb(struct super_block *sb, struct kmem_cache *cache,
		     gfp_t gfp);
void inode_set_iversion(struct inode *inode, u64 version);
int inode_generic_drop(struct inode *inode);

/* rwlock_init is a macro in linux/spinlock.h */

/* slab usercopy - use regular kmem_cache_create */
#define kmem_cache_create_usercopy(n, sz, al, fl, uo, us, c) \
	kmem_cache_create(n, sz, al, fl, c)

/* Inode buffer operations */
#define invalidate_inode_buffers(i)	do { } while (0)
#define clear_inode(i)			do { } while (0)

/* fsverity stubs (fscrypt macros are in ext4_fscrypt.h) */
#define fsverity_cleanup_inode(i)	do { } while (0)

/* NFS export helpers - declarations for stub.c */
struct dentry *generic_fh_to_dentry(struct super_block *sb, struct fid *fid,
				    int fh_len, int fh_type,
				    struct inode *(*get_inode)(struct super_block *, u64, u32));
struct dentry *generic_fh_to_parent(struct super_block *sb, struct fid *fid,
				    int fh_len, int fh_type,
				    struct inode *(*get_inode)(struct super_block *, u64, u32));

/* Path operations */
#define path_put(p)			do { } while (0)

/* I/O priority - declaration for stub.c */
int IOPRIO_PRIO_VALUE(int class, int data);

/* String operations */
char *kmemdup_nul(const char *s, size_t len, gfp_t gfp);
#define strscpy_pad(dst, src)		strncpy(dst, src, sizeof(dst))

/* fscrypt declarations are in ext4_fscrypt.h */

/* Memory allocation - declarations for stub.c */
void *kvzalloc(size_t size, gfp_t flags);
#define kvmalloc(size, flags)	kvzalloc(size, flags)

/* Time operations */
#define ktime_get_ns()			(0ULL)
#define nsecs_to_jiffies(ns)		((ns) / (NSEC_PER_SEC / HZ))

/* Superblock write operations */
#define sb_start_write_trylock(sb)	({ (void)(sb); 1; })
#define sb_start_write(sb)		do { } while (0)
#define sb_end_write(sb)		do { } while (0)

/* Scheduler stubs */
#define schedule_timeout_interruptible(t)	({ (void)(t); 0; })

/* Page allocation - declarations for stub.c */
unsigned long get_zeroed_page(gfp_t gfp);
void free_page(unsigned long addr);

/* DAX - declaration for stub.c */
void *fs_dax_get_by_bdev(struct block_device *bdev, u64 *start, u64 *len,
			 void *holder);

/* Block device atomic write stubs */
#define bdev_can_atomic_write(bdev)		({ (void)(bdev); 0; })
#define bdev_atomic_write_unit_max_bytes(bdev)	({ (void)(bdev); (unsigned int)0; })
#define bdev_atomic_write_unit_min_bytes(bdev)	({ (void)(bdev); 0UL; })

/* Superblock blocksize - declaration for stub.c */
int sb_set_blocksize(struct super_block *sb, int size);

/* Superblock min blocksize - stub */
static inline int sb_min_blocksize(struct super_block *sb, int size)
{
	return sb_set_blocksize(sb, size);
}

/* Block device size - declarations for stub.c */
int generic_check_addressable(unsigned int blocksize_bits, u64 num_blocks);
u64 sb_bdev_nr_blocks(struct super_block *sb);
unsigned int bdev_max_discard_sectors(struct block_device *bdev);

/* Blockgroup lock init - stub */
#define bgl_lock_init(lock)		do { } while (0)

/* Task I/O priority - declaration for stub.c */
void set_task_ioprio(void *task, int ioprio);

/* Superblock identity functions */
static inline void super_set_uuid(struct super_block *sb, const u8 *uuid,
				  unsigned len)
{
	if (len > sizeof(sb->s_uuid.b))
		len = sizeof(sb->s_uuid.b);
	memcpy(sb->s_uuid.b, uuid, len);
}

#define super_set_sysfs_name_bdev(sb)		do { } while (0)

/*
 * mb_cache - metadata block cache stubs for xattr.c
 * Not supported in U-Boot - xattr caching disabled
 */
struct mb_cache {
	int dummy;
};

struct mb_cache_entry {
	u64 e_value;
	unsigned long e_flags;
};

/* MB cache flags */
#define MBE_REUSABLE_B	0

#define mb_cache_create(bits)			kzalloc(sizeof(struct mb_cache), GFP_KERNEL)
#define mb_cache_destroy(cache)			do { kfree(cache); } while (0)
#define mb_cache_entry_find_first(c, h)		((struct mb_cache_entry *)NULL)
#define mb_cache_entry_find_next(c, e)		((struct mb_cache_entry *)NULL)
#define mb_cache_entry_delete_or_get(c, k, v)	((struct mb_cache_entry *)NULL)
#define mb_cache_entry_get(c, k, v)		((struct mb_cache_entry *)NULL)
#define mb_cache_entry_put(c, e)		do { (void)(c); (void)(e); } while (0)
#define mb_cache_entry_create(c, f, k, v, r)	({ (void)(c); (void)(f); (void)(k); (void)(v); (void)(r); 0; })
#define mb_cache_entry_delete(c, k, v)		do { (void)(c); (void)(k); (void)(v); } while (0)
#define mb_cache_entry_touch(c, e)		do { (void)(c); (void)(e); } while (0)
#define mb_cache_entry_wait_unused(e)		do { (void)(e); } while (0)

/* xattr helper stubs for xattr.c */
#define xattr_handler_can_list(h, d)		({ (void)(h); (void)(d); 0; })
#define xattr_prefix(h)				({ (void)(h); (const char *)NULL; })

/* Inode lock mutex classes */
#define I_MUTEX_XATTR		5
#define I_MUTEX_CHILD		4
#define I_MUTEX_PARENT		3
#define I_MUTEX_NORMAL		2

/* Nested inode locking stub */
#define inode_lock_nested(i, c)			do { (void)(i); (void)(c); } while (0)

/* Process flags */
#ifndef PF_MEMALLOC_NOFS
#define PF_MEMALLOC_NOFS	0x00040000
#endif

/* Dentry operations - declarations for stub.c */
void generic_set_sb_d_ops(struct super_block *sb);
struct dentry *d_make_root(struct inode *inode);

/* String operations - declarations for stub.c */
char *strreplace(const char *str, char old, char new);

/* Ratelimit - declaration for stub.c */
void ratelimit_state_init(void *rs, int interval, int burst);

/* Block device operations - declarations for stub.c */
void bdev_fput(void *file);
void *bdev_file_open_by_dev(dev_t dev, int flags, void *holder,
			    const struct blk_holder_ops *ops);

/* Filesystem sync - declaration for stub.c */
int sync_filesystem(void *sb);

/* Quota - declarations for stub.c */
#define dquot_suspend(sb, type)		({ (void)(sb); (void)(type); 0; })
int dquot_alloc_space_nodirty(struct inode *inode, loff_t size);
void dquot_free_space_nodirty(struct inode *inode, loff_t size);
int dquot_alloc_block(struct inode *inode, loff_t nr);
void dquot_free_block(struct inode *inode, loff_t nr);

/* Block device file operations - stubs */
#define set_blocksize(f, size)		({ (void)(f); (void)(size); 0; })
struct buffer_head *__bread(struct block_device *bdev, sector_t block, unsigned size);

/* flush_workqueue is now in linux/workqueue.h */

/* Quota stubs for super.c */
#define dquot_writeback_dquots(sb, type) do { (void)(sb); (void)(type); } while (0)
#define dquot_resume(sb, type)		do { (void)(sb); (void)(type); } while (0)
#define sb_any_quota_suspended(sb)	({ (void)(sb); 0; })

/*
 * Stubs for mballoc.c
 */

/* XArray stub structure */
struct xarray {
	int dummy;
};

/* Per-CPU stubs are in linux/percpu.h */

/* XArray function stubs */
#define xa_init(xa)			do { } while (0)
#define xa_destroy(xa)			do { } while (0)
#define xa_load(xa, index)		((void *)NULL)
#define xa_erase(xa, index)		do { (void)(xa); (void)(index); } while (0)
#define xa_insert(xa, index, entry, gfp) ({ (void)(xa); (void)(index); (void)(entry); (void)(gfp); 0; })
#define xa_empty(xa)			({ (void)(xa); 1; })

/* XArray iteration stubs - iterate zero times */
#define xa_for_each(xa, index, entry) \
	for ((index) = 0, (entry) = NULL; 0; )

#define xa_for_each_range(xa, index, entry, start, end) \
	for ((index) = (start), (entry) = NULL; 0; )

/* Bit operations for little-endian bitmaps */
#define __clear_bit_le(bit, addr)	clear_bit_le(bit, addr)

static inline void clear_bit_le(int nr, void *addr)
{
	unsigned char *p = (unsigned char *)addr + (nr >> 3);

	*p &= ~(1 << (nr & 7));
}

#define find_next_bit_le(addr, size, offset) \
	ext4_find_next_bit_le(addr, size, offset)

static inline unsigned long ext4_find_next_bit_le(const void *addr,
						  unsigned long size,
						  unsigned long offset)
{
	const unsigned char *p = addr;
	unsigned long bit;

	for (bit = offset; bit < size; bit++) {
		if (p[bit >> 3] & (1 << (bit & 7)))
			return bit;
	}
	return size;
}

/* atomic64 operations are now in asm-generic/atomic.h */

/* CPU cycle counter stub */
#define get_cycles()			(0ULL)

/* folio_address - get virtual address of folio data */
#undef folio_address
#define folio_address(folio)		((folio)->data)

/* sb_end_intwrite defined earlier */

/* WARN_RATELIMIT - just evaluate condition, no warning in U-Boot */
#define WARN_RATELIMIT(condition, ...) (condition)

/* folio_get - now implemented in support.c */

/* array_index_nospec - bounds checking without speculation (no-op in U-Boot) */
#define array_index_nospec(index, size) (index)

/* atomic_inc_return and atomic_add_return are now in asm-generic/atomic.h */

/* pde_data - proc dir entry data (not supported in U-Boot) */
#define pde_data(inode)			((void *)NULL)

/* seq_operations for procfs iteration */
struct seq_operations {
	void *(*start)(struct seq_file *m, loff_t *pos);
	void (*stop)(struct seq_file *m, void *v);
	void *(*next)(struct seq_file *m, void *v, loff_t *pos);
	int (*show)(struct seq_file *m, void *v);
};

/* DEFINE_RAW_FLEX - define a flexible array struct on the stack (stubbed to NULL) */
#define DEFINE_RAW_FLEX(type, name, member, count) \
	type *name = NULL

/* Block layer constants */
#define BLK_MAX_SEGMENT_SIZE		65536

/* num_possible_cpus, alloc_percpu, free_percpu are in linux/percpu.h */

/* Block device properties */
#define bdev_nonrot(bdev)		({ (void)(bdev); 0; })

/* sb_issue_discard - issue discard request (no-op in U-Boot) */
#define sb_issue_discard(sb, sector, nr_sects, gfp, flags) \
	({ (void)(sb); (void)(sector); (void)(nr_sects); (void)(gfp); (void)(flags); 0; })

/* atomic_sub, atomic64_sub, atomic_dec_and_test are in asm-generic/atomic.h */

/* RCU list operations - use regular list operations in U-Boot */
#define list_for_each_entry_rcu(pos, head, member, ...) \
	list_for_each_entry(pos, head, member)
#define list_del_rcu(entry)		list_del(entry)
#define list_add_rcu(new, head)		list_add(new, head)
#define list_add_tail_rcu(new, head)	list_add_tail(new, head)
/* Other RCU stubs are defined earlier in this file */

/* raw_cpu_ptr - get pointer to per-CPU data for current CPU */
#define raw_cpu_ptr(ptr)		(ptr)

/* Scheduler stubs */
#define schedule_timeout_uninterruptible(t) do { } while (0)
#define need_resched()			(0)

/* Block device operations */
#define sb_find_get_block_nonatomic(sb, block) \
	({ (void)(sb); (void)(block); (struct buffer_head *)NULL; })
#define __find_get_block_nonatomic(bdev, block, size) \
	({ (void)(bdev); (void)(block); (void)(size); (struct buffer_head *)NULL; })
#define bdev_discard_granularity(bdev) \
	({ (void)(bdev); 0U; })

/*
 * Stubs for page-io.c
 */

/* bio_vec - segment in a bio */
struct bio_vec {
	struct page *bv_page;
	unsigned int bv_len;
	unsigned int bv_offset;
};

/* bvec_iter - iterator for bio_vec */
struct bvec_iter {
	sector_t bi_sector;
	unsigned int bi_size;
	unsigned int bi_idx;
	unsigned int bi_bvec_done;
};

/* bio - block I/O structure */
struct bio {
	struct bio *bi_next;
	struct block_device *bi_bdev;
	unsigned long bi_opf;
	unsigned short bi_flags;
	unsigned short bi_ioprio;
	unsigned short bi_write_hint;
	int bi_status;
	struct bvec_iter bi_iter;
	atomic_t __bi_remaining;
	void *bi_private;
	void (*bi_end_io)(struct bio *);
};

/* bio_sectors - return number of sectors in bio */
static inline unsigned int bio_sectors(struct bio *bio)
{
	return bio->bi_iter.bi_size >> 9;
}

/* folio_iter for bio iteration */
struct folio_iter {
	int i;
	struct folio *folio;
	size_t offset;
	size_t length;
};

/* bio operations - stubs */
#define bio_for_each_folio_all(fi, bio) \
	for ((fi).i = 0; (fi).i < 0; (fi).i++)
#define bio_put(bio)			free(bio)
#define bio_alloc(bdev, vecs, op, gfp)	((struct bio *)calloc(1, sizeof(struct bio)))
#define submit_bio(bio)			do { } while (0)
#define BIO_MAX_VECS			256

/* refcount operations - map to atomic */
#define refcount_set(r, v)		atomic_set((atomic_t *)(r), v)
#define refcount_dec_and_test(r)	atomic_dec_and_test((atomic_t *)(r))
#define refcount_inc(r)			atomic_inc((atomic_t *)(r))

/* xchg - exchange value atomically */
#define xchg(ptr, new)			({ typeof(*(ptr)) __old = *(ptr); *(ptr) = (new); __old; })

/* printk_ratelimited - just use regular printk */
#define printk_ratelimited(fmt, ...)	do { } while (0)

/* mapping_set_error - record error in address_space */
#define mapping_set_error(m, e)		do { (void)(m); (void)(e); } while (0)

/* blk_status_to_errno - convert block status to errno */
#define blk_status_to_errno(status)	(-(status))

/* atomic_inc is in asm-generic/atomic.h */

/* GFP_NOIO - allocation without I/O */
#define GFP_NOIO			0

/* fscrypt page-io stubs are in ext4_fscrypt.h */

/* folio writeback operations */
#define folio_end_writeback(f)		do { (void)(f); } while (0)
#define folio_start_writeback(f)	do { (void)(f); } while (0)
#define folio_start_writeback_keepwrite(f) do { (void)(f); } while (0)
bool __folio_start_writeback(struct folio *folio, bool keep_write);

/* writeback control stubs */
#define wbc_init_bio(wbc, bio)		do { (void)(wbc); (void)(bio); } while (0)
#define wbc_account_cgroup_owner(wbc, folio, bytes) \
	do { (void)(wbc); (void)(folio); (void)(bytes); } while (0)

/* bio operations */
#define bio_add_folio(bio, folio, len, off) \
	({ (void)(bio); (void)(folio); (void)(len); (void)(off); 1; })

/*
 * Stubs for readpage.c
 */

/* mempool - memory pool stubs */
typedef void *mempool_t;
#define mempool_alloc(pool, gfp)	({ (void)(pool); (void)(gfp); (void *)NULL; })
#define mempool_free(elem, pool)	do { (void)(elem); (void)(pool); } while (0)
#define mempool_create_slab_pool(n, c)	({ (void)(n); (void)(c); (mempool_t *)NULL; })
#define mempool_destroy(pool)		do { (void)(pool); } while (0)

/* folio read operations */
#define folio_end_read(f, success)	do { (void)(f); (void)(success); } while (0)
#define folio_set_mappedtodisk(f)	do { (void)(f); } while (0)

/* fscrypt readpage stubs are in ext4_fscrypt.h */

/* fsverity stubs */
#define fsverity_verify_bio(bio)	do { (void)(bio); } while (0)
#define fsverity_enqueue_verify_work(work) do { (void)(work); } while (0)
#define fsverity_verify_folio(f)	({ (void)(f); 1; })
#define IS_VERITY(inode)		(0)

/* readahead operations */
#define readahead_count(rac)		({ (void)(rac); 0UL; })
#define readahead_folio(rac)		({ (void)(rac); (struct folio *)NULL; })

/* prefetch operations */
#define prefetchw(addr)			do { (void)(addr); } while (0)

/* block read operations */
#define block_read_full_folio(folio, get_block) \
	({ (void)(folio); (void)(get_block); 0; })

/*
 * Stubs for fast_commit.c
 */

/* Wait bit operations - stubbed for single-threaded U-Boot */
struct wait_bit_entry {
	struct list_head wq_entry;
};
#define DEFINE_WAIT_BIT(name, word, bit) \
	struct wait_bit_entry name = { }
#define bit_waitqueue(word, bit) \
	({ (void)(word); (void)(bit); (wait_queue_head_t *)NULL; })
#define prepare_to_wait(wq, wait, state) \
	do { (void)(wq); (void)(wait); (void)(state); } while (0)
#define prepare_to_wait_exclusive(wq, wait, state) \
	do { (void)(wq); (void)(wait); (void)(state); } while (0)
#define finish_wait(wq, wait) \
	do { (void)(wq); (void)(wait); } while (0)

/* Dentry name snapshot operations */
#define take_dentry_name_snapshot(snap, dentry) \
	do { (snap)->name = (dentry)->d_name; } while (0)
#define release_dentry_name_snapshot(snap) \
	do { (void)(snap); } while (0)

/* lockdep stubs */
#define lockdep_assert_not_held(lock)	do { (void)(lock); } while (0)

/* Request flags for block I/O */
#define REQ_IDLE		0
#define REQ_PREFLUSH		0

/* wake_up_bit - wake up threads waiting on a bit */
#define wake_up_bit(word, bit)		do { (void)(word); (void)(bit); } while (0)

/* Dentry allocation stubs */
#define d_alloc(parent, name)		({ (void)(parent); (void)(name); (struct dentry *)NULL; })
#define d_drop(dentry)			do { (void)(dentry); } while (0)

/* get_current_ioprio - I/O priority (not used in U-Boot) */
#define get_current_ioprio()		(0)

/* JBD2 checkpoint.c stubs */
#define mutex_lock_io(m)		mutex_lock(m)
#define write_dirty_buffer(bh, flags)	sync_dirty_buffer(bh)
#define spin_needbreak(l)		({ (void)(l); 0; })

/* JBD2 commit.c stubs */
#define clear_bit_unlock(nr, addr)	clear_bit(nr, addr)
#define smp_mb__after_atomic()		do { } while (0)
#define folio_trylock(f)		({ (void)(f); 1; })
#define ktime_get_coarse_real_ts64(ts)	do { (ts)->tv_sec = 0; (ts)->tv_nsec = 0; } while (0)
#define filemap_fdatawait_range_keep_errors(m, s, e) \
	({ (void)(m); (void)(s); (void)(e); 0; })
#define crc32_be(crc, p, len)		crc32(crc, p, len)
void free_buffer_head(struct buffer_head *bh);

/* ext4l support functions (support.c) */
void ext4l_crc32c_init(void);
void bh_cache_release_jbd(void);
void bh_cache_clear(void);
int bh_cache_sync(void);
int ext4l_read_block(sector_t block, size_t size, void *buffer);
int ext4l_write_block(sector_t block, size_t size, void *buffer);
void ext4l_msg_init(void);
void ext4l_record_msg(const char *msg, int len);
struct membuf *ext4l_get_msg_buf(void);
void ext4l_print_msgs(void);

/* ext4l interface functions (interface.c) */
struct blk_desc *ext4l_get_blk_dev(void);
struct disk_partition *ext4l_get_partition(void);

#define sb_is_blkdev_sb(sb)		({ (void)(sb); 0; })

/* DEFINE_WAIT stub - creates a wait queue entry */
#define DEFINE_WAIT(name)		int name = 0

/* cond_resched_lock - conditionally reschedule while holding a lock */
#define cond_resched_lock(lock)		do { (void)(lock); } while (0)

/* JBD2 journal.c stubs */
struct buffer_head *alloc_buffer_head(gfp_t gfp_mask);
struct buffer_head *__getblk(struct block_device *bdev, sector_t block,
			     unsigned int size);
int bmap(struct inode *inode, sector_t *block);

/* seq_file operations for /proc - stubs */
#define seq_open(f, ops)		({ (void)(f); (void)(ops); 0; })
#define seq_release(i, f)		({ (void)(i); (void)(f); 0; })

/* proc_ops structure for journal.c */
struct proc_ops {
	int (*proc_open)(struct inode *, struct file *);
	ssize_t (*proc_read)(struct file *, char *, size_t, loff_t *);
	loff_t (*proc_lseek)(struct file *, loff_t, int);
	int (*proc_release)(struct inode *, struct file *);
};

/* seq_read and seq_lseek declarations (defined in stub.c) */
ssize_t seq_read(struct file *f, char *b, size_t s, loff_t *p);
loff_t seq_lseek(struct file *f, loff_t o, int w);

/* S_IRUGO file mode if not defined */
#ifndef S_IRUGO
#define S_IRUGO		(S_IRUSR | S_IRGRP | S_IROTH)
#endif

/* procfs stubs */
#define proc_mkdir(name, parent)	({ (void)(name); (void)(parent); (struct proc_dir_entry *)NULL; })
#define proc_create_data(n, m, p, ops, d) \
	({ (void)(n); (void)(m); (void)(p); (void)(ops); (void)(d); (struct proc_dir_entry *)NULL; })
#define remove_proc_entry(n, p)		do { (void)(n); (void)(p); } while (0)

/* lockdep stubs (struct lock_class_key defined earlier) */
#define lockdep_init_map(...)	do { } while (0)

/* Block device operations for journal.c */
int bh_read(struct buffer_head *bh, int flags);
#define bh_read_nowait(bh, flags)	bh_read(bh, flags)
#define bh_readahead_batch(n, bhs, f)	do { (void)(n); (void)(bhs); (void)(f); } while (0)
#define truncate_inode_pages_range(m, s, e) \
	do { (void)(m); (void)(s); (void)(e); } while (0)
#define blkdev_issue_discard(bdev, s, n, gfp) \
	({ (void)(bdev); (void)(s); (void)(n); (void)(gfp); 0; })
#define blkdev_issue_zeroout(bdev, s, n, gfp, f) \
	({ (void)(bdev); (void)(s); (void)(n); (void)(gfp); (void)(f); 0; })
#ifndef SECTOR_SHIFT
#define SECTOR_SHIFT	9
#endif
#define mapping_max_folio_order(m)	({ (void)(m); 0; })

/* Memory allocation for journal.c */
#define __get_free_pages(gfp, order)	((unsigned long)memalign(PAGE_SIZE, PAGE_SIZE << (order)))
#define free_pages(addr, order)		free((void *)(addr))
#define get_order(size)			ilog2(roundup_pow_of_two((size) / PAGE_SIZE))

/* Ratelimited printk for journal.c */
#define pr_notice_ratelimited(fmt, ...)	pr_notice(fmt, ##__VA_ARGS__)

/*
 * Stubs for mmp.c
 */

/* init_utsname - returns pointer to system name structure */
struct new_utsname {
	char nodename[65];
};
static inline struct new_utsname *init_utsname(void)
{
	static struct new_utsname uts = { .nodename = "u-boot" };
	return &uts;
}

/*
 * Stubs for move_extent.c
 */

/* down_write_nested - nested write lock acquisition */
#define down_write_nested(sem, subclass) \
	do { (void)(sem); (void)(subclass); } while (0)

/* filemap_release_folio - try to release a folio */
#define filemap_release_folio(folio, gfp) \
	({ (void)(folio); (void)(gfp); 1; })

/* IS_SWAPFILE - check if inode is a swap file */
#define IS_SWAPFILE(inode)	({ (void)(inode); 0; })

/* PAGE_MASK - mask for page alignment */
#ifndef PAGE_MASK
#define PAGE_MASK	(~(PAGE_SIZE - 1))
#endif

/* lock_two_nondirectories - lock two inodes in order */
#define lock_two_nondirectories(i1, i2) \
	do { (void)(i1); (void)(i2); } while (0)
#define unlock_two_nondirectories(i1, i2) \
	do { (void)(i1); (void)(i2); } while (0)

/*
 * Stubs for resize.c
 */

/* test_and_set_bit_lock - test and set a bit atomically */
#define test_and_set_bit_lock(nr, addr)	test_and_set_bit(nr, addr)

/* time_is_before_jiffies - check if time is before current jiffies */
#define time_is_before_jiffies(a)	({ (void)(a); 0; })

/* ext4_update_overhead - declaration for stub.c */
int ext4_update_overhead(struct super_block *sb, bool force);

/*
 * Stubs for fsmap.c
 */

/* fsmap.c stubs - struct fsmap from linux/fsmap.h */
struct fsmap {
	__u32	fmr_device;	/* device id */
	__u32	fmr_flags;	/* mapping flags */
	__u64	fmr_physical;	/* device offset of segment */
	__u64	fmr_owner;	/* owner id */
	__u64	fmr_offset;	/* file offset of segment */
	__u64	fmr_length;	/* length of segment */
	__u64	fmr_reserved[3]; /* must be zero */
};

#define FMR_OWN_FREE		(-1ULL)
#define FMR_OWN_UNKNOWN		(-2ULL)
#define FMR_OWNER(type, code)	(((__u64)(type) << 32) | (__u64)(code))
#define FMR_OF_SPECIAL_OWNER	(1 << 0)
#define FMH_IF_VALID		0
#define FMH_OF_DEV_T		(1 << 0)

/* list_sort and sort stubs for fsmap.c */
#define list_sort(priv, head, cmp) \
	do { (void)(priv); (void)(head); (void)(cmp); } while (0)
#define sort(base, num, size, cmp, swap) \
	do { (void)(base); (void)(num); (void)(size); (void)(cmp); (void)(swap); } while (0)

#endif /* __EXT4_UBOOT_H__ */

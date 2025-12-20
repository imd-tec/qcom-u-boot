/* SPDX-License-Identifier: GPL-2.0 */
/*
 * U-Boot compatibility header for ext4l filesystem
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

#include <linux/types.h>
#include <linux/bitops.h>
#include <vsprintf.h>		/* For panic() */
#include <linux/string.h>
#include <linux/stat.h>
#include <asm/byteorder.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/cred.h>
#include <linux/fs.h>
#include <linux/iomap.h>
#include <linux/seq_file.h>

/*
 * Override no_printk to avoid format warnings in disabled debug prints.
 * The Linux kernel uses sector_t as u64, but U-Boot uses unsigned long.
 * This causes format mismatches with %llu that we want to ignore.
 */
#undef no_printk
#define no_printk(fmt, ...)	({ 0; })

/* Rotate left - not available in U-Boot */
static inline u32 rol32(u32 word, unsigned int shift)
{
	return (word << (shift & 31)) | (word >> ((-shift) & 31));
}

/* Time types */
struct timespec64 {
	time_t tv_sec;
	long tv_nsec;
};

/* ktime_t - kernel time type */
typedef s64 ktime_t;

/* Jiffy constants */
#define MAX_JIFFY_OFFSET	((~0UL >> 1) - 1)

/* Block device name size */
#define BDEVNAME_SIZE		32

/* Atomic types - stubs for single-threaded U-Boot */
typedef struct { int counter; } atomic_t;
typedef struct { long counter; } atomic64_t;

#define atomic_read(v)		((v)->counter)
#define atomic_set(v, i)	((v)->counter = (i))
#define atomic_inc(v)		((v)->counter++)
#define atomic_dec(v)		((v)->counter--)
#define atomic64_read(v)	((v)->counter)
#define atomic64_set(v, i)	((v)->counter = (i))
#define atomic_dec_if_positive(v)	(--(v)->counter)

/* SMP stubs - U-Boot is single-threaded */
#define raw_smp_processor_id()	0

/* cmpxchg - compare and exchange, single-threaded version */
#define cmpxchg(ptr, old, new) ({		\
	typeof(*(ptr)) __old = (old);		\
	typeof(*(ptr)) __new = (new);		\
	typeof(*(ptr)) __ret = *(ptr);		\
	if (__ret == __old)			\
		*(ptr) = __new;			\
	__ret;					\
})

/* Reference count type */
typedef struct { atomic_t refs; } refcount_t;

/* Lock types - stubs for single-threaded U-Boot */
typedef int rwlock_t;
/* spinlock_t is defined in linux/compat.h */

#define read_lock(l)		do { } while (0)
#define read_unlock(l)		do { } while (0)
#define write_lock(l)		do { } while (0)
#define write_unlock(l)		do { } while (0)

/* RB tree types - stubs */
struct rb_node {
	unsigned long __rb_parent_color;
	struct rb_node *rb_right;
	struct rb_node *rb_left;
};

struct rb_root {
	struct rb_node *rb_node;
};

#define RB_ROOT (struct rb_root) { NULL, }

/* percpu_counter - use Linux header */
#include <linux/percpu_counter.h>

/* name_snapshot - stub */
struct name_snapshot {
	const char *name;
};

/* Project ID type */
typedef struct { unsigned int val; } kprojid_t;

#define make_kprojid(ns, id)	((kprojid_t){ .val = (id) })
#define from_kprojid(ns, kprojid)	((kprojid).val)
#define projid_eq(a, b)		((a).val == (b).val)

/* kobject - stub */
struct kobject {
	const char *name;
};

/* completion - stub */
struct completion {
	unsigned int done;
};

/* Cache alignment - stub */
#define ____cacheline_aligned_in_smp

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

/* Forward declarations for iomap_ops */
struct inode;
struct address_space;

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

/* iomap types - only define if linux/iomap.h not included */
#ifndef LINUX_IOMAP_H
#define IOMAP_MAPPED	0
#define IOMAP_INLINE	1
#define IOMAP_UNWRITTEN	2
#define IOMAP_DELALLOC	3
#define IOMAP_HOLE	4

struct iomap {
	u64 addr;
	loff_t offset;
	loff_t length;
	u16 type;
	u16 flags;
	struct block_device *bdev;
	void *inline_data;
};

struct iomap_ops {
	int (*iomap_begin)(struct inode *inode, loff_t pos, loff_t length,
			   unsigned flags, struct iomap *iomap, struct iomap *srcmap);
	int (*iomap_end)(struct inode *inode, loff_t pos, loff_t length,
			 ssize_t written, unsigned flags, struct iomap *iomap);
};

/* iomap DIO flags */
#define IOMAP_DIO_UNWRITTEN	(1 << 0)
#define IOMAP_DIO_FORCE_WAIT	(1 << 1)
#endif /* LINUX_IOMAP_H */

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

/* fscrypt_str - stub */
struct fscrypt_str {
	unsigned char *name;
	u32 len;
};

/* percpu rw semaphore - stubs */
struct percpu_rw_semaphore {
	int dummy;
};

#define percpu_down_read(sem)	do { } while (0)
#define percpu_up_read(sem)	do { } while (0)
#define percpu_down_write(sem)	do { } while (0)
#define percpu_up_write(sem)	do { } while (0)

/* Memory allocation context - stubs */
static inline unsigned int memalloc_nofs_save(void) { return 0; }
static inline void memalloc_nofs_restore(unsigned int flags) { }

/* Inode flags - stubs */
#define IS_CASEFOLDED(inode)	(0)
#define IS_ENCRYPTED(inode)	(0)
#define S_NOQUOTA		0

/* fscrypt context - stub */
#define FSCRYPT_SET_CONTEXT_MAX_SIZE	40

/* User namespace - stub */
struct user_namespace {
	int dummy;
};
extern struct user_namespace init_user_ns;

/* BUG_ON / BUG - stubs (panic is in vsprintf.h) */
#define BUG_ON(cond)	do { } while (0)
#define BUG()		do { } while (0)

/* might_sleep - stub */
#define might_sleep()	do { } while (0)

/* sb_rdonly - stub */
#define sb_rdonly(sb)	(0)

/* Trace stubs */
#define trace_ext4_journal_start_inode(...)	do { } while (0)
#define trace_ext4_journal_start_sb(...)	do { } while (0)
#define trace_ext4_journal_start_reserved(...)	do { } while (0)
#define trace_ext4_forget(...)			do { } while (0)
#define trace_ext4_read_block_bitmap_load(...)	do { } while (0)

/* Buffer operations - stubs */
#define wait_on_buffer(bh)		do { } while (0)
#define __bforget(bh)			do { } while (0)
#define mark_buffer_dirty_inode(bh, i)	do { } while (0)
#define mark_buffer_dirty(bh)		do { } while (0)
#define lock_buffer(bh)			do { } while (0)
#define unlock_buffer(bh)		do { } while (0)
#define sb_getblk(sb, block)		((struct buffer_head *)NULL)

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

/* Little-endian bit operations */
#define __set_bit_le(nr, addr)		((void)(nr), (void)(addr))
#define test_bit_le(nr, addr)		({ (void)(nr); (void)(addr); 0; })
#define find_next_zero_bit_le(addr, size, offset) \
	({ (void)(addr); (void)(size); (offset); })
#define __test_and_clear_bit_le(nr, addr) ({ (void)(nr); (void)(addr); 0; })
#define __test_and_set_bit_le(nr, addr)	({ (void)(nr); (void)(addr); 0; })

/* KUNIT stub */
#define KUNIT_STATIC_STUB_REDIRECT(...)	do { } while (0)

/* percpu_counter operations - stubs */
#define percpu_counter_read_positive(fbc)	((fbc)->count)
#define percpu_counter_sum_positive(fbc)	((fbc)->count)
#define percpu_counter_add(fbc, amount)		((fbc)->count += (amount))
#define percpu_counter_inc(fbc)			((fbc)->count++)
#define percpu_counter_dec(fbc)			((fbc)->count--)
#define percpu_counter_initialized(fbc)		(1)

/* Group permission - stub */
#define in_group_p(gid)			(0)

/* Quota operations - stubs (only define if quotaops.h not included) */
#ifndef _LINUX_QUOTAOPS_H
#define dquot_alloc_block_nofail(inode, nr)	({ (void)(inode); (void)(nr); 0; })
#define dquot_initialize(inode)			({ (void)(inode); 0; })
#define dquot_free_inode(inode)			do { (void)(inode); } while (0)
#define dquot_alloc_inode(inode)		({ (void)(inode); 0; })
#define dquot_drop(inode)			do { (void)(inode); } while (0)
#endif /* _LINUX_QUOTAOPS_H */

/* Trace stubs for ialloc.c */
#define trace_ext4_load_inode_bitmap(...)	do { } while (0)
#define trace_ext4_free_inode(...)		do { } while (0)
#define trace_ext4_allocate_inode(...)		do { } while (0)
#define trace_ext4_request_inode(...)		do { } while (0)

/* icount - inode reference count */
#define icount_read(inode)			(1)

/* d_inode - get inode from dentry */
#define d_inode(dentry)				((dentry) ? (dentry)->d_inode : NULL)

/* Random number functions */
#define get_random_u32_below(max)		(0)

/* Buffer cache operations */
#define sb_find_get_block(sb, block)		((struct buffer_head *)NULL)
#define sync_dirty_buffer(bh)			({ (void)(bh); 0; })

/* Time functions */
#define ktime_get_real_seconds()		(0)
#define time_before32(a, b)			(0)

/* Inode operations - stubs */
#define new_inode(sb)				((struct inode *)NULL)
#define i_uid_write(inode, uid)			do { } while (0)
#define i_gid_write(inode, gid)			do { } while (0)
#define inode_fsuid_set(inode, idmap)		do { } while (0)
#define inode_init_owner(idmap, i, dir, mode)	do { } while (0)
#define insert_inode_locked(inode)		(0)
#define unlock_new_inode(inode)			do { } while (0)
#define clear_nlink(inode)			do { } while (0)
#define IS_DIRSYNC(inode)			({ (void)(inode); 0; })

/* fscrypt stubs */
#define fscrypt_prepare_new_inode(dir, i, e)	({ (void)(dir); (void)(i); (void)(e); 0; })
#define fscrypt_set_context(inode, handle)	({ (void)(inode); (void)(handle); 0; })

/* ext4_init_acl is provided by acl.h */
/* xattr stubs for files that don't include xattr.h */
struct super_block;
struct buffer_head;
struct qstr;

int __ext4_xattr_set_credits(struct super_block *sb, struct inode *inode,
			     struct buffer_head *block_bh, size_t value_len,
			     bool is_create);
/* ext4_init_security is provided by xattr.h */

/* inode state stubs */
#define is_bad_inode(inode)			(0)

/* Block device operations - stubs */
#define sb_issue_zeroout(sb, blk, num, gfp)	({ (void)(sb); (void)(blk); (void)(num); (void)(gfp); 0; })
#define blkdev_issue_flush(bdev)		({ (void)(bdev); 0; })

/* do_div - divide u64 by u32 */
#define do_div(n, base) ({			\
	unsigned int __base = (base);		\
	unsigned int __rem;			\
	__rem = ((unsigned long long)(n)) % __base;	\
	(n) = ((unsigned long long)(n)) / __base;	\
	__rem;					\
})

/* Inode locking - stubs */
#define inode_is_locked(i)	(1)
#define i_size_write(i, s)	do { (i)->i_size = (s); } while (0)
#define i_size_read(i)		((i)->i_size)

/* spin_trylock is defined in linux/spinlock.h */

/* Atomic extras */
#define atomic_add_unless(v, a, u)	({ (void)(v); (void)(a); (void)(u); 1; })

/* Block group lock - stub */
#define bgl_lock_ptr(lock, group)	NULL

/* RCU stubs */
#define rcu_read_lock()			do { } while (0)
#define rcu_read_unlock()		do { } while (0)
#define rcu_dereference(p)		(p)
#define rcu_dereference_protected(p, c)	(p)
#define rcu_assign_pointer(p, v)	((p) = (v))
#define call_rcu(head, func)		do { func(head); } while (0)

/* RCU head for callbacks - defined in linux/compat.h as callback_head */

/* lockdep stubs */
#define lockdep_is_held(lock)		(1)

/* Memory allocation - use linux/slab.h which is already available */
#include <linux/slab.h>

/* KMEM_CACHE macro - not in U-Boot's slab.h */
#define KMEM_CACHE(s, flags)		((struct kmem_cache *)1)

/* RB tree operations - stubs */
#define rb_entry(ptr, type, member) \
	container_of(ptr, type, member)
#define rb_first(root)		((root)->rb_node)
#define rb_next(node)		((node)->rb_right)
#define rb_prev(node)		((node)->rb_left)
#define rb_insert_color(node, root)	do { } while (0)
#define rb_erase(node, root)		do { } while (0)
#define rb_link_node(node, parent, rb_link)	do { *(rb_link) = (node); } while (0)
#define RB_EMPTY_ROOT(root)	((root)->rb_node == NULL)
#define rbtree_postorder_for_each_entry_safe(pos, n, root, field) \
	for (pos = NULL, (void)(n); pos != NULL; )

/* RCU barrier - stub */
#define rcu_barrier()		do { } while (0)

/* inode/dentry operations - stubs */
#define iput(inode)		do { } while (0)

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

/* sector_t - needed before buffer_head.h */
typedef unsigned long sector_t;

/* Buffer head - from linux/buffer_head.h */
#include <linux/buffer_head.h>

/* BH_JBDPrivateStart is defined in jbd2.h as an enum value */

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

/* fscrypt_dummy_policy - stub */
struct fscrypt_dummy_policy {
	int dummy;
};

/* errseq_t is defined in linux/fs.h */

/* time64_t */
typedef s64 time64_t;

/* IS_NOQUOTA - stub */
#define IS_NOQUOTA(inode)	(0)

/* qstr - quick string for filenames (must be before dentry) */
struct qstr {
	const unsigned char *name;
	unsigned int len;
};

/* dentry - stub */
struct dentry {
	struct qstr d_name;
	struct inode *d_inode;
	struct super_block *d_sb;
	struct dentry *d_parent;
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
	const char *s_id;
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
};

/* Block device read-only check - stub */
static inline int bdev_read_only(struct block_device *bdev)
{
	return 0;
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
	struct rw_semaphore i_rwsem;	/* inode lock */
	const char *i_link;		/* Symlink target for fast symlinks */
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

/* fscrypt stubs - encryption not supported in U-Boot */
static inline bool fscrypt_has_encryption_key(const struct inode *inode)
{
	return false;
}

static inline u64 fscrypt_fname_siphash(const struct inode *dir,
					const struct qstr *name)
{
	return 0;
}

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

/* BUILD_BUG_ON - compile-time assertion */
#define BUILD_BUG_ON(cond) ((void)sizeof(char[1 - 2 * !!(cond)]))

/* Warning macros - stubs */
#define WARN_ON_ONCE(cond) ({ (void)(cond); 0; })
#define WARN_ON(cond) ({ (void)(cond); 0; })
#define WARN_ONCE(cond, fmt, ...) ({ (void)(cond); 0; })
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

/* Trace functions for extents.c */
#define trace_ext4_ext_load_extent(...)		do { } while (0)
#define trace_ext4_ext_rm_idx(...)		do { } while (0)
#define trace_ext4_remove_blocks(...)		do { } while (0)
#define trace_ext4_ext_rm_leaf(...)		do { } while (0)
#define trace_ext4_ext_remove_space(...)	do { } while (0)
#define trace_ext4_ext_remove_space_done(...)	do { } while (0)
#define trace_ext4_ext_convert_to_initialized_enter(...)	do { } while (0)
#define trace_ext4_ext_convert_to_initialized_fastpath(...)	do { } while (0)
#define trace_ext4_ext_handle_unwritten_extents(...)	do { } while (0)
#define trace_ext4_get_implied_cluster_alloc_exit(...)	do { } while (0)
#define trace_ext4_ext_map_blocks_enter(...)	do { } while (0)
#define trace_ext4_ext_map_blocks_exit(...)	do { } while (0)
#define trace_ext4_ext_show_extent(...)		do { } while (0)
#define trace_ext4_collapse_range(...)		do { } while (0)
#define trace_ext4_insert_range(...)		do { } while (0)
#define trace_ext4_zero_range(...)		do { } while (0)
#define trace_ext4_fallocate_enter(...)		do { } while (0)
#define trace_ext4_fallocate_exit(...)		do { } while (0)

/* rwsem is_locked stub */
#define rwsem_is_locked(sem)		(1)

/* Buffer operations */
#define sb_getblk_gfp(sb, blk, gfp)	((struct buffer_head *)NULL)
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

/* Trace functions for indirect.c */
#define trace_ext4_ind_map_blocks_enter(...)	do { } while (0)
#define trace_ext4_ind_map_blocks_exit(...)	do { } while (0)

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
	return NULL;
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

/* write lock variants */
#define write_trylock(lock)		({ (void)(lock); 1; })

/* percpu counter init/destroy */
#define percpu_counter_init(fbc, val, gfp)	({ (fbc)->count = (val); 0; })
#define percpu_counter_destroy(fbc)		do { } while (0)

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
#define offset_in_folio(f, p)			({ (void)(f); (unsigned int)((p) & (PAGE_SIZE - 1)); })
#define folio_buffers(f)			({ (void)(f); (struct buffer_head *)NULL; })
#define folio_test_uptodate(f)			({ (void)(f); 1; })
#define folio_pos(f)				({ (void)(f); 0LL; })
#define folio_size(f)				({ (void)(f); PAGE_SIZE; })
#define folio_unlock(f)				do { (void)(f); } while (0)
#define folio_put(f)				do { (void)(f); } while (0)
#define folio_lock(f)				do { (void)(f); } while (0)
#define folio_batch_init(fb)			do { (fb)->nr = 0; } while (0)
#define filemap_get_folios(m, i, e, fb)		({ (void)(m); (void)(i); (void)(e); (void)(fb); 0U; })

/* xa_mark_t - xarray mark type */
typedef unsigned int xa_mark_t;

/* Page cache tags */
#define PAGECACHE_TAG_DIRTY	0
#define PAGECACHE_TAG_TOWRITE	1
#define PAGECACHE_TAG_WRITEBACK	2

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

/* Forward declarations for address_space_operations */
struct writeback_control;
struct swap_info_struct;

/* address_space_operations stub */
struct address_space_operations {
	int (*read_folio)(struct file *, struct folio *);
	void (*readahead)(struct readahead_control *);
	sector_t (*bmap)(struct address_space *, sector_t);
	void (*invalidate_folio)(struct folio *, unsigned long, unsigned long);
	bool (*release_folio)(struct folio *, gfp_t);
	int (*write_begin)(const struct kiocb *, struct address_space *, loff_t, unsigned, struct folio **, void **);
	int (*write_end)(const struct kiocb *, struct address_space *, loff_t, unsigned, unsigned, struct folio *, void *);
	int (*writepages)(struct address_space *, struct writeback_control *);
	bool (*dirty_folio)(struct address_space *, struct folio *);
	bool (*is_partially_uptodate)(struct folio *, size_t, size_t);
	int (*error_remove_folio)(struct address_space *, struct folio *);
	int (*migrate_folio)(struct address_space *, struct folio *, struct folio *, int);
	int (*swap_activate)(struct swap_info_struct *, struct file *, sector_t *);
};

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
#define kmap_local_folio(folio, off)	({ (void)(folio); (void)(off); (void *)NULL; })
#define kunmap_local(addr)		do { (void)(addr); } while (0)

/* Folio zeroing stubs for inline.c */
#define folio_zero_tail(f, off, kaddr)	({ (void)(f); (void)(off); (void)(kaddr); (void *)NULL; })
#define folio_zero_segment(f, s, e)	do { (void)(f); (void)(s); (void)(e); } while (0)

/* mapping_gfp_mask stub */
#define mapping_gfp_mask(m)		({ (void)(m); GFP_KERNEL; })

/* __filemap_get_folio stub */
static inline struct folio *__filemap_get_folio(struct address_space *mapping,
						pgoff_t index, unsigned int fgp_flags,
						gfp_t gfp)
{
	return NULL;
}

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

/* ilog2 - log base 2 */
#include <log.h>
#define ilog2(n) (fls(n) - 1)

/* Trace stubs for inode.c */
#define trace_ext4_begin_ordered_truncate(...)	do { } while (0)
#define trace_ext4_evict_inode(...)		do { } while (0)
#define trace_ext4_da_update_reserve_space(...)	do { } while (0)
#define trace_ext4_da_reserve_space(...)	do { } while (0)
#define trace_ext4_da_release_space(...)	do { } while (0)
#define trace_ext4_da_write_pages_extent(...)	do { } while (0)
#define trace_ext4_writepages(...)		do { } while (0)
#define trace_ext4_da_write_folios_start(...)	do { } while (0)
#define trace_ext4_da_write_folios_end(...)	do { } while (0)
#define trace_ext4_writepages_result(...)	do { } while (0)
#define trace_ext4_da_write_begin(...)		do { } while (0)
#define trace_ext4_da_write_end(...)		do { } while (0)
#define trace_ext4_alloc_da_blocks(...)		do { } while (0)
#define trace_ext4_read_folio(...)		do { } while (0)
#define trace_ext4_invalidate_folio(...)	do { } while (0)
#define trace_ext4_journalled_invalidate_folio(...)	do { } while (0)
#define trace_ext4_release_folio(...)		do { } while (0)
#define trace_ext4_punch_hole(...)		do { } while (0)
#define trace_ext4_truncate_enter(...)		do { } while (0)
#define trace_ext4_truncate_exit(...)		do { } while (0)
#define trace_ext4_load_inode(...)		do { } while (0)
#define trace_ext4_other_inode_update_time(...)	do { } while (0)
#define trace_ext4_mark_inode_dirty(...)	do { } while (0)
#define trace_ext4_write_begin(...)		do { } while (0)
#define trace_ext4_write_end(...)		do { } while (0)
#define trace_ext4_journalled_write_end(...)	do { } while (0)
#define trace_ext4_sync_file_enter(...)		do { } while (0)
#define trace_ext4_sync_file_exit(...)		do { } while (0)
#define trace_ext4_unlink_enter(...)		do { } while (0)
#define trace_ext4_unlink_exit(...)		do { } while (0)

/* Dentry operations - stubs */
#define d_find_any_alias(i)			({ (void)(i); (struct dentry *)NULL; })
#define dget_parent(d)				({ (void)(d); (struct dentry *)NULL; })
#define dput(d)					do { (void)(d); } while (0)
#define d_splice_alias(i, d)			({ (void)(i); (void)(d); (struct dentry *)NULL; })
#define d_obtain_alias(i)			({ (void)(i); (struct dentry *)NULL; })
#define d_instantiate_new(d, i)			do { (void)(d); (void)(i); } while (0)
#define d_instantiate(d, i)			do { (void)(d); (void)(i); } while (0)
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

/* fscrypt/fsverity stubs */
#define fscrypt_file_open(i, f)			({ (void)(i); (void)(f); 0; })
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
#define folio_mapped(f)				(0)
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

/* Percpu counter sub */
#define percpu_counter_sub(fbc, amount)		((fbc)->count -= (amount))

/* Filemap operations - additional */
#define filemap_get_folio(m, i)			((struct folio *)NULL)
#define filemap_get_folios_tag(m, s, e, t, fb)	({ (void)(m); (void)(s); (void)(e); (void)(t); (void)(fb); 0U; })
#define filemap_flush(m)			({ (void)(m); 0; })
#define filemap_write_and_wait(m)		({ (void)(m); 0; })
#define filemap_dirty_folio(m, f)		({ (void)(m); (void)(f); false; })
#define filemap_lock_folio(m, i)		((struct folio *)NULL)
#define filemap_invalidate_lock_shared(m)	do { } while (0)
#define filemap_invalidate_unlock_shared(m)	do { } while (0)
#define mapping_tagged(m, t)			(0)
#define tag_pages_for_writeback(m, s, e)	do { } while (0)
#define try_to_writeback_inodes_sb(sb, r)	do { } while (0)
#define mapping_gfp_constraint(m, g)		(g)
#define mapping_set_folio_order_range(m, l, h)	do { } while (0)
#define filemap_splice_read(i, p, pi, l, f)	({ (void)(i); (void)(p); (void)(pi); (void)(l); (void)(f); 0L; })

/* Buffer operations - additional */
#define getblk_unmovable(bd, b, s)		((struct buffer_head *)NULL)
#define create_empty_buffers(f, s, flags)	({ (void)(f); (void)(s); (void)(flags); (struct buffer_head *)NULL; })
#define bh_offset(bh)				(0UL)
#define block_invalidate_folio(f, o, l)		do { } while (0)
#define block_write_end(pos, len, copied, folio) ({ (void)(pos); (void)(len); (void)(folio); (copied); })
#define block_dirty_folio(m, f)			({ (void)(m); (void)(f); false; })
#define try_to_free_buffers(f)			({ (void)(f); true; })
#define block_commit_write(f, f2, t)		do { } while (0)
#define block_page_mkwrite(v, f, g)		((vm_fault_t)0)
#define map_bh(bh, sb, block)			do { } while (0)
#define write_begin_get_folio(iocb, m, idx, l)	({ (void)(iocb); (void)(m); (void)(idx); (void)(l); (struct folio *)NULL; })

/* fscrypt stubs - additional */
#define fscrypt_inode_uses_fs_layer_crypto(i)	(0)
#define fscrypt_decrypt_pagecache_blocks(f, l, o) ({ (void)(f); (void)(l); (void)(o); 0; })
#define fscrypt_zeroout_range(i, lb, pb, l)	({ (void)(i); (void)(lb); (void)(pb); (void)(l); 0; })
#define fscrypt_limit_io_blocks(i, lb, l)	(l)
#define fscrypt_prepare_setattr(d, a)		({ (void)(d); (void)(a); 0; })
#define fscrypt_dio_supported(i)		(1)
#define fscrypt_match_name(f, n, l)		({ (void)(f); (void)(n); (void)(l); 1; })
#define fscrypt_has_permitted_context(p, c)	({ (void)(p); (void)(c); 1; })
#define fscrypt_is_nokey_name(d)		({ (void)(d); 0; })
#define fscrypt_prepare_symlink(d, s, l, m, dl)	({ (void)(d); (void)(s); (void)(l); (void)(m); (void)(dl); 0; })
#define fscrypt_encrypt_symlink(i, s, l, d)	({ (void)(i); (void)(s); (void)(l); (void)(d); 0; })
#define fscrypt_prepare_link(o, d, n)		({ (void)(o); (void)(d); (void)(n); 0; })
#define fscrypt_prepare_rename(od, ode, nd, nde, f) ({ (void)(od); (void)(ode); (void)(nd); (void)(nde); (void)(f); 0; })

/* fscrypt_name - stub structure for encrypted filenames */
struct fscrypt_name {
	const struct qstr *usr_fname;
	struct fscrypt_str disk_name;
	u32 hash;
	u32 minor_hash;
	bool is_nokey_name;
};

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
#define iget_locked(sb, ino)		((struct inode *)NULL)
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

/* fscrypt_str - encrypted filename string */
#define FSTR_INIT(n, l)		{ .name = (n), .len = (l) }

/* fscrypt directory operations */
#define fscrypt_prepare_readdir(i)		({ (void)(i); 0; })
#define fscrypt_fname_alloc_buffer(len, buf)	({ (void)(len); (void)(buf); 0; })
#define fscrypt_fname_free_buffer(buf)		do { (void)(buf); } while (0)
#define fscrypt_fname_disk_to_usr(i, h1, h2, d, u) ({ (void)(i); (void)(h1); (void)(h2); (void)(d); (void)(u); 0; })

/* Readahead operations */
#define ra_has_index(ra, idx)			({ (void)(ra); (void)(idx); 0; })
#define page_cache_sync_readahead(m, ra, f, i, n) do { } while (0)

/* Inode version operations */
#define inode_eq_iversion(i, v)			({ (void)(i); (void)(v); 1; })
#define inode_query_iversion(i)			({ (void)(i); 0ULL; })

/* Directory context operations */
#define dir_emit(ctx, name, len, ino, type)	({ (void)(ctx); (void)(name); (void)(len); (void)(ino); (void)(type); 1; })
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

/* fscrypt symlink stubs */
#define fscrypt_get_symlink(i, c, m, d)	({ (void)(i); (void)(c); (void)(m); (void)(d); ERR_PTR(-EOPNOTSUPP); })
#define fscrypt_symlink_getattr(p, s)	({ (void)(p); (void)(s); 0; })

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
#define end_buffer_read_sync	NULL
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

/* __kernel_fsid_t - must be before kstatfs */
typedef struct {
	int val[2];
} __kernel_fsid_t;

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
#define seq_printf(m, fmt, ...)		do { } while (0)
#define seq_puts(m, s)			do { } while (0)
#define seq_putc(m, c)			do { } while (0)
#define seq_escape(m, s, esc)		do { } while (0)

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

/* crc16 */
#define crc16(crc, buf, len)		(0)

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

/* Request flags */
#define REQ_OP_WRITE			1
#define REQ_SYNC			(1 << 0)
#define REQ_FUA				(1 << 1)

/* blk_holder_ops for block device */
struct blk_holder_ops {
	void (*mark_dead)(struct block_device *, bool);
};
static const struct blk_holder_ops fs_holder_ops;

/* end_buffer_write_sync */
#define end_buffer_write_sync		NULL

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
void submit_bh(int op_flags, struct buffer_head *bh);
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

/* Percpu rwsem - declarations for stub.c */
int percpu_init_rwsem(struct percpu_rw_semaphore *sem);
void percpu_free_rwsem(struct percpu_rw_semaphore *sem);

/* Block device sync - declarations for stub.c */
void sync_blockdev(struct block_device *bdev);
void invalidate_bdev(struct block_device *bdev);

/* Kobject - declarations for stub.c */
void kobject_put(struct kobject *kobj);
void wait_for_completion(struct completion *comp);

/* DAX - declaration for stub.c */
void fs_put_dax(void *dax, void *holder);

/* fscrypt - declarations for stub.c */
void fscrypt_free_dummy_policy(struct fscrypt_dummy_policy *policy);
int fscrypt_drop_inode(struct inode *inode);
void fscrypt_free_inode(struct inode *inode);

/* Inode allocation - declaration for stub.c */
void *alloc_inode_sb(struct super_block *sb, struct kmem_cache *cache,
		     gfp_t gfp);
void inode_set_iversion(struct inode *inode, u64 version);
int inode_generic_drop(struct inode *inode);

/* Lock init - declaration for stub.c */
void rwlock_init(rwlock_t *lock);

/* Trace stubs */
#define trace_ext4_drop_inode(i, d)		do { } while (0)
#define trace_ext4_nfs_commit_metadata(i)	do { } while (0)
#define trace_ext4_prefetch_bitmaps(...)	do { } while (0)
#define trace_ext4_lazy_itable_init(...)	do { } while (0)

/* slab usercopy - use regular kmem_cache_create */
#define kmem_cache_create_usercopy(n, sz, al, fl, uo, us, c) \
	kmem_cache_create(n, sz, al, fl, c)

/* Inode buffer operations */
#define invalidate_inode_buffers(i)	do { } while (0)
#define clear_inode(i)			do { } while (0)

/* fscrypt/fsverity additional stubs */
#define fscrypt_put_encryption_info(i)	do { } while (0)
#define fsverity_cleanup_inode(i)	do { } while (0)
#define fscrypt_parse_test_dummy_encryption(p, d) ({ (void)(p); (void)(d); 0; })

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

/* fscrypt/fsverity declarations for stub.c */
int fscrypt_is_dummy_policy_set(const struct fscrypt_dummy_policy *policy);
int fscrypt_dummy_policies_equal(const struct fscrypt_dummy_policy *p1,
				 const struct fscrypt_dummy_policy *p2);
void fscrypt_show_test_dummy_encryption(struct seq_file *seq, char sep,
					struct super_block *sb);

/* Memory allocation - declarations for stub.c */
void *kvzalloc(size_t size, gfp_t flags);
#define kvmalloc(size, flags)	kvzalloc(size, flags)
unsigned long roundup_pow_of_two(unsigned long n);

/* Atomic operations - declarations for stub.c */
void atomic_add(int val, atomic_t *v);
void atomic64_add(s64 val, atomic64_t *v);

/* Power of 2 check - declaration for stub.c */
int is_power_of_2(unsigned long n);

/* Time operations */
#define ktime_get_ns()			(0ULL)
#define nsecs_to_jiffies(ns)		((ns) / (NSEC_PER_SEC / HZ))

/* Superblock write operations */
#define sb_start_write_trylock(sb)	({ (void)(sb); 1; })
#define sb_end_write(sb)		do { } while (0)

/* Scheduler stubs */
#define schedule_timeout_interruptible(t)	do { } while (0)

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

/* Superblock identity stubs */
#define super_set_uuid(sb, uuid, len)		do { } while (0)
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

#define mb_cache_create(bits)			((struct mb_cache *)NULL)
#define mb_cache_destroy(cache)			do { (void)(cache); } while (0)
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

/* Quota - declaration for stub.c */
int dquot_suspend(void *sb, int flags);
int dquot_alloc_space_nodirty(struct inode *inode, loff_t size);
void dquot_free_space_nodirty(struct inode *inode, loff_t size);
int dquot_alloc_block(struct inode *inode, loff_t nr);
void dquot_free_block(struct inode *inode, loff_t nr);

/* Block device file operations - stubs */
#define set_blocksize(f, size)		({ (void)(f); (void)(size); 0; })
#define __bread(bdev, block, size)	({ (void)(bdev); (void)(block); (void)(size); (struct buffer_head *)NULL; })

/* Trace stubs for super.c */
#define trace_ext4_sync_fs(sb, wait)	do { (void)(sb); (void)(wait); } while (0)

/* Workqueue operations - stubs */
#define flush_workqueue(wq)		do { (void)(wq); } while (0)

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

/* Per-CPU stubs - U-Boot is single-threaded */
#define DEFINE_PER_CPU(type, name)	type name
#define per_cpu(var, cpu)		(var)
#define per_cpu_ptr(ptr, cpu)		(ptr)
#define this_cpu_inc(var)		((var)++)
#define this_cpu_read(var)		(var)
#define for_each_possible_cpu(cpu)	for ((cpu) = 0; (cpu) < 1; (cpu)++)
#define smp_processor_id()		0

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

/* Atomic64 operations */
#define atomic64_inc(v)			do { (void)(v); } while (0)
#define atomic64_add(i, v)		do { (void)(i); (void)(v); } while (0)

/* CPU cycle counter stub */
#define get_cycles()			(0ULL)

/* folio_address - get virtual address of folio data */
#undef folio_address
#define folio_address(folio)		((folio)->data)

/* Trace stubs for mballoc.c */
#define trace_ext4_mb_bitmap_load(sb, group) \
	do { (void)(sb); (void)(group); } while (0)
#define trace_ext4_mb_buddy_bitmap_load(sb, group) \
	do { (void)(sb); (void)(group); } while (0)
#define trace_ext4_mballoc_alloc(ac) \
	do { (void)(ac); } while (0)
#define trace_ext4_mballoc_prealloc(ac) \
	do { (void)(ac); } while (0)
#define trace_ext4_mballoc_discard(sb, inode, group, start, len) \
	do { (void)(sb); (void)(inode); (void)(group); (void)(start); (void)(len); } while (0)
#define trace_ext4_mballoc_free(sb, inode, group, start, len) \
	do { (void)(sb); (void)(inode); (void)(group); (void)(start); (void)(len); } while (0)
#define trace_ext4_mb_release_inode_pa(pa, block, count) \
	do { (void)(pa); (void)(block); (void)(count); } while (0)
#define trace_ext4_mb_release_group_pa(sb, pa) \
	do { (void)(sb); (void)(pa); } while (0)
#define trace_ext4_mb_new_inode_pa(ac, pa) \
	do { (void)(ac); (void)(pa); } while (0)
#define trace_ext4_mb_new_group_pa(ac, pa) \
	do { (void)(ac); (void)(pa); } while (0)

/* sb_end_intwrite stub */
#define sb_end_intwrite(sb)		do { (void)(sb); } while (0)

/* WARN_RATELIMIT - just evaluate condition, no warning in U-Boot */
#define WARN_RATELIMIT(condition, ...) (condition)

/* folio_get - increment folio refcount (no-op in U-Boot) */
#define folio_get(f)			do { (void)(f); } while (0)

/* array_index_nospec - bounds checking without speculation (no-op in U-Boot) */
#define array_index_nospec(index, size) (index)

/* atomic_inc_return - increment and return new value */
static inline int atomic_inc_return(atomic_t *v)
{
	return ++(v->counter);
}

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

/* order_base_2 - log2 rounded up */
#define order_base_2(n)			ilog2(roundup_pow_of_two(n))

/* num_possible_cpus - number of possible CPUs (always 1 in U-Boot) */
#define num_possible_cpus()		1

/* Per-CPU allocation stubs */
#define alloc_percpu(type)		((type *)kzalloc(sizeof(type), GFP_KERNEL))
#define free_percpu(ptr)		kfree(ptr)

/* Block device properties */
#define bdev_nonrot(bdev)		({ (void)(bdev); 0; })

/* Trace stub for discard */
#define trace_ext4_discard_blocks(sb, blk, count) \
	do { (void)(sb); (void)(blk); (void)(count); } while (0)

/* sb_issue_discard - issue discard request (no-op in U-Boot) */
#define sb_issue_discard(sb, sector, nr_sects, gfp, flags) \
	({ (void)(sb); (void)(sector); (void)(nr_sects); (void)(gfp); (void)(flags); 0; })

/* Atomic operations */
#define atomic_sub(i, v)		((v)->counter -= (i))
#define atomic64_sub(i, v)		((v)->counter -= (i))
#define atomic_dec_and_test(v)		(--((v)->counter) == 0)

/* RCU list operations - use regular list operations in U-Boot */
#define list_for_each_entry_rcu(pos, head, member, ...) \
	list_for_each_entry(pos, head, member)
#define list_del_rcu(entry)		list_del(entry)
#define list_add_rcu(new, head)		list_add(new, head)
#define list_add_tail_rcu(new, head)	list_add_tail(new, head)
#define rcu_read_lock()			do { } while (0)
#define rcu_read_unlock()		do { } while (0)
#define synchronize_rcu()		do { } while (0)
#define rcu_assign_pointer(p, v)	((p) = (v))
#define rcu_dereference(p)		(p)

/* raw_cpu_ptr - get pointer to per-CPU data for current CPU */
#define raw_cpu_ptr(ptr)		(ptr)

/* Scheduler stubs */
#define schedule_timeout_uninterruptible(t) do { } while (0)
#define need_resched()			(0)

/* Trace stubs for mballoc.c */
#define trace_ext4_discard_preallocations(inode, cnt) \
	do { (void)(inode); (void)(cnt); } while (0)
#define trace_ext4_mb_discard_preallocations(sb, needed) \
	do { (void)(sb); (void)(needed); } while (0)
#define trace_ext4_request_blocks(ar) \
	do { (void)(ar); } while (0)
#define trace_ext4_allocate_blocks(ar, block) \
	do { (void)(ar); (void)(block); } while (0)
#define trace_ext4_free_blocks(inode, block, count, flags) \
	do { (void)(inode); (void)(block); (void)(count); (void)(flags); } while (0)
#define trace_ext4_trim_extent(sb, group, start, count) \
	do { (void)(sb); (void)(group); (void)(start); (void)(count); } while (0)
#define trace_ext4_trim_all_free(sb, group, start, max) \
	do { (void)(sb); (void)(group); (void)(start); (void)(max); } while (0)

/* Block device operations */
#define sb_find_get_block_nonatomic(sb, block) \
	({ (void)(sb); (void)(block); (struct buffer_head *)NULL; })
#define bdev_discard_granularity(bdev) \
	({ (void)(bdev); 0U; })

#endif /* __EXT4_UBOOT_H__ */

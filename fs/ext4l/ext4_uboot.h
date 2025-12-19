/* SPDX-License-Identifier: GPL-2.0 */
/*
 * U-Boot compatibility header for ext4l filesystem
 *
 * This provides minimal definitions to allow Linux ext4 code to compile
 * in U-Boot.
 */

#ifndef __EXT4_UBOOT_H__
#define __EXT4_UBOOT_H__

#include <linux/types.h>
#include <linux/bitops.h>
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

/* percpu_counter - stub */
struct percpu_counter {
	long count;
};

static inline long percpu_counter_sum(struct percpu_counter *fbc)
{
	return fbc->count;
}

/* name_snapshot - stub */
struct name_snapshot {
	const char *name;
};

/* Project ID type */
typedef struct { unsigned int val; } kprojid_t;

#define make_kprojid(ns, id)	((kprojid_t){ .val = (id) })
#define from_kprojid(ns, kprojid)	((kprojid).val)

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

/* BUG_ON / BUG - stubs */
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
#define IS_DIRSYNC(inode)			(0)

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
#define blkdev_issue_flush(bdev)		do { (void)(bdev); } while (0)

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

/* inode operations - stubs */
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
};

/* vm_fault_t - stub */
typedef unsigned int vm_fault_t;

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
struct mnt_idmap;

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

/* super_block - minimal stub */
struct super_block {
	void *s_fs_info;
	unsigned long s_blocksize;
	unsigned char s_blocksize_bits;
	unsigned long s_magic;
	loff_t s_maxbytes;
	unsigned long s_flags;
	struct rw_semaphore s_umount;
	struct sb_writers s_writers;
	struct block_device *s_bdev;
	const char *s_id;
	struct dentry *s_root;
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

/* struct path - filesystem path */
struct path {
	struct vfsmount *mnt;
	struct dentry *dentry;
};

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

static inline void simple_inode_init_ts(struct inode *inode)
{
	struct timespec64 ts = { .tv_sec = 0, .tv_nsec = 0 };

	inode->i_atime = ts;
	inode->i_mtime = ts;
	inode->i_ctime = ts;
}

#define QSTR_INIT(n, l) { .name = (const unsigned char *)(n), .len = (l) }

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
#define inode_dio_wait(inode)		do { } while (0)

/* File operations */
#define file_modified(file)		({ (void)(file); 0; })

/* Filemap operations */
#define filemap_invalidate_lock(m)	do { } while (0)
#define filemap_invalidate_unlock(m)	do { } while (0)
#define filemap_write_and_wait_range(m, s, e) ({ (void)(m); (void)(s); (void)(e); 0; })
#define truncate_pagecache(i, s)	do { } while (0)
#define pagecache_isize_extended(i, f, t) do { } while (0)

/* Inode time/size operations */
#define inode_newsize_ok(i, s)		({ (void)(i); (void)(s); 0; })
#define inode_set_ctime_current(i)	({ (void)(i); (struct timespec64){}; })
#define inode_set_mtime_to_ts(i, ts)	({ (void)(i); (ts); })
#define i_blocksize(i)			(1UL << (i)->i_blkbits)

/* IS_SYNC macro */
#define IS_SYNC(inode)			(0)

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
	struct address_space *mapping;
	unsigned long index;
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

/* DAX stubs - DAX not supported in U-Boot */
#define IS_DAX(inode)				(0)
#define dax_break_layout_final(inode)		do { } while (0)
#define dax_writeback_mapping_range(m, bd, wb)	({ (void)(m); (void)(bd); (void)(wb); 0; })
#define dax_zero_range(i, p, l, d, op)		({ (void)(i); (void)(p); (void)(l); (void)(d); (void)(op); -EOPNOTSUPP; })
#define dax_break_layout_inode(i, m)		({ (void)(i); (void)(m); 0; })

/* Superblock freezing stubs */
#define sb_start_intwrite(sb)			do { (void)(sb); } while (0)
#define sb_end_intwrite(sb)			do { (void)(sb); } while (0)
#define sb_start_pagefault(sb)			do { (void)(sb); } while (0)
#define sb_end_pagefault(sb)			do { (void)(sb); } while (0)

/* Inode I/O list management */
#define inode_io_list_del(inode)		do { } while (0)
#define inode_is_open_for_write(i)		(0)
#define inode_is_dirtytime_only(i)		(0)

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

/* file open helper */
#define simple_open(i, f)		({ (void)(i); (void)(f); 0; })

#endif /* __EXT4_UBOOT_H__ */

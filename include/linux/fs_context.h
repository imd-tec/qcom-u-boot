/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Filesystem context stubs for U-Boot
 *
 * Based on Linux fs_context.h - U-Boot doesn't have real mount contexts,
 * so these are stubs for compilation.
 */
#ifndef _LINUX_FS_CONTEXT_H
#define _LINUX_FS_CONTEXT_H

#include <linux/types.h>
#include <linux/list.h>

/* Forward declarations */
struct dentry;
struct fs_parameter;
struct module;
struct super_block;
struct user_namespace;

/**
 * enum fs_context_purpose - what the context is for
 * @FS_CONTEXT_FOR_MOUNT: New superblock for explicit mount
 * @FS_CONTEXT_FOR_SUBMOUNT: New superblock for automatic submount
 * @FS_CONTEXT_FOR_RECONFIGURE: Superblock reconfiguration (remount)
 */
enum fs_context_purpose {
	FS_CONTEXT_FOR_MOUNT,
	FS_CONTEXT_FOR_SUBMOUNT,
	FS_CONTEXT_FOR_RECONFIGURE,
};

/**
 * enum fs_value_type - type of parameter value
 * @fs_value_is_undefined: Value not specified
 * @fs_value_is_flag: Value not given a value
 * @fs_value_is_string: Value is a string
 * @fs_value_is_blob: Value is a binary blob
 * @fs_value_is_filename: Value is a filename + dirfd
 * @fs_value_is_file: Value is a file pointer
 */
enum fs_value_type {
	fs_value_is_undefined,
	fs_value_is_flag,
	fs_value_is_string,
	fs_value_is_blob,
	fs_value_is_filename,
	fs_value_is_file,
};

struct fs_context;

/**
 * struct fs_context_operations - filesystem context operations
 * @parse_param: parse a single parameter
 * @get_tree: get the superblock
 * @reconfigure: reconfigure the superblock
 * @free: free the context
 */
struct fs_context_operations {
	int (*parse_param)(struct fs_context *, struct fs_parameter *);
	int (*get_tree)(struct fs_context *);
	int (*reconfigure)(struct fs_context *);
	void (*free)(struct fs_context *);
};

/**
 * struct file_system_type - filesystem type descriptor
 * @owner: module owner
 * @name: filesystem name
 * @init_fs_context: initialise a filesystem context
 * @parameters: mount parameter specification
 * @kill_sb: destroy a superblock
 * @fs_flags: filesystem flags
 * @fs_supers: list of superblocks of this type
 */
struct file_system_type {
	struct module *owner;
	const char *name;
	int (*init_fs_context)(struct fs_context *);
	const struct fs_parameter_spec *parameters;
	void (*kill_sb)(struct super_block *);
	int fs_flags;
	struct list_head fs_supers;
};

/* Filesystem type flags */
#define FS_REQUIRES_DEV		1
#define FS_BINARY_MOUNTDATA	2
#define FS_HAS_SUBTYPE		4
#define FS_USERNS_MOUNT		8
#define FS_DISALLOW_NOTIFY_PERM	16
#define FS_ALLOW_IDMAP		32

/**
 * struct fs_context - filesystem context for mount/reconfigure
 * @ops: operations for this context
 * @fs_type: filesystem type
 * @fs_private: filesystem private data
 * @root: root dentry (for reconfigure)
 * @user_ns: user namespace for this mount
 * @s_fs_info: proposed s_fs_info
 * @sb_flags: proposed superblock flags
 * @sb_flags_mask: superblock flags that changed
 * @lsm_flags: LSM flags
 * @purpose: what this context is for
 * @sloppy: permit unrecognised options
 * @silent: suppress mount errors
 */
struct fs_context {
	const struct fs_context_operations *ops;
	struct file_system_type *fs_type;
	void *fs_private;
	struct dentry *root;
	struct user_namespace *user_ns;
	void *s_fs_info;
	unsigned int sb_flags;
	unsigned int sb_flags_mask;
	unsigned int lsm_flags;
	enum fs_context_purpose purpose;
	bool sloppy;
	bool silent;
};

/**
 * struct fs_parameter - filesystem mount parameter
 * @key: parameter name
 * @type: value type
 * @size: size of value
 * @dirfd: directory fd for filename values
 * @string: string value
 * @boolean: boolean value
 * @integer: integer value
 */
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

/* get_tree helpers - stubs */
#define get_tree_bdev(fc, fill_super)	({ (void)(fc); (void)(fill_super); -ENODEV; })
#define get_tree_nodev(fc, fill_super)	({ (void)(fc); (void)(fill_super); -ENODEV; })

/* kill_sb helpers - stubs */
#define kill_block_super(sb)		do { } while (0)

#endif /* _LINUX_FS_CONTEXT_H */

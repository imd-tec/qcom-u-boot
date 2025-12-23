// SPDX-License-Identifier: GPL-2.0+
/*
 * U-Boot interface for ext4l filesystem (Linux port)
 *
 * Copyright 2025 Canonical Ltd
 * Written by Simon Glass <simon.glass@canonical.com>
 *
 * This provides the interface between U-Boot's filesystem layer and
 * the ext4l driver.
 */

#include <blk.h>
#include <env.h>
#include <fs.h>
#include <membuf.h>
#include <part.h>
#include <malloc.h>
#include <linux/errno.h>
#include <linux/jbd2.h>
#include <linux/types.h>

#include "ext4_uboot.h"
#include "ext4.h"

/* Message buffer size */
#define EXT4L_MSG_BUF_SIZE	4096

/* Global state */
static struct blk_desc *ext4l_dev_desc;
static struct disk_partition ext4l_part;

/* Global block device tracking for buffer I/O */
static struct blk_desc *ext4l_blk_dev;
static struct disk_partition ext4l_partition;
static int ext4l_mounted;

/* Count of open directory streams (prevents unmount while iterating) */
static int ext4l_open_dirs;

/* Global super_block pointer for filesystem operations */
static struct super_block *ext4l_sb;

/* Message recording buffer */
static struct membuf ext4l_msg_buf;
static char ext4l_msg_data[EXT4L_MSG_BUF_SIZE];

/**
 * ext4l_get_blk_dev() - Get the current block device
 *
 * Return: Block device descriptor or NULL if not mounted
 */
struct blk_desc *ext4l_get_blk_dev(void)
{
	if (!ext4l_mounted)
		return NULL;
	return ext4l_blk_dev;
}

/**
 * ext4l_get_partition() - Get the current partition info
 *
 * Return: Partition info pointer
 */
struct disk_partition *ext4l_get_partition(void)
{
	return &ext4l_partition;
}

/**
 * ext4l_get_uuid() - Get the filesystem UUID
 *
 * @uuid: Buffer to receive the 16-byte UUID
 * Return: 0 on success, -ENODEV if not mounted
 */
int ext4l_get_uuid(u8 *uuid)
{
	if (!ext4l_sb)
		return -ENODEV;
	memcpy(uuid, ext4l_sb->s_uuid.b, 16);
	return 0;
}

/**
 * ext4l_set_blk_dev() - Set the block device for ext4l operations
 *
 * @blk_dev: Block device descriptor
 * @partition: Partition info (can be NULL for whole disk)
 */
void ext4l_set_blk_dev(struct blk_desc *blk_dev, struct disk_partition *partition)
{
	ext4l_blk_dev = blk_dev;
	if (partition)
		memcpy(&ext4l_partition, partition, sizeof(struct disk_partition));
	else
		memset(&ext4l_partition, 0, sizeof(struct disk_partition));
	ext4l_mounted = 1;
}

/**
 * ext4l_clear_blk_dev() - Clear block device (unmount)
 */
void ext4l_clear_blk_dev(void)
{
	/* Clear buffer cache before unmounting */
	bh_cache_clear();

	ext4l_blk_dev = NULL;
	ext4l_mounted = 0;
}

/**
 * ext4l_msg_init() - Initialize the message buffer
 */
static void ext4l_msg_init(void)
{
	membuf_init(&ext4l_msg_buf, ext4l_msg_data, EXT4L_MSG_BUF_SIZE);
}

/**
 * ext4l_record_msg() - Record a message in the buffer
 *
 * @msg: Message string to record
 * @len: Length of message
 */
void ext4l_record_msg(const char *msg, int len)
{
	membuf_put(&ext4l_msg_buf, msg, len);
}

/**
 * ext4l_get_msg_buf() - Get the message buffer
 *
 * Return: Pointer to the message buffer
 */
struct membuf *ext4l_get_msg_buf(void)
{
	return &ext4l_msg_buf;
}

/**
 * ext4l_print_msgs() - Print all recorded messages
 *
 * Prints the contents of the message buffer to the console.
 */
static void ext4l_print_msgs(void)
{
	char *data;
	int len;

	while ((len = membuf_getraw(&ext4l_msg_buf, 80, true, &data)) > 0)
		printf("%.*s", len, data);
}

int ext4l_probe(struct blk_desc *fs_dev_desc,
		struct disk_partition *fs_partition)
{
	struct ext4_fs_context *ctx;
	struct super_block *sb;
	struct fs_context *fc;
	loff_t part_offset;
	__le16 *magic;
	u8 *buf;
	int ret;

	if (!fs_dev_desc)
		return -EINVAL;

	/* Initialise message buffer for recording ext4 messages */
	ext4l_msg_init();

	/* Initialise CRC32C table for checksum verification */
	ext4l_crc32c_init();

	/* Initialise journal subsystem if enabled */
	if (IS_ENABLED(CONFIG_EXT4_JOURNAL)) {
		ret = jbd2_journal_init_global();
		if (ret)
			return ret;
	}

	/* Initialise multi-block allocator for write support */
	if (IS_ENABLED(CONFIG_EXT4_WRITE)) {
		ret = ext4_init_mballoc();
		if (ret)
			return ret;
	}

	/* Initialise extent status cache */
	ret = ext4_init_es();
	if (ret)
		return ret;

	/* Initialise system zone for block validity checking */
	ret = ext4_init_system_zone();
	if (ret)
		goto err_exit_es;

	/* Allocate super_block */
	sb = kzalloc(sizeof(struct super_block), GFP_KERNEL);
	if (!sb) {
		ret = -ENOMEM;
		goto err_exit_es;
	}

	/* Allocate block_device */
	sb->s_bdev = kzalloc(sizeof(struct block_device), GFP_KERNEL);
	if (!sb->s_bdev) {
		ret = -ENOMEM;
		goto err_free_sb;
	}

	sb->s_bdev->bd_mapping = kzalloc(sizeof(struct address_space), GFP_KERNEL);
	if (!sb->s_bdev->bd_mapping) {
		ret = -ENOMEM;
		goto err_free_bdev;
	}

	/* Initialise super_block fields */
	sb->s_bdev->bd_super = sb;
	sb->s_blocksize = 1024;
	sb->s_blocksize_bits = 10;
	snprintf(sb->s_id, sizeof(sb->s_id), "ext4l_mmc%d",
		 fs_dev_desc->devnum);
	sb->s_flags = 0;
	sb->s_fs_info = NULL;

	/* Allocate fs_context */
	fc = kzalloc(sizeof(struct fs_context), GFP_KERNEL);
	if (!fc) {
		ret = -ENOMEM;
		goto err_free_mapping;
	}

	/* Allocate ext4_fs_context */
	ctx = kzalloc(sizeof(struct ext4_fs_context), GFP_KERNEL);
	if (!ctx) {
		ret = -ENOMEM;
		goto err_free_fc;
	}

	/* Initialise fs_context fields */
	fc->fs_private = ctx;
	fc->sb_flags |= SB_I_VERSION;
	fc->root = (struct dentry *)sb;	/* Hack: store sb for ext4_fill_super */

	buf = malloc(BLOCK_SIZE + 512);
	if (!buf) {
		ret = -ENOMEM;
		goto err_free_ctx;
	}

	/* Calculate partition offset in bytes */
	part_offset = fs_partition ? (loff_t)fs_partition->start * fs_dev_desc->blksz : 0;

	/* Read sectors containing the superblock */
	if (blk_dread(fs_dev_desc,
		      (part_offset + BLOCK_SIZE) / fs_dev_desc->blksz,
		      2, buf) != 2) {
		ret = -EIO;
		goto err_free_buf;
	}

	/* Check magic number within superblock */
	magic = (__le16 *)(buf + (BLOCK_SIZE % fs_dev_desc->blksz) +
			   offsetof(struct ext4_super_block, s_magic));
	if (le16_to_cpu(*magic) != EXT4_SUPER_MAGIC) {
		ret = -EINVAL;
		goto err_free_buf;
	}

	free(buf);

	/* Save device info for later operations */
	ext4l_dev_desc = fs_dev_desc;
	if (fs_partition)
		memcpy(&ext4l_part, fs_partition, sizeof(ext4l_part));

	/* Set block device for buffer I/O */
	ext4l_set_blk_dev(fs_dev_desc, fs_partition);

	/* Mount the filesystem */
	ret = ext4_fill_super(sb, fc);
	if (ret) {
		printf("ext4l: ext4_fill_super failed: %d\n", ret);
		goto err_free_ctx;
	}

	/* Store super_block for later operations */
	ext4l_sb = sb;

	/* Print messages if ext4l_msgs environment variable is set */
	if (env_get_yesno("ext4l_msgs") == 1)
		ext4l_print_msgs();

	return 0;

err_free_buf:
	free(buf);
err_free_ctx:
	kfree(ctx);
err_free_fc:
	kfree(fc);
err_free_mapping:
	kfree(sb->s_bdev->bd_mapping);
err_free_bdev:
	kfree(sb->s_bdev);
err_free_sb:
	kfree(sb);
err_exit_es:
	ext4_exit_es();
	return ret;
}

/**
 * ext4l_read_symlink() - Read the target of a symlink inode
 *
 * @inode: Symlink inode
 * @target: Buffer to store target
 * @max_len: Maximum length of target buffer
 * Return: Length of target on success, negative on error
 */
static int ext4l_read_symlink(struct inode *inode, char *target, size_t max_len)
{
	struct buffer_head *bh;
	size_t len;

	if (!S_ISLNK(inode->i_mode))
		return -EINVAL;

	if (ext4_inode_is_fast_symlink(inode)) {
		/* Fast symlink: target stored in i_data */
		len = inode->i_size;
		if (len >= max_len)
			len = max_len - 1;
		memcpy(target, EXT4_I(inode)->i_data, len);
		target[len] = '\0';
		return len;
	}

	/* Slow symlink: target stored in data block */
	bh = ext4_bread(NULL, inode, 0, 0);
	if (IS_ERR(bh))
		return PTR_ERR(bh);
	if (!bh)
		return -EIO;

	len = inode->i_size;
	if (len >= max_len)
		len = max_len - 1;
	memcpy(target, bh->b_data, len);
	target[len] = '\0';
	brelse(bh);

	return len;
}

/* Forward declaration for recursive resolution */
static int ext4l_resolve_path_internal(const char *path, struct inode **inodep,
				       int depth);

/**
 * ext4l_resolve_path() - Resolve path to inode
 *
 * @path: Path to resolve
 * @inodep: Output inode pointer
 * Return: 0 on success, negative on error
 */
static int ext4l_resolve_path(const char *path, struct inode **inodep)
{
	return ext4l_resolve_path_internal(path, inodep, 0);
}

/**
 * ext4l_resolve_path_internal() - Resolve path with symlink following
 *
 * @path: Path to resolve
 * @inodep: Output inode pointer
 * @depth: Current recursion depth (for symlink loop detection)
 * Return: 0 on success, negative on error
 */
static int ext4l_resolve_path_internal(const char *path, struct inode **inodep,
				       int depth)
{
	struct inode *dir;
	struct dentry *dentry, *result;
	char *path_copy, *component, *next_component;
	int ret;

	/* Prevent symlink loops */
	if (depth > 8)
		return -ELOOP;

	if (!ext4l_mounted) {
		ext4_debug("ext4l_resolve_path: filesystem not mounted\n");
		return -ENODEV;
	}

	dir = ext4l_sb->s_root->d_inode;

	if (!path || !*path || (strcmp(path, "/") == 0)) {
		*inodep = dir;
		return 0;
	}

	path_copy = strdup(path);
	if (!path_copy)
		return -ENOMEM;

	component = path_copy;
	/* Skip leading slash */
	if (*component == '/')
		component++;

	while (component && *component) {
		next_component = strchr(component, '/');
		if (next_component) {
			*next_component = '\0';
			next_component++;
		}

		if (!*component) {
			component = next_component;
			continue;
		}

		/* Handle special directory entries */
		if (strcmp(component, ".") == 0) {
			component = next_component;
			continue;
		}
		if (strcmp(component, "..") == 0) {
			/* Parent directory - look up ".." entry */
			dentry = kzalloc(sizeof(struct dentry), GFP_KERNEL);
			if (!dentry) {
				free(path_copy);
				return -ENOMEM;
			}
			dentry->d_name.name = "..";
			dentry->d_name.len = 2;
			dentry->d_sb = ext4l_sb;
			dentry->d_parent = NULL;

			result = ext4_lookup(dir, dentry, 0);
			if (IS_ERR(result)) {
				kfree(dentry);
				free(path_copy);
				return PTR_ERR(result);
			}
			if (result && result->d_inode) {
				dir = result->d_inode;
				if (result != dentry)
					kfree(dentry);
				kfree(result);
			} else if (dentry->d_inode) {
				dir = dentry->d_inode;
				kfree(dentry);
			} else {
				/* ".." not found - stay at root */
				kfree(dentry);
				if (result && result != dentry)
					kfree(result);
			}
			component = next_component;
			continue;
		}

		dentry = kzalloc(sizeof(struct dentry), GFP_KERNEL);
		if (!dentry) {
			free(path_copy);
			return -ENOMEM;
		}

		dentry->d_name.name = component;
		dentry->d_name.len = strlen(component);
		dentry->d_sb = ext4l_sb;
		dentry->d_parent = NULL;

		result = ext4_lookup(dir, dentry, 0);

		if (IS_ERR(result)) {
			kfree(dentry);
			free(path_copy);
			return PTR_ERR(result);
		}

		if (result) {
			if (!result->d_inode) {
				if (result != dentry)
					kfree(dentry);
				kfree(result);
				free(path_copy);
				return -ENOENT;
			}
			dir = result->d_inode;
			if (result != dentry)
				kfree(dentry);
			kfree(result);
		} else {
			if (!dentry->d_inode) {
				kfree(dentry);
				free(path_copy);
				return -ENOENT;
			}
			dir = dentry->d_inode;
			kfree(dentry);
		}

		if (!dir) {
			free(path_copy);
			return -ENOENT;
		}

		/* Check if this is a symlink and follow it */
		if (S_ISLNK(dir->i_mode)) {
			char link_target[256];
			char *new_path;

			ret = ext4l_read_symlink(dir, link_target,
						 sizeof(link_target));
			if (ret < 0) {
				free(path_copy);
				return ret;
			}

			/* Build new path: link_target + remaining path */
			if (next_component && *next_component) {
				size_t target_len = strlen(link_target);
				size_t remaining_len = strlen(next_component);

				new_path = malloc(target_len + 1 +
						  remaining_len + 1);
				if (!new_path) {
					free(path_copy);
					return -ENOMEM;
				}
				strcpy(new_path, link_target);
				strcat(new_path, "/");
				strcat(new_path, next_component);
			} else {
				new_path = strdup(link_target);
				if (!new_path) {
					free(path_copy);
					return -ENOMEM;
				}
			}

			free(path_copy);

			/* Recursively resolve the new path */
			ret = ext4l_resolve_path_internal(new_path, inodep,
							  depth + 1);
			free(new_path);
			return ret;
		}

		component = next_component;
	}

	free(path_copy);
	*inodep = dir;
	return 0;
}

/**
 * ext4l_dir_actor() - Directory entry callback for ext4_readdir
 *
 * @ctx: Directory context
 * @name: Entry name
 * @namelen: Length of name
 * @offset: Directory offset
 * @ino: Inode number
 * @d_type: Entry type
 * Return: 0 to continue iteration
 */
static int ext4l_dir_actor(struct dir_context *ctx, const char *name,
			   int namelen, loff_t offset, u64 ino,
			   unsigned int d_type)
{
	struct inode *inode;
	char namebuf[256];

	/* Copy the name to a null-terminated buffer */
	if (namelen >= sizeof(namebuf))
		namelen = sizeof(namebuf) - 1;
	memcpy(namebuf, name, namelen);
	namebuf[namelen] = '\0';

	/* Look up the inode to get file size */
	inode = ext4_iget(ext4l_sb, ino, 0);
	if (IS_ERR(inode)) {
		printf(" %8s   %s\n", "?", namebuf);
		return 0;
	}

	if (d_type == DT_DIR || S_ISDIR(inode->i_mode))
		printf("            %s/\n", namebuf);
	else if (d_type == DT_LNK || S_ISLNK(inode->i_mode))
		printf("    <SYM>   %s\n", namebuf);
	else
		printf(" %8lld   %s\n", (long long)inode->i_size, namebuf);

	return 0;
}

int ext4l_ls(const char *dirname)
{
	struct inode *dir;
	struct file file;
	struct dir_context ctx;
	int ret;

	ret = ext4l_resolve_path(dirname, &dir);
	if (ret)
		return ret;

	if (!S_ISDIR(dir->i_mode))
		return -ENOTDIR;

	memset(&file, 0, sizeof(file));
	file.f_inode = dir;
	file.f_mapping = dir->i_mapping;

	/* Allocate private_data for readdir */
	file.private_data = kzalloc(sizeof(struct dir_private_info), GFP_KERNEL);
	if (!file.private_data)
		return -ENOMEM;

	memset(&ctx, 0, sizeof(ctx));
	ctx.actor = ext4l_dir_actor;

	ret = ext4_readdir(&file, &ctx);

	if (file.private_data)
		ext4_htree_free_dir_info(file.private_data);

	return ret;
}

void ext4l_close(void)
{
	if (ext4l_open_dirs > 0)
		return;

	ext4l_dev_desc = NULL;
	ext4l_sb = NULL;
	ext4l_clear_blk_dev();
}

/**
 * struct ext4l_dir - ext4l directory stream state
 * @parent: base fs_dir_stream structure
 * @dirent: directory entry to return to caller
 * @dir_inode: pointer to directory inode
 * @file: file structure for ext4_readdir
 * @entry_found: flag set by actor when entry is captured
 * @last_ino: inode number of last returned entry (to skip on next call)
 * @skip_last: true if we need to skip the last_ino entry
 *
 * The filesystem stays mounted while directory streams are open (ext4l_close
 * checks ext4l_open_dirs), so we can keep direct pointers to inodes.
 */
struct ext4l_dir {
	struct fs_dir_stream parent;
	struct fs_dirent dirent;
	struct inode *dir_inode;
	struct file file;
	bool entry_found;
	u64 last_ino;
	bool skip_last;
};

/**
 * struct ext4l_readdir_ctx - Extended dir_context with back-pointer
 * @ctx: base dir_context structure (must be first)
 * @dir: pointer to ext4l_dir for state updates
 */
struct ext4l_readdir_ctx {
	struct dir_context ctx;
	struct ext4l_dir *dir;
};

/**
 * ext4l_opendir_actor() - dir_context actor that captures single entry
 *
 * This actor is called by ext4_readdir for each directory entry. It captures
 * the first entry found (skipping the previously returned entry if needed)
 * and returns non-zero to stop iteration.
 */
static int ext4l_opendir_actor(struct dir_context *ctx, const char *name,
			       int namelen, loff_t offset, u64 ino,
			       unsigned int d_type)
{
	struct ext4l_readdir_ctx *rctx;
	struct ext4l_dir *dir;
	struct fs_dirent *dent;
	struct inode *inode;

	rctx = container_of(ctx, struct ext4l_readdir_ctx, ctx);
	dir = rctx->dir;

	/*
	 * Skip the entry we returned last time. The htree code may call us
	 * with the same entry again due to its extra_fname handling.
	 */
	if (dir->skip_last && ino == dir->last_ino) {
		dir->skip_last = false;
		return 0;  /* Continue to next entry */
	}

	dent = &dir->dirent;

	/* Copy name */
	if (namelen >= FS_DIRENT_NAME_LEN)
		namelen = FS_DIRENT_NAME_LEN - 1;
	memcpy(dent->name, name, namelen);
	dent->name[namelen] = '\0';

	/* Set type based on d_type hint */
	switch (d_type) {
	case DT_DIR:
		dent->type = FS_DT_DIR;
		break;
	case DT_LNK:
		dent->type = FS_DT_LNK;
		break;
	default:
		dent->type = FS_DT_REG;
		break;
	}

	/* Look up inode to get size and other attributes */
	inode = ext4_iget(ext4l_sb, ino, 0);
	if (!IS_ERR(inode)) {
		dent->size = inode->i_size;
		/* Refine type from inode mode if needed */
		if (S_ISDIR(inode->i_mode))
			dent->type = FS_DT_DIR;
		else if (S_ISLNK(inode->i_mode))
			dent->type = FS_DT_LNK;
		else
			dent->type = FS_DT_REG;
	} else {
		dent->size = 0;
	}

	dir->entry_found = true;
	dir->last_ino = ino;

	/*
	 * Return non-zero to stop iteration after one entry.
	 * dir_emit() returns (actor(...) == 0), so:
	 *   actor returns 0 -> dir_emit returns 1 (continue)
	 *   actor returns non-zero -> dir_emit returns 0 (stop)
	 */
	return 1;
}

int ext4l_opendir(const char *filename, struct fs_dir_stream **dirsp)
{
	struct ext4l_dir *dir;
	struct inode *inode;
	int ret;

	if (!ext4l_mounted)
		return -ENODEV;

	ret = ext4l_resolve_path(filename, &inode);
	if (ret)
		return ret;

	if (!S_ISDIR(inode->i_mode))
		return -ENOTDIR;

	dir = calloc(1, sizeof(*dir));
	if (!dir)
		return -ENOMEM;

	dir->dir_inode = inode;
	dir->entry_found = false;

	/* Set up file structure for ext4_readdir */
	dir->file.f_inode = inode;
	dir->file.f_mapping = inode->i_mapping;
	dir->file.private_data = kzalloc(sizeof(struct dir_private_info),
					 GFP_KERNEL);
	if (!dir->file.private_data) {
		free(dir);
		return -ENOMEM;
	}

	/* Increment open dir count to prevent unmount */
	ext4l_open_dirs++;

	*dirsp = (struct fs_dir_stream *)dir;

	return 0;
}

int ext4l_readdir(struct fs_dir_stream *dirs, struct fs_dirent **dentp)
{
	struct ext4l_dir *dir = (struct ext4l_dir *)dirs;
	struct ext4l_readdir_ctx ctx;
	int ret;

	if (!ext4l_mounted)
		return -ENODEV;

	memset(&dir->dirent, '\0', sizeof(dir->dirent));
	dir->entry_found = false;

	/* Skip the entry we returned last time (htree may re-emit it) */
	if (dir->last_ino)
		dir->skip_last = true;

	/* Set up extended dir_context for this iteration */
	memset(&ctx, '\0', sizeof(ctx));
	ctx.ctx.actor = ext4l_opendir_actor;
	ctx.ctx.pos = dir->file.f_pos;
	ctx.dir = dir;

	ret = ext4_readdir(&dir->file, &ctx.ctx);

	/* Update file position for next call */
	dir->file.f_pos = ctx.ctx.pos;

	if (ret < 0)
		return ret;

	if (!dir->entry_found)
		return -ENOENT;

	*dentp = &dir->dirent;

	return 0;
}

void ext4l_closedir(struct fs_dir_stream *dirs)
{
	struct ext4l_dir *dir = (struct ext4l_dir *)dirs;

	if (dir) {
		if (dir->file.private_data)
			ext4_htree_free_dir_info(dir->file.private_data);
		free(dir);
	}

	/* Decrement open dir count */
	if (ext4l_open_dirs > 0)
		ext4l_open_dirs--;
}

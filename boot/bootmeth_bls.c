// SPDX-License-Identifier: GPL-2.0+
/*
 * Bootmethod for Boot Loader Specification (BLS) Type #1
 *
 * Copyright 2026 Canonical Ltd
 * Written by Simon Glass <simon.glass@canonical.com>
 *
 * This implements support for BLS Type #1 entries as defined in:
 * https://uapi-group.org/specifications/specs/boot_loader_specification/
 *
 * Supported features:
 * - Single BLS entry file at loader/entry.conf
 * - Fields: title, version, linux, options, initrd, devicetree
 * - Multiple options lines (concatenated with spaces)
 * - Multiple initrd lines (only first used, PXE limitation)
 * - FITs with #config syntax in linux field
 * - Zero-copy parsing (fields point into bootflow buffer)
 *
 * Current limitations:
 * - Single entry file only, not multiple entries in loader/entries/
 * - Only first initrd used (PXE infrastructure supports one)
 * - No devicetree-overlay support
 * - No architecture/machine-id filtering
 * - No version-based sorting
 * - No UKI/EFI support (Type #2)
 */

#define LOG_CATEGORY UCLASS_BOOTSTD

#include <asm/cache.h>
#include <bls.h>
#include <bootdev.h>
#include <bootflow.h>
#include <bootmeth.h>
#include <bootstd.h>
#include <dm.h>
#include <fs_legacy.h>
#include <malloc.h>
#include <mapmem.h>
#include <part.h>
#include <pxe_utils.h>
#include <linux/string.h>

/* Single BLS entry file to check */
#define BLS_ENTRY_FILE		"loader/entry.conf"

/**
 * struct bls_info - context information for BLS getfile callback
 *
 * @dev: Bootmethod device being used to boot
 * @bflow: Bootflow being booted
 */
struct bls_info {
	struct udevice *dev;
	struct bootflow *bflow;
};

static int bls_get_state_desc(struct udevice *dev, char *buf, int maxsize)
{
	if (IS_ENABLED(CONFIG_SANDBOX)) {
		int len;

		len = snprintf(buf, maxsize, "OK");

		return len + 1 < maxsize ? 0 : -ENOSPC;
	}

	return 0;
}

static int bls_getfile(struct pxe_context *ctx, const char *file_path,
		       ulong *addrp, ulong align, enum bootflow_img_t type,
		       ulong *sizep)
{
	struct bls_info *info = ctx->userdata;
	int ret;

	/* Allow up to 1GB */
	*sizep = 1 << 30;
	ret = bootmeth_read_file(info->dev, info->bflow, file_path, addrp,
				 align, type, sizep);
	if (ret)
		return log_msg_ret("read", ret);

	return 0;
}

static int bls_check(struct udevice *dev, struct bootflow_iter *iter)
{
	int ret;

	/* This only works on block devices */
	ret = bootflow_iter_check_blk(iter);
	if (ret)
		return log_msg_ret("blk", ret);

	return 0;
}

/**
 * bls_to_pxe_label() - Convert bootflow to PXE label for boot execution
 *
 * @bflow: Bootflow containing BLS entry and discovered images
 * @labelp: Returns allocated PXE label structure
 * Return: 0 on success, -ENOMEM if out of memory
 */
static int bls_to_pxe_label(struct bootflow *bflow,
			    struct pxe_label **labelp)
{
	struct pxe_label *label;
	struct bootflow_img *img;
	int ret;

	label = calloc(1, sizeof(*label));
	if (!label)
		return log_msg_ret("alloc", -ENOMEM);

	INIT_LIST_HEAD(&label->list);
	alist_init_struct(&label->files, struct pxe_file);

	label->menu = strdup(bflow->os_name ?: "");
	label->append = strdup(bflow->cmdline ?: "");
	if (!label->menu || !label->append) {
		ret = -ENOMEM;
		goto err;
	}

	/* Extract kernel, initrd and FDT from the bootflow images */
	alist_for_each(img, &bflow->images) {
		char **fieldp;

		if (img->type == (enum bootflow_img_t)IH_TYPE_KERNEL)
			fieldp = &label->kernel;
		else if (img->type == (enum bootflow_img_t)IH_TYPE_RAMDISK)
			fieldp = &label->initrd;
		else if (img->type == (enum bootflow_img_t)IH_TYPE_FLATDT)
			fieldp = &label->fdt;
		else
			continue;

		if (!*fieldp) {
			*fieldp = strdup(img->fname);
			if (!*fieldp) {
				ret = -ENOMEM;
				goto err;
			}
		}
	}

	*labelp = label;
	return 0;

err:
	label_destroy(label);
	return ret;
}

/**
 * bls_entry_init() - Parse entry and register images with bootflow
 *
 * @entry: Entry structure to initialize
 * @bflow: Bootflow to populate
 * @size: Size of BLS entry file in bflow->buf
 * Return: 0 on success, -ve on error
 */
static int bls_entry_init(struct bls_entry *entry, struct bootflow *bflow,
			  loff_t size)
{
	char **initrd;
	int ret;

	/* Parse BLS entry (fields point into bflow->buf) */
	ret = bls_parse_entry(bflow->buf, size, entry);
	if (ret)
		return log_msg_ret("parse", ret);

	/* Save title as os_name */
	if (entry->title) {
		bflow->os_name = strdup(entry->title);
		if (!bflow->os_name)
			return log_msg_ret("name", -ENOMEM);
	}

	/* Transfer cmdline ownership to bflow */
	if (entry->options) {
		bflow->cmdline = entry->options;
		entry->options = NULL;
	}

	/* Register discovered images (not yet loaded, addr=0) */
	if (entry->kernel) {
		if (!bootflow_img_add(bflow, entry->kernel,
				      (enum bootflow_img_t)IH_TYPE_KERNEL,
				      0, 0))
			return log_msg_ret("imk", -ENOMEM);
	}

	alist_for_each(initrd, &entry->initrds) {
		if (!bootflow_img_add(bflow, *initrd,
				      (enum bootflow_img_t)IH_TYPE_RAMDISK,
				      0, 0))
			return log_msg_ret("imi", -ENOMEM);
	}

	if (entry->devicetree) {
		if (!bootflow_img_add(bflow, entry->devicetree,
				      (enum bootflow_img_t)IH_TYPE_FLATDT,
				      0, 0))
			return log_msg_ret("imf", -ENOMEM);
	}

	return 0;
}

static int bls_read_bootflow(struct udevice *dev, struct bootflow *bflow)
{
	struct bls_entry entry;
	struct blk_desc *desc;
	const char *const *prefixes;
	struct udevice *bootstd;
	const char *prefix;
	loff_t size;
	int ret, i;

	log_debug("BLS: starting part %d\n", bflow->part);

	/* Get bootstd device for prefixes */
	ret = uclass_first_device_err(UCLASS_BOOTSTD, &bootstd);
	if (ret) {
		log_debug("no bootstd\n");
		return log_msg_ret("std", ret);
	}

	/* Block devices require a partition table */
	if (bflow->blk && !bflow->part) {
		log_debug("no partition table\n");
		return -ENOENT;
	}

	prefixes = bootstd_get_prefixes(bootstd);
	desc = bflow->blk ? dev_get_uclass_plat(bflow->blk) : NULL;

	/* Try each prefix to find the BLS entry file */
	i = 0;
	do {
		prefix = prefixes ? prefixes[i] : NULL;
		log_debug("trying prefix %s\n", prefix);

		ret = bootmeth_try_file(bflow, desc, prefix, BLS_ENTRY_FILE);
	} while (ret && prefixes && prefixes[++i]);

	if (ret) {
		log_debug("no BLS entry file found\n");
		return log_msg_ret("try", ret);
	}

	size = bflow->size;

	/* Read the file */
	ret = bootmeth_alloc_file(bflow, 0x10000, ARCH_DMA_MINALIGN,
				  BFI_BLS_CFG);
	if (ret)
		return log_msg_ret("read", ret);

	ret = bls_entry_init(&entry, bflow, size);
	bls_entry_uninit(&entry);

	return ret;
}

/**
 * bls_load_files() - Load files using an existing label
 *
 * @dev: Bootmethod device
 * @bflow: Bootflow to load files for
 * @pxe_ctx: Returns initialized PXE context (caller must destroy)
 * @label: PXE label to use for loading
 * Return: 0 on success, -ve on error
 */
static int bls_load_files(struct udevice *dev, struct bootflow *bflow,
			  struct pxe_context *pxe_ctx,
			  struct pxe_label *label)
{
	const struct bootflow_img *first_img;
	struct bls_info info;
	struct pxe_file *file;
	bool already_loaded;
	int ret;

	/* Check if files are already loaded (first image has address) */
	first_img = alist_get(&bflow->images, 0, struct bootflow_img);
	already_loaded = first_img && first_img->addr;

	/* Set up PXE context */
	info.dev = dev;
	info.bflow = bflow;
	ret = pxe_setup_ctx(pxe_ctx, bls_getfile, &info, true, bflow->fname,
			    false, false, bflow);
	if (ret)
		return log_msg_ret("ctx", ret);

	if (!already_loaded) {
		/* Load files (kernel, initrd, FDT) */
		ret = pxe_load_files(pxe_ctx, label, NULL);
		if (ret) {
			pxe_destroy_ctx(pxe_ctx);
			return log_msg_ret("load", ret);
		}

		/* Update loaded images with their addresses */
		alist_for_each(file, &label->files) {
			struct bootflow_img *img;

			/* Find the corresponding image in bootflow */
			alist_for_each(img, &bflow->images) {
				if (!strcmp(img->fname, file->path)) {
					img->addr = file->addr;
					img->size = file->size;
					break;
				}
			}
		}
	}

	/* Process FDT (apply overlays, etc.) */
	ret = pxe_setup_label(pxe_ctx, label);
	if (ret) {
		pxe_destroy_ctx(pxe_ctx);
		return log_msg_ret("setup", ret);
	}

	return 0;
}

/**
 * bls_load_all() - Load all files needed for boot
 *
 * @dev: Bootmethod device
 * @bflow: Bootflow to load files for
 * @pxe_ctx: Returns initialized PXE context (caller must destroy)
 * @labelp: Returns PXE label (caller must destroy)
 * Return: 0 on success, -ve on error
 */
static int bls_load_all(struct udevice *dev, struct bootflow *bflow,
			struct pxe_context *pxe_ctx,
			struct pxe_label **labelp)
{
	struct pxe_label *label;
	int ret;

	/* Convert bootflow to PXE label for boot execution */
	ret = bls_to_pxe_label(bflow, &label);
	if (ret)
		return log_msg_ret("label", ret);

	ret = bls_load_files(dev, bflow, pxe_ctx, label);
	if (ret) {
		label_destroy(label);
		return ret;
	}

	*labelp = label;

	return 0;
}

static int __maybe_unused bls_read_all(struct udevice *dev,
				       struct bootflow *bflow)
{
	struct pxe_context pxe_ctx;
	struct pxe_label *label;
	int ret;

	ret = bls_load_all(dev, bflow, &pxe_ctx, &label);
	if (ret)
		return ret;

	pxe_destroy_ctx(&pxe_ctx);
	label_destroy(label);

	return 0;
}

static int bls_boot(struct udevice *dev, struct bootflow *bflow)
{
	struct pxe_context pxe_ctx;
	struct pxe_label *label;
	int ret;

	ret = bls_load_all(dev, bflow, &pxe_ctx, &label);
	if (ret)
		return ret;

	/* Boot the label */
	pxe_ctx.label = label;
	ret = pxe_boot(&pxe_ctx);

	/* Cleanup */
	pxe_destroy_ctx(&pxe_ctx);
	label_destroy(label);

	return log_msg_ret("boot", ret);
}

static int bls_bootmeth_bind(struct udevice *dev)
{
	struct bootmeth_uc_plat *plat = dev_get_uclass_plat(dev);

	plat->desc = IS_ENABLED(CONFIG_BOOTSTD_FULL) ?
		"Boot Loader Specification (BLS) Type #1" : "bls";

	return 0;
}

static struct bootmeth_ops bls_bootmeth_ops = {
	.get_state_desc	= bls_get_state_desc,
	.check		= bls_check,
	.read_bootflow	= bls_read_bootflow,
	.read_file	= bootmeth_common_read_file,
#if CONFIG_IS_ENABLED(BOOTSTD_FULL)
	.read_all	= bls_read_all,
#endif
	.boot		= bls_boot,
};

static const struct udevice_id bls_bootmeth_ids[] = {
	{ .compatible = "u-boot,boot-loader-specification" },
	{ }
};

/* Put a number before 'bls' to provide a default ordering */
U_BOOT_DRIVER(bootmeth_2bls) = {
	.name		= "bootmeth_bls",
	.id		= UCLASS_BOOTMETH,
	.of_match	= bls_bootmeth_ids,
	.ops		= &bls_bootmeth_ops,
	.bind		= bls_bootmeth_bind,
};

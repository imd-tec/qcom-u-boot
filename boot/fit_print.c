// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2013, Google Inc.
 *
 * (C) Copyright 2008 Semihalf
 *
 * (C) Copyright 2000-2006
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

#define LOG_CATEGORY LOGC_BOOT

#ifdef USE_HOSTCC
#include "mkimage.h"
#include <time.h>
#include <linux/libfdt.h>
#else
#include <log.h>
#include <malloc.h>
#include <mapmem.h>
#include <linux/compiler.h>
#endif

#include <image.h>
#include <u-boot/crc.h>

/**
 * fit_print_init() - initialize FIT print context
 * @ctx: pointer to FIT print context to initialize
 * @fit: pointer to the FIT format image header
 * @indent: indentation string for printing
 *
 * This initializes a fit_print_ctx structure with the given FIT image.
 */
void fit_print_init(struct fit_print_ctx *ctx, const void *fit,
		    const char *indent)
{
	ctx->fit = fit;
	ctx->indent = indent;
	ctx->tab = 16 + strlen(indent);
}

/**
 * emit_type() - print a label with indentation and padding
 * @ctx: pointer to FIT print context
 * @type: type prefix (e.g., "Hash" or "Sign")
 * @label: label suffix (e.g., "algo" or "value")
 *
 * Prints the indentation from the context, followed by two spaces, the type,
 * a space, the label, a colon, and padding to align values to ctx->tab.
 */
static void emit_type(struct fit_print_ctx *ctx, const char *type,
		      const char *label)
{
	int len;

	len = printf("%s  %s %s:", ctx->indent, type, label);
	printf("%*s", ctx->tab - len, "");
}

/**
 * emit_label() - print a label with indentation and padding
 * @ctx: pointer to FIT print context
 * @type: type prefix (e.g., "Hash" or "Sign")
 * @label: label suffix (e.g., "algo" or "value")
 *
 * Prints the indentation from the context, followed by two spaces, a space,
 * the label, a colon, and padding to align values to ctx->tab.
 */
static void emit_label(struct fit_print_ctx *ctx, const char *label)
{
	int len;

	len = printf("%s  %s:", ctx->indent, label);
	printf("%*s", ctx->tab - len, "");
}

/**
 * fit_image_print_data() - prints out the hash node details
 * @ctx: pointer to FIT print context
 * @noffset: offset of the hash node
 * @type: Type of information to print ("hash" or "sign")
 *
 * fit_image_print_data() lists properties for the processed hash node
 *
 * This function avoid using puts() since it prints a newline on the host
 * but does not in U-Boot.
 *
 * returns:
 *     no returned results
 */
static void fit_image_print_data(struct fit_print_ctx *ctx, int noffset,
				 const char *type)
{
	const char *keyname, *padding, *algo;
	const char *p = ctx->indent;
	const void *fit = ctx->fit;
	int value_len, ret, i;
	uint8_t *value;

	debug("%s  %s node:    '%s'\n", p, type, fit_get_name(fit, noffset));
	emit_type(ctx, type, "algo");
	if (fit_image_hash_get_algo(fit, noffset, &algo)) {
		printf("invalid/unsupported\n");
		return;
	}
	printf("%s", algo);
	keyname = fdt_getprop(fit, noffset, FIT_KEY_HINT, NULL);
	if (keyname)
		printf(":%s", keyname);
	printf("\n");

	padding = fdt_getprop(fit, noffset, "padding", NULL);
	if (padding)
		printf("%s  %s padding: %s\n", p, type, padding);

	ret = fit_image_hash_get_value(fit, noffset, &value, &value_len);
	emit_type(ctx, type, "value");
	if (ret) {
		printf("unavailable\n");
	} else {
		for (i = 0; i < value_len; i++)
			printf("%02x", value[i]);
		printf("\n");
	}

	debug("%s  %s len:     %d\n", p, type, value_len);

	/* Signatures have a time stamp */
	if (IMAGE_ENABLE_TIMESTAMP && keyname) {
		time_t timestamp;

		emit_label(ctx, "Timestamp");
		if (fit_get_timestamp(fit, noffset, &timestamp))
			printf("unavailable\n");
		else
			genimg_print_time(timestamp);
	}
}

/**
 * fit_image_print_verification_data() - prints out the hash/signature details
 * @ctx: pointer to FIT print context
 * @noffset: offset of the hash or signature node
 *
 * This lists properties for the processed hash node
 *
 * returns:
 *     no returned results
 */
static void fit_image_print_verification_data(struct fit_print_ctx *ctx,
					      int noffset)
{
	const void *fit = ctx->fit;
	const char *name;

	/*
	 * Check subnode name, must be equal to "hash" or "signature".
	 * Multiple hash/signature nodes require unique unit node
	 * names, e.g. hash-1, hash-2, signature-1, signature-2, etc.
	 */
	name = fit_get_name(fit, noffset);
	if (!strncmp(name, FIT_HASH_NODENAME, strlen(FIT_HASH_NODENAME)))
		fit_image_print_data(ctx, noffset, "Hash");
	else if (!strncmp(name, FIT_SIG_NODENAME, strlen(FIT_SIG_NODENAME)))
		fit_image_print_data(ctx, noffset, "Sign");
}

/**
 * fit_image_print - prints out the FIT component image details
 * @ctx: pointer to FIT print context
 * @image_noffset: offset of the component image node
 * @p: pointer to prefix string
 *
 * fit_image_print() lists all mandatory properties for the processed component
 * image. If present, hash nodes are printed out as well. Load
 * address for images of type firmware is also printed out. Since the load
 * address is not mandatory for firmware images, it will be output as
 * "unavailable" when not present.
 *
 * returns:
 *     no returned results
 */
void fit_image_print(struct fit_print_ctx *ctx, int image_noffset)
{
	const void *fit = ctx->fit;
	const char *p = ctx->indent;
	uint8_t type, arch, os, comp = IH_COMP_NONE;
	const char *desc;
	size_t size;
	ulong load, entry;
	const void *data;
	int noffset;
	int ndepth;
	int ret;

	/* Mandatory properties */
	ret = fit_get_desc(fit, image_noffset, &desc);
	printf("%s  Description:  ", p);
	if (ret)
		printf("unavailable\n");
	else
		printf("%s\n", desc);

	if (IMAGE_ENABLE_TIMESTAMP) {
		time_t timestamp;

		ret = fit_get_timestamp(fit, 0, &timestamp);
		printf("%s  Created:      ", p);
		if (ret)
			printf("unavailable\n");
		else
			genimg_print_time(timestamp);
	}

	fit_image_get_type(fit, image_noffset, &type);
	printf("%s  Type:         %s\n", p, genimg_get_type_name(type));

	fit_image_get_comp(fit, image_noffset, &comp);
	printf("%s  Compression:  %s\n", p, genimg_get_comp_name(comp));

	ret = fit_image_get_data(fit, image_noffset, &data, &size);

	if (!tools_build()) {
		printf("%s  Data Start:   ", p);
		if (ret) {
			printf("unavailable\n");
		} else {
			void *vdata = (void *)data;

			printf("0x%08lx\n", (ulong)map_to_sysmem(vdata));
		}
	}

	printf("%s  Data Size:    ", p);
	if (ret)
		printf("unavailable\n");
	else
		genimg_print_size(size);

	/* Remaining, type dependent properties */
	if (type == IH_TYPE_KERNEL || type == IH_TYPE_STANDALONE ||
	    type == IH_TYPE_RAMDISK || type == IH_TYPE_FIRMWARE ||
	    type == IH_TYPE_FLATDT) {
		fit_image_get_arch(fit, image_noffset, &arch);
		printf("%s  Architecture: %s\n", p, genimg_get_arch_name(arch));
	}

	if (type == IH_TYPE_KERNEL || type == IH_TYPE_RAMDISK ||
	    type == IH_TYPE_FIRMWARE) {
		fit_image_get_os(fit, image_noffset, &os);
		printf("%s  OS:           %s\n", p, genimg_get_os_name(os));
	}

	if (type == IH_TYPE_KERNEL || type == IH_TYPE_STANDALONE ||
	    type == IH_TYPE_FIRMWARE || type == IH_TYPE_RAMDISK ||
	    type == IH_TYPE_FPGA) {
		ret = fit_image_get_load(fit, image_noffset, &load);
		printf("%s  Load Address: ", p);
		if (ret)
			printf("unavailable\n");
		else
			printf("0x%08lx\n", load);
	}

	/* optional load address for FDT */
	if (type == IH_TYPE_FLATDT &&
	    !fit_image_get_load(fit, image_noffset, &load))
		printf("%s  Load Address: 0x%08lx\n", p, load);

	if (type == IH_TYPE_KERNEL || type == IH_TYPE_STANDALONE ||
	    type == IH_TYPE_RAMDISK) {
		ret = fit_image_get_entry(fit, image_noffset, &entry);
		printf("%s  Entry Point:  ", p);
		if (ret)
			printf("unavailable\n");
		else
			printf("0x%08lx\n", entry);
	}

	/* Process all hash subnodes of the component image node */
	for (ndepth = 0, noffset = fdt_next_node(fit, image_noffset, &ndepth);
	     (noffset >= 0) && (ndepth > 0);
	     noffset = fdt_next_node(fit, noffset, &ndepth)) {
		if (ndepth == 1) {
			/* Direct child node of the component image node */
			fit_image_print_verification_data(ctx, noffset);
		}
	}
}

/**
 * fit_conf_print - prints out the FIT configuration details
 * @ctx: pointer to FIT print context
 * @noffset: offset of the configuration node
 *
 * fit_conf_print() lists all mandatory properties for the processed
 * configuration node.
 *
 * returns:
 *     no returned results
 */
static void fit_conf_print(struct fit_print_ctx *ctx, int noffset)
{
	const void *fit = ctx->fit;
	const char *p = ctx->indent;
	const char *uname, *desc;
	int ret, ndepth, i;

	/* Mandatory properties */
	ret = fit_get_desc(fit, noffset, &desc);
	printf("%s  Description:  ", p);
	if (ret)
		printf("unavailable\n");
	else
		printf("%s\n", desc);

	uname = fdt_getprop(fit, noffset, FIT_KERNEL_PROP, NULL);
	printf("%s  Kernel:       ", p);
	if (!uname)
		printf("unavailable\n");
	else
		printf("%s\n", uname);

	/* Optional properties */
	uname = fdt_getprop(fit, noffset, FIT_RAMDISK_PROP, NULL);
	if (uname)
		printf("%s  Init Ramdisk: %s\n", p, uname);

	uname = fdt_getprop(fit, noffset, FIT_FIRMWARE_PROP, NULL);
	if (uname)
		printf("%s  Firmware:     %s\n", p, uname);

	for (i = 0;
	     uname = fdt_stringlist_get(fit, noffset, FIT_FDT_PROP,
					i, NULL), uname;
	     i++) {
		if (!i)
			printf("%s  FDT:          ", p);
		else
			printf("%s                ", p);
		printf("%s\n", uname);
	}

	uname = fdt_getprop(fit, noffset, FIT_FPGA_PROP, NULL);
	if (uname)
		printf("%s  FPGA:         %s\n", p, uname);

	/* Print out all of the specified loadables */
	for (i = 0;
	     uname = fdt_stringlist_get(fit, noffset, FIT_LOADABLE_PROP,
					i, NULL), uname;
	     i++) {
		if (!i)
			printf("%s  Loadables:    ", p);
		else
			printf("%s                ", p);
		printf("%s\n", uname);
	}

	/* Show the list of compatible strings */
	for (i = 0; uname = fdt_stringlist_get(fit, noffset,
				FIT_COMPATIBLE_PROP, i, NULL), uname; i++) {
		if (!i)
			printf("%s  Compatible:   ", p);
		else
			printf("%s                ", p);
		printf("%s\n", uname);
	}

	/* Process all hash subnodes of the component configuration node */
	for (ndepth = 0, noffset = fdt_next_node(fit, noffset, &ndepth);
	     (noffset >= 0) && (ndepth > 0);
	     noffset = fdt_next_node(fit, noffset, &ndepth)) {
		if (ndepth == 1) {
			/* Direct child node of the component config node */
			fit_image_print_verification_data(ctx, noffset);
		}
	}
}

void fit_print(struct fit_print_ctx *ctx)
{
	const void *fit = ctx->fit;
	const char *p = ctx->indent;
	const char *desc;
	char *uname;
	int images_noffset;
	int confs_noffset;
	int noffset;
	int ndepth;
	int count = 0;
	int ret;
	time_t timestamp;

	/* Root node properties */
	ret = fit_get_desc(fit, 0, &desc);
	printf("%sFIT description: ", p);
	if (ret)
		printf("unavailable\n");
	else
		printf("%s\n", desc);

	if (IMAGE_ENABLE_TIMESTAMP) {
		ret = fit_get_timestamp(fit, 0, &timestamp);
		printf("%sCreated:         ", p);
		if (ret)
			printf("unavailable\n");
		else
			genimg_print_time(timestamp);
	}

	/* Find images parent node offset */
	images_noffset = fdt_path_offset(fit, FIT_IMAGES_PATH);
	if (images_noffset < 0) {
		printf("Can't find images parent node '%s' (%s)\n",
		       FIT_IMAGES_PATH, fdt_strerror(images_noffset));
		return;
	}

	/* Process its subnodes, print out component images details */
	for (ndepth = 0, count = 0,
		noffset = fdt_next_node(fit, images_noffset, &ndepth);
	     (noffset >= 0) && (ndepth > 0);
	     noffset = fdt_next_node(fit, noffset, &ndepth)) {
		if (ndepth == 1) {
			/*
			 * Direct child node of the images parent node,
			 * i.e. component image node.
			 */
			printf("%s Image %u (%s)\n", p, count++,
			       fit_get_name(fit, noffset));

			fit_image_print(ctx, noffset);
		}
	}

	/* Find configurations parent node offset */
	confs_noffset = fdt_path_offset(fit, FIT_CONFS_PATH);
	if (confs_noffset < 0) {
		debug("Can't get configurations parent node '%s' (%s)\n",
		      FIT_CONFS_PATH, fdt_strerror(confs_noffset));
		return;
	}

	/* get default configuration unit name from default property */
	uname = (char *)fdt_getprop(fit, noffset, FIT_DEFAULT_PROP, NULL);
	if (uname)
		printf("%s Default Configuration: '%s'\n", p, uname);

	/* Process its subnodes, print out configurations details */
	for (ndepth = 0, count = 0,
		noffset = fdt_next_node(fit, confs_noffset, &ndepth);
	     (noffset >= 0) && (ndepth > 0);
	     noffset = fdt_next_node(fit, noffset, &ndepth)) {
		if (ndepth == 1) {
			/*
			 * Direct child node of the configurations parent node,
			 * i.e. configuration node.
			 */
			printf("%s Configuration %u (%s)\n", p, count++,
			       fit_get_name(fit, noffset));

			fit_conf_print(ctx, noffset);
		}
	}
}

void fit_print_contents(const void *fit)
{
	struct fit_print_ctx ctx;

	fit_print_init(&ctx, fit, IMAGE_INDENT_STRING);
	fit_print(&ctx);
}

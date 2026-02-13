/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Boot Loader Specification (BLS) Type #1 support
 *
 * Copyright 2026 Canonical Ltd
 * Written by Simon Glass <simon.glass@canonical.com>
 */

#ifndef __BLS_H
#define __BLS_H

#include <alist.h>
#include <linux/types.h>

/**
 * struct bls_entry - represents a single BLS boot entry
 *
 * This structure holds the parsed fields from a BLS Type #1 entry file.
 * BLS entries use a simple key-value format with one field per line.
 *
 * Most fields point directly into the parsed buffer and are only valid while
 * the buffer remains valid. The exception is @options which is allocated
 * because multiple option lines must be concatenated.
 *
 * @title: Human-readable name (points into buffer)
 * @version: OS version string (points into buffer)
 * @kernel: Kernel path - required unless @fit is set (points into buffer)
 *          Can include FIT config syntax: path#config
 * @fit: FIT image path - required unless @kernel is set (points into buffer)
 * @options: Kernel command line - ALLOCATED, must be freed
 *           Multiple options lines are concatenated with spaces
 * @initrds: List of initrd paths (alist of char * pointing into buffer)
 *           Multiple initrd lines are supported and accumulated
 * @devicetree: Device tree blob path (points into buffer)
 * @dt_overlays: Device tree overlays (points into buffer)
 * @architecture: Target architecture (points into buffer)
 * @machine_id: OS identifier (points into buffer)
 * @sort_key: Sorting identifier (points into buffer)
 * @filename: Path to .conf file (points into buffer)
 */
struct bls_entry {
	char *title;
	char *version;
	char *kernel;
	char *fit;
	char *options;		/* Allocated */
	struct alist initrds;	/* list of char * into buffer */
	char *devicetree;
	char *dt_overlays;
	char *architecture;
	char *machine_id;
	char *sort_key;
	char *filename;
};

/**
 * bls_parse_entry() - Parse a BLS entry file
 *
 * Parses the contents of a BLS Type #1 entry file into a pre-allocated
 * entry structure. The format is simple key-value pairs with one field per
 * line. Lines starting with '#' are comments and blank lines are ignored.
 *
 * The entry is initialized to zero before parsing. Most entry fields will
 * point directly into the buffer (which is modified to add null terminators).
 * The buffer must remain valid for the lifetime of the entry. Only the
 * 'options' field is allocated separately because multiple option lines must
 * be concatenated.
 *
 * The caller must call bls_entry_uninit() on the entry when done, regardless
 * of whether this function succeeds or fails, to free any allocated memory.
 *
 * Supported fields:
 *   title       - Human-readable name
 *   version     - OS version string
 *   linux       - Kernel path (required unless 'fit' is present)
 *   fit         - FIT image path (required unless 'linux' is present)
 *   options     - Kernel command line (allocated, can appear multiple times)
 *   initrd      - Initramfs path (can appear multiple times)
 *   devicetree  - Device tree blob path
 *
 * Unknown fields are ignored for forward compatibility.
 *
 * @buf: Buffer containing the BLS entry file contents (will be modified)
 * @size: Size of the buffer in bytes
 * @entry: BLS entry structure to fill in (will be initialized)
 * Return: 0 on success, -ENOMEM if out of memory, -EINVAL if required fields
 *         are missing
 */
int bls_parse_entry(const char *buf, size_t size, struct bls_entry *entry);


/**
 * bls_entry_uninit() - Clean up a BLS entry's fields
 *
 * Frees all allocated fields within the entry but does not free the entry
 * structure itself. Use this for stack-allocated entries.
 *
 * @entry: Entry to clean up
 */
void bls_entry_uninit(struct bls_entry *entry);


#endif /* __BLS_H */

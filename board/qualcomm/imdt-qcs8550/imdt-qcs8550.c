// SPDX-License-Identifier: GPL-2.0+
/*
 * IMDT QCS8550 SBC board support
 *
 * Copyright (c) 2026 IMD Technologies Ltd
 */

#include <bootm.h>
#include <efi.h>
#include <efi_api.h>
#include <event.h>
#include <fdt_support.h>
#include <init.h>
#include <hexdump.h>
#include <malloc.h>
#include <linux/libfdt.h>
#include <log.h>

/* UEFI type compat for Qualcomm EFI headers */
#define EFI_STATUS efi_status_t
#define BOOLEAN u8
#define CHAR8 char
#define IN
#define OUT

/*
 * U-Boot's EFI_GUID is a function-like macro; the Qualcomm headers
 * also use EFI_GUID as a type name.  Temporarily swap it to a typedef.
 */
typedef efi_guid_t EFI_GUID;

#include "EFIChipInfo.h"


struct mm_region *mem_map;

/*
 * CPU partial goods tables for Snapdragon 8550 SOCs.
 *
 * Each bit in the fuse value corresponds to a disabled CPU.
 * When a bit is set, we set that CPU's enable-method to "none"
 * and status to "fail" in the FDT.
 *
 * Three CPU topologies exist across 8550 variants. We probe
 * which one matches the DTS at runtime (matching ABL's
 * CheckCPUType logic).
 */
#define NUM_CPUS	8
#define NUM_CPU_TYPES	3

static const char * const cpu_type0[NUM_CPUS] = {
	"cpu@0", "cpu@100", "cpu@200", "cpu@300",
	"cpu@400", "cpu@500", "cpu@600", "cpu@700",
};

static const char * const cpu_type1[NUM_CPUS] = {
	"cpu@101", "cpu@102", "cpu@103", "cpu@104",
	"cpu@105", "cpu@106", "cpu@107", "cpu@108",
};

static const char * const cpu_type2[NUM_CPUS] = {
	"cpu@0", "cpu@1", "cpu@2", "cpu@3",
	"cpu@100", "cpu@101", "cpu@102", "cpu@103",
};

static const char * const *cpu_types[NUM_CPU_TYPES] = {
	cpu_type0, cpu_type1, cpu_type2,
};

static int read_cpu_fuse_value(u32 *valuep)
{
	efi_guid_t guid = EFI_CHIPINFO_PROTOCOL_GUID;
	struct efi_boot_services *boot = efi_get_boot();
	EFI_CHIPINFO_PROTOCOL *chip;
	efi_status_t ret;
	u32 val = 0;

	ret = boot->locate_protocol(&guid, NULL, (void **)&chip);
	if (ret) {
		log_err("Failed to locate ChipInfo protocol: %lx\n",
			(ulong)ret);
		return -ENOENT;
	}

	if (chip->Revision >= EFI_CHIPINFO_PROTOCOL_REVISION_5) {
		ret = chip->GetDisabledCPUs(chip, 0, &val);
		if (ret && ret != EFI_NOT_FOUND)
			return -EIO;
	}

	if (chip->Revision < EFI_CHIPINFO_PROTOCOL_REVISION_5 ||
	    ret == EFI_NOT_FOUND) {
		ret = chip->GetSubsetCPUs(chip, 0, &val);
		if (ret)
			return -EIO;
	}

	*valuep = val;
	return 0;
}

/*
 * Detect which CPU topology matches the DTS by checking whether
 * all nodes in a given table exist under /cpus.
 */
static const char * const *detect_cpu_type(void *fdt, int cpus_off)
{
	int t, i;

	for (t = 0; t < NUM_CPU_TYPES; t++) {
		for (i = 0; i < NUM_CPUS; i++) {
			if (fdt_subnode_offset(fdt, cpus_off,
					       cpu_types[t][i]) < 0)
				break;
		}
		if (i == NUM_CPUS)
			return cpu_types[t];
	}

	return NULL;
}

/*
 * The DRAM is populated on the SoM, but as a UEFI app we only get our own
 * pool (CONFIG_EFI_RAM_SIZE, 512MB) reflected in gd->ram_size - so we must
 * not let the generic memory fixup rewrite the kernel /memory node. Instead
 * we detect the installed DRAM size from the EFI memory map and apply the
 * matching bank layout, which mirrors the previous bootloader's usable-RAM
 * partition table for each SoM variant.
 *
 * Entries are ordered highest threshold first.
 */
struct imdt_mem_layout {
	const char *name;
	phys_size_t threshold;	/* minimum total DRAM to select this layout */
	int count;
	u64 start[8];
	u64 size[8];
};

static const struct imdt_mem_layout imdt_mem_layouts[] = {
	{
		.name = "12GB", .threshold = 9ULL << 30, .count = 3,
		.start = {
			0xbab00000ULL,	/* 210 MiB, < 4 GiB (DMA) */
			0x880000000ULL,	/* 922 MiB */
			0x8c0000000ULL,	/* 9 GiB (kernel load region) */
		},
		.size = {
			0x0d1b8000ULL,
			0x39a00000ULL,
			0x240000000ULL,
		},
	},
	{
		.name = "8GB", .threshold = 0, .count = 3,
		.start = {
			0xbab00000ULL,	/* 210 MiB, < 4 GiB (DMA) */
			0x880000000ULL,	/* 922 MiB */
			0x8c0000000ULL,	/* 5 GiB (kernel load region) */
		},
		.size = {
			0x0d1b8000ULL,
			0x39a00000ULL,
			0x140000000ULL,
		},
	},
};

/* Total installed DRAM as reported by the EFI memory map. */
static phys_size_t imdt_total_dram(void)
{
	struct efi_mem_desc *map, *desc, *end;
	phys_size_t total = 0;
	int size, desc_size, ret;
	uint version, key;

	ret = efi_get_mmap(&map, &size, &key, &desc_size, &version);
	if (ret) {
		log_warning("Failed to read EFI memory map: %d\n", ret);
		return 0;
	}

	end = (void *)map + size;
	for (desc = map; desc < end;
	     desc = efi_get_next_mem_desc(desc, desc_size)) {
		if (desc->type == EFI_MMAP_IO ||
		    desc->type == EFI_MMAP_IO_PORT)
			continue;
		total += desc->num_pages << EFI_PAGE_SHIFT;
	}

	free(map);
	return total;
}

/*
 * The Qualcomm kernel DT names its DRAM node "memory@a0000000" rather than
 * the generic "/memory". The standard fixup helpers target "/memory" and so
 * would create a second, conflicting node - we must update this one in place.
 */
#define IMDT_MEMORY_NODE	"/memory@a0000000"

/* Pack (start, size) banks into a "reg" stream per the root address/size
 * cells, mirroring fdt_pack_reg() which is private to fdt_support.c.
 */
static int imdt_pack_reg(const void *blob, fdt32_t *buf, const u64 *start,
			 const u64 *size, int banks)
{
	int ac = fdt_address_cells(blob, 0);
	int sc = fdt_size_cells(blob, 0);
	fdt32_t *p = buf;
	int i;

	for (i = 0; i < banks; i++) {
		if (ac == 2)
			*p++ = cpu_to_fdt32(upper_32_bits(start[i]));
		*p++ = cpu_to_fdt32(lower_32_bits(start[i]));
		if (sc == 2)
			*p++ = cpu_to_fdt32(upper_32_bits(size[i]));
		*p++ = cpu_to_fdt32(lower_32_bits(size[i]));
	}

	return (p - buf) * sizeof(*buf);
}

/* Rewrite the kernel memory node to match the installed DRAM size. */
static void imdt_fixup_memory(void *blob)
{
	const struct imdt_mem_layout *layout = NULL;
	phys_size_t total = imdt_total_dram();
	fdt32_t reg[8 * 4]; /* up to 8 banks, 2 address + 2 size cells each */
	int i, off, len, err;

	log_info("EFI memory map reports %llu MiB of DRAM\n",
		 (unsigned long long)(total >> 20));

	for (i = 0; i < ARRAY_SIZE(imdt_mem_layouts); i++) {
		if (total >= imdt_mem_layouts[i].threshold) {
			layout = &imdt_mem_layouts[i];
			break;
		}
	}
	if (!layout) {
		log_err("No matching DRAM layout; leaving memory node as-is\n");
		return;
	}

	off = fdt_path_offset(blob, IMDT_MEMORY_NODE);
	if (off < 0) {
		log_err("Memory node %s not found: %s\n", IMDT_MEMORY_NODE,
			fdt_strerror(off));
		return;
	}

	log_info("Applying %s layout to %s (%d banks)\n",
		 layout->name, IMDT_MEMORY_NODE, layout->count);

	len = imdt_pack_reg(blob, reg, layout->start, layout->size,
			    layout->count);
	err = fdt_setprop(blob, off, "reg", reg, len);
	if (err)
		log_err("Failed to update %s reg: %s\n", IMDT_MEMORY_NODE,
			fdt_strerror(err));
}

/* Print every /memory node in the blob as it will be handed to the kernel. */
static void imdt_print_memory(const void *blob)
{
	int ac = fdt_address_cells(blob, 0);
	int sc = fdt_size_cells(blob, 0);
	int off;

	if (ac < 1 || sc < 1)
		return;

	for (off = fdt_next_node(blob, -1, NULL); off >= 0;
	     off = fdt_next_node(blob, off, NULL)) {
		const fdt32_t *reg;
		const char *type;
		int len, i;

		type = fdt_getprop(blob, off, "device_type", NULL);
		if (!type || strcmp(type, "memory"))
			continue;

		reg = fdt_getprop(blob, off, "reg", &len);
		if (!reg)
			continue;

		printf("Memory node %s:\n", fdt_get_name(blob, off, NULL));
		for (i = 0; i + ac + sc <= len / sizeof(*reg); i += ac + sc) {
			u64 base = fdt_read_number(&reg[i], ac);
			u64 size = fdt_read_number(&reg[i + ac], sc);

			printf("  bank: 0x%010llx - 0x%010llx (%llu MiB)\n",
			       (unsigned long long)base,
			       (unsigned long long)(base + size),
			       (unsigned long long)(size >> 20));
		}
	}
}

int ft_system_setup(void *blob, struct bd_info *bd)
{
	const char * const *cpu_map;
	int cpus_off, i, ret;
	u32 fuse_val;

	imdt_fixup_memory(blob);
	imdt_print_memory(blob);

	ret = read_cpu_fuse_value(&fuse_val);
	if (ret || !fuse_val)
		return 0;
	printf("CPU partial goods fuse value: 0x%08x\n", fuse_val);

	cpus_off = fdt_path_offset(blob, "/cpus");
	if (cpus_off < 0)
		return 0;

	cpu_map = detect_cpu_type(blob, cpus_off);
	if (!cpu_map) {
		log_warning("No matching CPU topology found\n");
		return 0;
	}

	for (i = 0; i < NUM_CPUS; i++) {
		int off;

		if (!(fuse_val & BIT(i)))
			continue;

		off = fdt_subnode_offset(blob, cpus_off, cpu_map[i]);
		if (off < 0)
			continue;

		fdt_setprop_string(blob, off, "enable-method", "none");
		fdt_set_node_status(blob, off, FDT_STATUS_FAIL);
		log_info("Disabled partial-goods CPU: %s\n", cpu_map[i]);
	}

	printf("CPU partial goods applied\n");
	return 0;
}

int print_cpuinfo(void)
{
	return 0;
}

int board_init(void)
{
	return 0;
}

int board_exit_boot_services(void *ctx, struct event *evt)
{
	struct efi_priv *priv = efi_get_priv();
	struct efi_mem_desc *desc;
	int desc_size;
	uint version;
	int size;
	uint key;
	int ret;

	if (evt->data.bootm_final.flags & BOOTM_FINAL_FAKE) {
		printf("Not exiting EFI (fake go)\n");
		return 0;
	}
	printf("Exiting EFI\n");
	ret = efi_get_mmap(&desc, &size, &key, &desc_size, &version);
	if (ret) {
		printf("efi: Failed to get memory map\n");
		return -EFAULT;
	}

	ret = efi_app_exit_boot_services(priv, key);
	if (ret)
		return ret;

	/* no console output after here as there are no EFI drivers! */

	return 0;
}
EVENT_SPY_FULL(EVT_BOOTM_FINAL, board_exit_boot_services);

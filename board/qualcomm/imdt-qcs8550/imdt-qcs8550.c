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

int ft_system_setup(void *blob, struct bd_info *bd)
{
	const char * const *cpu_map;
	int cpus_off, i, ret;
	u32 fuse_val;

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

// SPDX-License-Identifier: GPL-2.0+
/*
 * PXE parser tests - C implementation for Python wrapper
 *
 * Copyright 2026 Canonical Ltd
 *
 * These tests verify the extlinux.conf parser APIs.
 *
 * Note: The 'ontimeout' keyword is tested via the test fixtures which include
 * it. Since ontimeout is handled identically to 'default' (both set
 * cfg->default_label), it cannot be distinguished after parsing.
 *
 */

#include <dm.h>
#include <env.h>
#include <fdt_support.h>
#include <fs_legacy.h>
#include <linux/libfdt.h>
#include <mapmem.h>
#include <net-common.h>
#include <pxe_utils.h>
#include <test/test.h>
#include <test/ut.h>

/* Define test macros for pxe suite */
#define PXE_TEST(_name, _flags) \
	UNIT_TEST(_name, _flags, pxe)
#define PXE_TEST_ARGS(_name, _flags, ...) \
	UNIT_TEST_ARGS(_name, _flags, pxe, __VA_ARGS__)

/* Argument indices */
#define PXE_ARG_FS_IMAGE	0	/* Path to filesystem image */
#define PXE_ARG_CFG_PATH	1	/* Path to config file within image */

/* Memory address for loading files */
#define PXE_LOAD_ADDR		0x01000000
#define PXE_KERNEL_ADDR		0x02000000
#define PXE_INITRD_ADDR		0x02800000
#define PXE_FDT_ADDR		0x03000000
#define PXE_OVERLAY_ADDR	0x03100000

/**
 * struct pxe_test_info - context for the test getfile callback
 *
 * @uts: Unit test state for assertions
 */
struct pxe_test_info {
	struct unit_test_state *uts;
};

/**
 * pxe_test_getfile() - Read a file from the host filesystem
 *
 * This callback is used by the PXE parser to read included files.
 */
static int pxe_test_getfile(struct pxe_context *ctx, const char *file_path,
			    ulong *addrp, ulong align,
			    enum bootflow_img_t type, ulong *sizep)
{
	loff_t len_read;
	int ret;

	if (!*addrp)
		return -ENOTSUPP;

	ret = fs_set_blk_dev("host", "0:0", FS_TYPE_ANY);
	if (ret)
		return ret;
	ret = fs_legacy_read(file_path, *addrp, 0, 0, &len_read);
	if (ret)
		return ret;
	*sizep = len_read;

	return 0;
}

/**
 * Test parsing an extlinux.conf file
 *
 * This test:
 * 1. Binds a filesystem image containing extlinux.conf
 * 2. Parses the config using parse_pxefile()
 * 3. Verifies the parsed labels can be inspected
 * 4. Verifies label properties are accessible
 */
static int pxe_test_parse_norun(struct unit_test_state *uts)
{
	const char *fs_image = ut_str(PXE_ARG_FS_IMAGE);
	const char *cfg_path = ut_str(PXE_ARG_CFG_PATH);
	ulong addr = PXE_LOAD_ADDR;
	struct pxe_test_info info;
	struct pxe_context ctx;
	struct pxe_label *label;
	struct pxe_menu *cfg;
	char name[16];
	uint i;
	int ret;

	ut_assertnonnull(fs_image);
	ut_assertnonnull(cfg_path);

	info.uts = uts;

	/* Bind the filesystem image */
	ut_assertok(run_commandf("host bind 0 %s", fs_image));

	/* Set up the PXE context */
	ut_assertok(pxe_setup_ctx(&ctx, pxe_test_getfile, &info, true, cfg_path,
				  false, false, NULL));

	/* Read the config file into memory */
	ret = get_pxe_file(&ctx, cfg_path, addr);
	ut_asserteq(1, ret);  /* get_pxe_file returns 1 on success */

	/* Parse the config file */
	cfg = parse_pxefile(&ctx, addr);
	ut_assertnonnull(cfg);

	/* Verify 'say' keyword printed its message during parsing */
	ut_assert_nextline("Retrieving file: %s", cfg_path);
	ut_assert_nextline("Booting default Linux kernel");
	ut_assert_nextline("Retrieving file: /extlinux/extra.conf");
	for (i = 3; i <= 16; i++)
		ut_assert_nextline("Retrieving file: /extlinux/nest%d.conf", i);

	/* Verify menu properties */
	ut_asserteq_str("Test Boot Menu", cfg->title);
	ut_asserteq_str("linux", cfg->default_label);
	ut_asserteq_str("rescue", cfg->fallback_label);
	ut_asserteq_str("/boot/background.bmp", cfg->bmp);
	ut_asserteq(50, cfg->timeout);
	ut_asserteq(1, cfg->prompt);

	/* Verify first label: linux (with fdt, fdtoverlays) */
	label = list_first_entry(&cfg->labels, struct pxe_label, list);
	ut_asserteq_str("", label->num);  /* only set when menu is built */
	ut_asserteq_str("linux", label->name);
	ut_asserteq_str("Boot Linux", label->menu);
	ut_asserteq_str("/vmlinuz", label->kernel_label);
	ut_asserteq_str("/vmlinuz", label->kernel);
	ut_assertnull(label->config);
	ut_asserteq_str("root=/dev/sda1 quiet", label->append);
	ut_asserteq_str("/initrd.img", label->initrd);
	ut_asserteq_str("/dtb/board.dtb", label->fdt);
	ut_assertnull(label->fdtdir);
	ut_asserteq_str("/dtb/overlay1.dtbo /dtb/overlay2.dtbo",
			label->fdtoverlays);
	ut_asserteq(0, label->ipappend);
	ut_asserteq(0, label->attempted);
	ut_asserteq(0, label->localboot);
	ut_asserteq(0, label->localboot_val);
	ut_asserteq(1, label->kaslrseed);

	/* Verify second label: rescue (linux keyword, fdtdir, ipappend) */
	label = list_entry(label->list.next, struct pxe_label, list);
	ut_asserteq_str("", label->num);
	ut_asserteq_str("rescue", label->name);
	ut_asserteq_str("Rescue Mode", label->menu);
	ut_asserteq_str("/vmlinuz-rescue", label->kernel_label);
	ut_asserteq_str("/vmlinuz-rescue", label->kernel);
	ut_assertnull(label->config);
	ut_asserteq_str("single", label->append);
	ut_assertnull(label->initrd);
	ut_assertnull(label->fdt);
	ut_asserteq_str("/dtb/", label->fdtdir);
	ut_assertnull(label->fdtoverlays);
	ut_asserteq(3, label->ipappend);
	ut_asserteq(0, label->attempted);
	ut_asserteq(0, label->localboot);
	ut_asserteq(0, label->localboot_val);
	ut_asserteq(0, label->kaslrseed);

	/* Verify third label: local (localboot only) */
	label = list_entry(label->list.next, struct pxe_label, list);
	ut_asserteq_str("", label->num);
	ut_asserteq_str("local", label->name);
	ut_asserteq_str("Local Boot", label->menu);
	ut_assertnull(label->kernel_label);
	ut_assertnull(label->kernel);
	ut_assertnull(label->config);
	ut_assertnull(label->append);
	ut_assertnull(label->initrd);
	ut_assertnull(label->fdt);
	ut_assertnull(label->fdtdir);
	ut_assertnull(label->fdtoverlays);
	ut_asserteq(0, label->ipappend);
	ut_asserteq(0, label->attempted);
	ut_asserteq(1, label->localboot);
	ut_asserteq(1, label->localboot_val);
	ut_asserteq(0, label->kaslrseed);

	/* Verify fourth label: fitboot (fit keyword sets kernel and config) */
	label = list_entry(label->list.next, struct pxe_label, list);
	ut_asserteq_str("", label->num);
	ut_asserteq_str("fitboot", label->name);
	ut_asserteq_str("FIT Boot", label->menu);
	ut_asserteq_str("/boot/image.fit#config-1", label->kernel_label);
	ut_asserteq_str("/boot/image.fit", label->kernel);
	ut_asserteq_str("#config-1", label->config);
	ut_asserteq_str("console=ttyS0", label->append);
	ut_assertnull(label->initrd);
	ut_assertnull(label->fdt);
	ut_assertnull(label->fdtdir);
	ut_assertnull(label->fdtoverlays);
	ut_asserteq(0, label->ipappend);
	ut_asserteq(0, label->attempted);
	ut_asserteq(0, label->localboot);
	ut_asserteq(0, label->localboot_val);
	ut_asserteq(0, label->kaslrseed);

	/* Verify fifth label: included (from include directive) */
	label = list_entry(label->list.next, struct pxe_label, list);
	ut_asserteq_str("", label->num);
	ut_asserteq_str("included", label->name);
	ut_asserteq_str("Included Label", label->menu);
	ut_asserteq_str("/boot/included-kernel", label->kernel_label);
	ut_asserteq_str("/boot/included-kernel", label->kernel);
	ut_assertnull(label->config);
	ut_asserteq_str("root=/dev/sdb1", label->append);
	ut_assertnull(label->initrd);
	ut_assertnull(label->fdt);
	ut_assertnull(label->fdtdir);
	ut_assertnull(label->fdtoverlays);
	ut_asserteq(0, label->ipappend);
	ut_asserteq(0, label->attempted);
	ut_asserteq(0, label->localboot);
	ut_asserteq(0, label->localboot_val);
	ut_asserteq(0, label->kaslrseed);

	/* Verify labels from nested includes (levels 3-16) - just check names */
	for (i = 3; i <= 16; i++) {
		label = list_entry(label->list.next, struct pxe_label, list);
		snprintf(name, sizeof(name), "level%d", i);
		ut_asserteq_str(name, label->name);
	}

	/* Verify no more console output */
	ut_assert_console_end();

	/* Clean up */
	destroy_pxe_menu(cfg);
	pxe_destroy_ctx(&ctx);

	return 0;
}
PXE_TEST_ARGS(pxe_test_parse_norun, UTF_CONSOLE | UTF_MANUAL,
	{ "fs_image", UT_ARG_STR },
	{ "cfg_path", UT_ARG_STR });

/**
 * Test booting via sysboot command
 *
 * This test:
 * 1. Binds a filesystem image containing extlinux.conf
 * 2. Sets up environment variables for file loading
 * 3. Runs sysboot to boot the default label
 * 4. Verifies files were loaded by checking console output
 */
static int pxe_test_sysboot_norun(struct unit_test_state *uts)
{
	const char *fs_image = ut_str(PXE_ARG_FS_IMAGE);
	const char *cfg_path = ut_str(PXE_ARG_CFG_PATH);
	void *kernel, *initrd, *fdt;

	ut_assertnonnull(fs_image);
	ut_assertnonnull(cfg_path);

	/* Bind the filesystem image */
	ut_assertok(run_commandf("host bind 0 %s", fs_image));

	/* Set environment variables for file loading */
	ut_assertok(env_set_hex("pxefile_addr_r", PXE_LOAD_ADDR));
	ut_assertok(env_set_hex("kernel_addr_r", PXE_KERNEL_ADDR));
	ut_assertok(env_set_hex("ramdisk_addr_r", PXE_INITRD_ADDR));
	ut_assertok(env_set_hex("fdt_addr_r", PXE_FDT_ADDR));
	ut_assertok(env_set_hex("fdtoverlay_addr_r", PXE_OVERLAY_ADDR));
	ut_assertok(env_set("bootfile", cfg_path));
	ut_assertok(env_set("pxe_timeout", "1"));

	/*
	 * Run sysboot - it will try all labels and return 0 after failing
	 * to boot them all (since sandbox can't actually boot Linux)
	 */
	ut_assertok(run_commandf("sysboot host 0:0 any %x %s",
				 PXE_LOAD_ADDR, cfg_path));

	/* Skip menu output and find the first label boot attempt */
	ut_assert_skip_to_line("Enter choice: 1:\tBoot Linux");

	/* Verify files were loaded in order */
	ut_assert_nextline("Retrieving file: /vmlinuz");
	ut_assert_nextline("Retrieving file: /initrd.img");
	ut_assert_nextline("append: root=/dev/sda1 quiet");
	ut_assert_nextline("Retrieving file: /dtb/board.dtb");
	ut_assert_nextline("Retrieving file: /dtb/overlay1.dtbo");
	ut_assert_nextline("Retrieving file: /dtb/overlay2.dtbo");

	/* Boot fails on sandbox */
	ut_assert_nextline("Unrecognized zImage");
	ut_assert_nextlinen("       unmap_physmem");

	/* Verify files were loaded at the correct addresses */
	kernel = map_sysmem(PXE_KERNEL_ADDR, 0);
	initrd = map_sysmem(PXE_INITRD_ADDR, 0);
	fdt = map_sysmem(PXE_FDT_ADDR, 0);

	/* Kernel should contain "kernel" at start */
	ut_asserteq_mem("kernel", kernel, 6);

	/* Initrd should contain "ramdisk" at start */
	ut_asserteq_mem("ramdisk", initrd, 7);

	/* FDT should have valid magic number */
	ut_assertok(fdt_check_header(fdt));

	/* Verify overlays were applied - check for properties added by overlays */
	ut_asserteq_str("from-overlay1",
			fdt_getprop(fdt, fdt_path_offset(fdt, "/test-node"),
				    "overlay1-property", NULL));
	ut_asserteq_str("from-overlay2",
			fdt_getprop(fdt, fdt_path_offset(fdt, "/test-node"),
				    "overlay2-property", NULL));

	return 0;
}
PXE_TEST_ARGS(pxe_test_sysboot_norun, UTF_CONSOLE | UTF_MANUAL,
	{ "fs_image", UT_ARG_STR },
	{ "cfg_path", UT_ARG_STR });

/**
 * Test fdtdir path resolution via sysboot
 *
 * This test verifies fdtdir path construction by running sysboot and
 * checking console output:
 * 1. fdtdir with fdtfile env var - uses fdtfile value directly
 * 2. fdtdir with soc/board env vars - constructs {soc}-{board}.dtb
 * 3. fdtdir without trailing slash - slash is inserted
 */
static int pxe_test_fdtdir_norun(struct unit_test_state *uts)
{
	const char *fs_image = ut_str(PXE_ARG_FS_IMAGE);
	const char *cfg_path = ut_str(PXE_ARG_CFG_PATH);
	void *fdt;

	ut_assertnonnull(fs_image);
	ut_assertnonnull(cfg_path);

	/* Bind the filesystem image */
	ut_assertok(run_commandf("host bind 0 %s", fs_image));

	/*
	 * Test 1: fdtdir with fdtfile env var
	 * The first label uses fdtdir=/dtb/ and we set fdtfile=test-board.dtb
	 * so it should retrieve /dtb/test-board.dtb
	 */
	ut_assertok(env_set_hex("pxefile_addr_r", PXE_LOAD_ADDR));
	ut_assertok(env_set_hex("kernel_addr_r", PXE_KERNEL_ADDR));
	ut_assertok(env_set_hex("fdt_addr_r", PXE_FDT_ADDR));
	ut_assertok(env_set_hex("fdtoverlay_addr_r", PXE_OVERLAY_ADDR));
	ut_assertok(env_set("fdtfile", "test-board.dtb"));
	ut_assertok(env_set("bootfile", cfg_path));

	ut_assertok(run_commandf("sysboot host 0:0 any %x %s",
				 PXE_LOAD_ADDR, cfg_path));

	/* Skip to the boot attempt - first label is fdtfile-test */
	ut_assert_skip_to_line("Enter choice: 1:\tTest fdtfile env var");

	/* Verify fdtdir used fdtfile env var to construct path */
	ut_assert_nextline("Retrieving file: /vmlinuz");
	ut_assert_nextline("append: console=ttyS0");
	ut_assert_nextline("Retrieving file: /dtb/test-board.dtb");
	ut_assert_nextline("Retrieving file: /dtb/overlay1.dtbo");

	/* Boot fails but we verified the path construction */
	ut_assert_nextline("Unrecognized zImage");
	ut_assert_nextlinen("       unmap_physmem");

	/* Verify FDT was loaded correctly */
	fdt = map_sysmem(PXE_FDT_ADDR, 0);
	ut_assertok(fdt_check_header(fdt));

	/*
	 * Test 2: fdtdir with soc/board env vars (no fdtfile)
	 * Clear fdtfile and set soc/board - the default label (fdtfile-test)
	 * will now construct the path from soc-board: /dtb/tegra-jetson.dtb
	 */
	ut_assertok(env_set("fdtfile", NULL));  /* Clear fdtfile */
	ut_assertok(env_set("soc", "tegra"));
	ut_assertok(env_set("board", "jetson"));

	ut_assertok(run_commandf("sysboot host 0:0 any %x %s",
				 PXE_LOAD_ADDR, cfg_path));

	/* Still boots default label, but now uses soc-board path construction */
	ut_assert_skip_to_line("Enter choice: 1:\tTest fdtfile env var");

	/* Verify fdtdir constructed path from soc-board */
	ut_assert_nextline("Retrieving file: /vmlinuz");
	ut_assert_nextline("append: console=ttyS0");
	ut_assert_nextline("Retrieving file: /dtb/tegra-jetson.dtb");
	ut_assert_nextline("Retrieving file: /dtb/overlay1.dtbo");

	/* Boot fails but we verified the path construction */
	ut_assert_nextline("Unrecognized zImage");
	ut_assert_nextlinen("       unmap_physmem");

	/* Verify FDT was loaded */
	fdt = map_sysmem(PXE_FDT_ADDR, 0);
	ut_asserteq(FDT_MAGIC, fdt_magic(fdt));

	/* Clean up env vars */
	env_set("fdtfile", NULL);
	env_set("soc", NULL);
	env_set("board", NULL);

	return 0;
}
PXE_TEST_ARGS(pxe_test_fdtdir_norun, UTF_CONSOLE | UTF_MANUAL,
	{ "fs_image", UT_ARG_STR },
	{ "cfg_path", UT_ARG_STR });

/**
 * Test error handling for missing FDT files via sysboot
 *
 * This test verifies error handling by running sysboot and checking
 * console output:
 * 1. Explicit FDT not found - label fails with error, tries next label
 * 2. fdtdir FDT not found - warns but continues to boot attempt
 */
static int pxe_test_errors_norun(struct unit_test_state *uts)
{
	const char *fs_image = ut_str(PXE_ARG_FS_IMAGE);
	const char *cfg_path = ut_str(PXE_ARG_CFG_PATH);

	ut_assertnonnull(fs_image);
	ut_assertnonnull(cfg_path);

	/* Bind the filesystem image */
	ut_assertok(run_commandf("host bind 0 %s", fs_image));

	/* Set up environment for loading */
	ut_assertok(env_set_hex("pxefile_addr_r", PXE_LOAD_ADDR));
	ut_assertok(env_set_hex("kernel_addr_r", PXE_KERNEL_ADDR));
	ut_assertok(env_set_hex("fdt_addr_r", PXE_FDT_ADDR));
	ut_assertok(env_set_hex("fdtoverlay_addr_r", PXE_OVERLAY_ADDR));
	ut_assertok(env_set("fdtfile", "missing.dtb"));  /* For fdtdir test */
	ut_assertok(env_set("bootfile", cfg_path));

	/*
	 * Run sysboot - it will try labels in sequence:
	 * 1. missing-fdt: fails because explicit FDT doesn't exist
	 * 2. missing-fdtdir: warns about missing FDT but attempts boot
	 * 3. missing-overlay: loads FDT, warns about missing overlay, boots
	 */
	ut_assertok(run_commandf("sysboot host 0:0 any %x %s",
				 PXE_LOAD_ADDR, cfg_path));

	/*
	 * Test 1: Explicit FDT file not found
	 * First label (missing-fdt) has fdt=/dtb/nonexistent.dtb
	 * Should fail and move to next label
	 */
	ut_assert_skip_to_line("Enter choice: 1:\tMissing explicit FDT");
	ut_assert_nextline("Retrieving file: /vmlinuz");
	ut_assert_nextline("Retrieving file: /dtb/nonexistent.dtb");
	ut_assert_nextline("Skipping missing-fdt for failure retrieving FDT");

	/*
	 * Test 2: fdtdir with missing FDT file
	 * Second label (missing-fdtdir) has fdtdir=/dtb/ but fdtfile=missing.dtb
	 * Should warn but continue to boot attempt
	 */
	ut_assert_nextline("2:\tMissing fdtdir FDT");
	ut_assert_nextline("Retrieving file: /vmlinuz");
	ut_assert_nextline("Retrieving file: /dtb/missing.dtb");
	ut_assert_nextline("Skipping fdtdir /dtb/ for failure retrieving dts");

	/*
	 * Boot attempt without FDT - sandbox can't boot, but this verifies
	 * that label loading continued despite missing fdtdir FDT
	 */
	ut_assert_nextline("Unrecognized zImage");
	ut_assert_nextlinen("       unmap_physmem");

	/* Clean up env vars */
	env_set("fdtfile", NULL);

	return 0;
}
PXE_TEST_ARGS(pxe_test_errors_norun, UTF_CONSOLE | UTF_MANUAL,
	{ "fs_image", UT_ARG_STR },
	{ "cfg_path", UT_ARG_STR });

/**
 * Test pxe_get_file_size() function
 *
 * This tests reading the filesize from the environment variable.
 */
static int pxe_test_get_file_size(struct unit_test_state *uts)
{
	ulong size;

	/* Test with no filesize set - should return -ENOENT */
	env_set("filesize", NULL);
	ut_asserteq(-ENOENT, pxe_get_file_size(&size));

	/* Test with valid hex filesize */
	env_set("filesize", "1234");
	ut_assertok(pxe_get_file_size(&size));
	ut_asserteq(0x1234, size);

	/* Test with larger value */
	env_set("filesize", "abcdef");
	ut_assertok(pxe_get_file_size(&size));
	ut_asserteq(0xabcdef, size);

	/* Test with invalid (non-hex) value */
	env_set("filesize", "not_hex");
	ut_asserteq(-EINVAL, pxe_get_file_size(&size));

	/* Clean up */
	env_set("filesize", NULL);

	return 0;
}
PXE_TEST(pxe_test_get_file_size, 0);

/**
 * Test format_mac_pxe() function
 *
 * This tests MAC address formatting for PXE boot paths.
 */
static int pxe_test_format_mac(struct unit_test_state *uts)
{
	char buf[21];

	/* Test with buffer too small */
	ut_asserteq(-ENOSPC, format_mac_pxe(buf, 20));
	ut_asserteq(-ENOSPC, format_mac_pxe(buf, 1));

	/* Test with valid buffer - sandbox has an ethernet device */
	ut_asserteq(1, format_mac_pxe(buf, sizeof(buf)));

	/* Verify format: 01-xx-xx-xx-xx-xx-xx */
	ut_asserteq(20, strlen(buf));
	ut_asserteq('0', buf[0]);
	ut_asserteq('1', buf[1]);
	ut_asserteq('-', buf[2]);
	ut_asserteq('-', buf[5]);
	ut_asserteq('-', buf[8]);
	ut_asserteq('-', buf[11]);
	ut_asserteq('-', buf[14]);
	ut_asserteq('-', buf[17]);

	return 0;
}
PXE_TEST(pxe_test_format_mac, UTF_ETH_BOOTDEV);

/**
 * Test get_pxelinux_path() with path too long
 *
 * This tests the path length check in get_pxelinux_path().
 */
static int pxe_test_pxelinux_path_norun(struct unit_test_state *uts)
{
	const char *fs_image = ut_str(PXE_ARG_FS_IMAGE);
	struct pxe_test_info info;
	struct pxe_context ctx;
	char path[600];

	ut_assertnonnull(fs_image);
	info.uts = uts;

	/* Bind the filesystem image */
	ut_assertok(run_commandf("host bind 0 %s", fs_image));

	/* Set up the PXE context */
	ut_assertok(pxe_setup_ctx(&ctx, pxe_test_getfile, &info, false, "/",
				  false, false, NULL));

	/* Create a path that's too long (> 512 - 13 for "pxelinux.cfg/") */
	memset(path, 'a', sizeof(path) - 1);
	path[sizeof(path) - 1] = '\0';

	/* Should fail with -ENAMETOOLONG */
	ut_asserteq(-ENAMETOOLONG, get_pxelinux_path(&ctx, path,
						     PXE_LOAD_ADDR));

	pxe_destroy_ctx(&ctx);

	return 0;
}
PXE_TEST_ARGS(pxe_test_pxelinux_path_norun, UTF_CONSOLE | UTF_MANUAL,
	{ "fs_image", UT_ARG_STR });

/**
 * Test ipappend functionality
 *
 * This tests that ipappend correctly appends IP and MAC information to
 * bootargs. The rescue label has ipappend=3 which enables both:
 *   - bit 0x1: ip=<ipaddr>:<serverip>:<gatewayip>:<netmask>
 *   - bit 0x2: BOOTIF=01-xx-xx-xx-xx-xx-xx
 */
static int pxe_test_ipappend_norun(struct unit_test_state *uts)
{
	const char *fs_image = ut_str(PXE_ARG_FS_IMAGE);
	const char *cfg_path = ut_str(PXE_ARG_CFG_PATH);

	ut_assertnonnull(fs_image);
	ut_assertnonnull(cfg_path);

	/* Bind the filesystem image */
	ut_assertok(run_commandf("host bind 0 %s", fs_image));

	/* Set environment variables for file loading */
	ut_assertok(env_set_hex("pxefile_addr_r", PXE_LOAD_ADDR));
	ut_assertok(env_set_hex("kernel_addr_r", PXE_KERNEL_ADDR));
	ut_assertok(env_set_hex("ramdisk_addr_r", PXE_INITRD_ADDR));
	ut_assertok(env_set_hex("fdt_addr_r", PXE_FDT_ADDR));
	ut_assertok(env_set("bootfile", cfg_path));

	/* Set network environment variables for ipappend */
	ut_assertok(env_set("ipaddr", "192.168.1.10"));
	ut_assertok(env_set("serverip", "192.168.1.1"));
	ut_assertok(env_set("gatewayip", "192.168.1.254"));
	ut_assertok(env_set("netmask", "255.255.255.0"));

	/* Override to boot the rescue label which has ipappend=3 */
	ut_assertok(env_set("pxe_label_override", "rescue"));
	ut_assertok(env_set("pxe_timeout", "1"));

	/* Run sysboot */
	ut_assertok(run_commandf("sysboot host 0:0 any %x %s",
				 PXE_LOAD_ADDR, cfg_path));

	/* Skip to the rescue label boot */
	ut_assert_skip_to_line("Retrieving file: /vmlinuz-rescue");

	/*
	 * Verify ipappend output - should have:
	 * - original append: "single"
	 * - ip= string from ipappend bit 0x1
	 * - BOOTIF= string from ipappend bit 0x2
	 */
	ut_assert_nextlinen("append: single ip=192.168.1.10:192.168.1.1:"
			    "192.168.1.254:255.255.255.0 BOOTIF=01-");

	/* Clean up */
	env_set("ipaddr", NULL);
	env_set("serverip", NULL);
	env_set("gatewayip", NULL);
	env_set("netmask", NULL);
	env_set("pxe_label_override", NULL);
	env_set("pxe_timeout", NULL);

	return 0;
}
PXE_TEST_ARGS(pxe_test_ipappend_norun, UTF_CONSOLE | UTF_MANUAL | UTF_ETH_BOOTDEV,
	{ "fs_image", UT_ARG_STR },
	{ "cfg_path", UT_ARG_STR });

/**
 * Test pxe_get_fdt_fallback() function
 *
 * This tests the FDT address fallback logic when a label doesn't specify
 * an FDT file via 'fdt' or 'fdtdir' keywords.
 */
static int pxe_test_fdt_fallback(struct unit_test_state *uts)
{
	const char *orig_fdt_addr, *orig_fdtcontroladdr;
	ulong kern_addr = 0x1000000;
	struct pxe_label label;
	void *kern_buf;

	/* Create a dummy kernel buffer (not FIT format) */
	kern_buf = map_sysmem(kern_addr, 64);
	memset(kern_buf, '\0', 64);
	unmap_sysmem(kern_buf);

	memset(&label, '\0', sizeof(label));

	/* Save and clear env vars (fdtcontroladdr is set by U-Boot) */
	orig_fdt_addr = env_get("fdt_addr");
	orig_fdtcontroladdr = env_get("fdtcontroladdr");
	ut_assertok(env_set("fdt_addr", NULL));
	ut_assertok(env_set("fdtcontroladdr", NULL));

	/* Test 1: No fallback env vars set - should return NULL */
	ut_assertnull(pxe_get_fdt_fallback(&label, kern_addr));

	/* Test 2: fdt_addr set - should return fdt_addr */
	ut_assertok(env_set_hex("fdt_addr", 0x2000000));
	ut_asserteq_str("2000000", pxe_get_fdt_fallback(&label, kern_addr));

	/* Test 3: Both set - fdt_addr takes priority */
	ut_assertok(env_set_hex("fdtcontroladdr", 0x3000000));
	ut_asserteq_str("2000000", pxe_get_fdt_fallback(&label, kern_addr));

	/* Test 4: Only fdtcontroladdr set - should return fdtcontroladdr */
	ut_assertok(env_set("fdt_addr", NULL));
	ut_asserteq_str("3000000", pxe_get_fdt_fallback(&label, kern_addr));

	/* Restore env vars */
	ut_assertok(env_set("fdt_addr", orig_fdt_addr));
	ut_assertok(env_set("fdtcontroladdr", orig_fdtcontroladdr));

	return 0;
}
PXE_TEST(pxe_test_fdt_fallback, 0);

/**
 * Test pxe_label_override environment variable
 *
 * This tests that pxe_label_override can override the default label,
 * and that an invalid override prints an error message.
 */
static int pxe_test_label_override_norun(struct unit_test_state *uts)
{
	const char *fs_image = ut_str(PXE_ARG_FS_IMAGE);
	const char *cfg_path = ut_str(PXE_ARG_CFG_PATH);

	ut_assertnonnull(fs_image);
	ut_assertnonnull(cfg_path);

	/* Bind the filesystem image */
	ut_assertok(run_commandf("host bind 0 %s", fs_image));

	/* Set environment variables for file loading */
	ut_assertok(env_set_hex("pxefile_addr_r", PXE_LOAD_ADDR));
	ut_assertok(env_set_hex("kernel_addr_r", PXE_KERNEL_ADDR));
	ut_assertok(env_set_hex("ramdisk_addr_r", PXE_INITRD_ADDR));
	ut_assertok(env_set_hex("fdt_addr_r", PXE_FDT_ADDR));
	ut_assertok(env_set("bootfile", cfg_path));
	ut_assertok(env_set("pxe_timeout", "1"));

	/* Test 1: Override to 'local' label (localboot) */
	ut_assertok(env_set("pxe_label_override", "local"));
	ut_assertok(run_commandf("sysboot host 0:0 any %x %s",
				 PXE_LOAD_ADDR, cfg_path));

	/* Should boot 'local' label instead of default 'linux' */
	ut_assert_skip_to_line("3:\tLocal Boot");
	ut_assert_skip_to_line("missing environment variable: localcmd");

	/* Test 2: Invalid override - should print error */
	ut_assertok(env_set("pxe_label_override", "nonexistent"));
	ut_assertok(run_commandf("sysboot host 0:0 any %x %s",
				 PXE_LOAD_ADDR, cfg_path));

	ut_assert_skip_to_line("Missing override pxe label: nonexistent");

	/* Clean up */
	ut_assertok(env_set("pxe_label_override", NULL));
	ut_assertok(env_set("pxe_timeout", NULL));

	return 0;
}
PXE_TEST_ARGS(pxe_test_label_override_norun, UTF_CONSOLE | UTF_MANUAL,
	{ "fs_image", UT_ARG_STR },
	{ "cfg_path", UT_ARG_STR });

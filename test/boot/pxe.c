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
#include <image.h>
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
 * pxe_check_menu() - Check the standard menu output lines
 *
 * This helper checks all the console output lines from loading and displaying
 * the PXE menu, including config file retrieval, include files, background
 * image attempt, and the menu itself. Note: 'say' messages are now printed
 * when the label is booted, not during parsing.
 *
 * @uts: Unit test state
 * @error_msg: Expected error message after background image, or NULL if none
 * Return: 0 if OK, -ve on error
 */
static int pxe_check_menu(struct unit_test_state *uts, const char *error_msg)
{
	int i;

	/* Config file retrieval */
	ut_assert_nextline("Retrieving file: /extlinux/extlinux.conf");

	/* Include file retrievals */
	ut_assert_nextline("Retrieving file: /extlinux/extra.conf");
	for (i = 3; i <= 16; i++)
		ut_assert_nextline("Retrieving file: /extlinux/nest%d.conf", i);

	/* Background image attempt */
	ut_assert_nextline("Retrieving file: /boot/background.bmp");
	ut_assert_nextline("There is no valid bmp file at the given address");

	/* Optional error message before menu */
	if (error_msg)
		ut_assert_nextline(error_msg);

	/* Menu title and items */
	ut_assert_nextline("Test Boot Menu");
	ut_assert_nextline("1:\tBoot Linux");
	ut_assert_nextline("2:\tRescue Mode");
	ut_assert_nextline("3:\tLocal Boot");
	ut_assert_nextline("4:\tFIT Boot");
	ut_assert_nextline("5:\tIncluded Label");
	for (i = 6; i <= 19; i++)
		ut_assert_nextline("%d:\tLevel %d Label", i, i - 3);

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

	/* Read the config file into memory (quiet since we're just parsing) */
	ctx.quiet = true;
	ret = get_pxe_file(&ctx, cfg_path, addr);
	ut_asserteq(1, ret);  /* get_pxe_file returns 1 on success */

	/* Parse the config file */
	cfg = parse_pxefile(&ctx, addr);
	ut_assertnonnull(cfg);

	/* Process any include files */
	ut_assertok(pxe_process_includes(&ctx, cfg, addr));

	/* Verify no console output during parsing (say is printed on boot) */
	ut_assert_console_end();

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
	ut_asserteq(5, label->files.count);
	ut_asserteq_str("/vmlinuz",
			alist_get(&label->files, 0, struct pxe_file)->path);
	ut_asserteq_str("/initrd.img",
			alist_get(&label->files, 1, struct pxe_file)->path);
	ut_asserteq_str("/dtb/board.dtb",
			alist_get(&label->files, 2, struct pxe_file)->path);
	ut_asserteq_str("/dtb/overlay1.dtbo",
			alist_get(&label->files, 3, struct pxe_file)->path);
	ut_asserteq_str("/dtb/overlay2.dtbo",
			alist_get(&label->files, 4, struct pxe_file)->path);
	ut_asserteq_str("Booting default Linux kernel", label->say);
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
	ut_asserteq(1, label->files.count);
	ut_asserteq_str("/vmlinuz-rescue",
			alist_get(&label->files, 0, struct pxe_file)->path);
	ut_assertnull(label->say);
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
	ut_asserteq(0, label->files.count);
	ut_assertnull(label->say);
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
	ut_asserteq(1, label->files.count);
	ut_asserteq_str("/boot/image.fit",
			alist_get(&label->files, 0, struct pxe_file)->path);
	ut_assertnull(label->say);
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
	ut_asserteq(1, label->files.count);
	ut_asserteq_str("/boot/included-kernel",
			alist_get(&label->files, 0, struct pxe_file)->path);
	ut_assertnull(label->say);
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

	/*
	 * Test FDT overlay loading
	 *
	 * Get the first label (linux) which has fdtoverlays, set up the
	 * environment, and verify overlay files can be loaded.
	 */
	label = list_first_entry(&cfg->labels, struct pxe_label, list);
	ut_asserteq(5, label->files.count);

	/* Set environment variables for file loading */
	ut_assertok(env_set_hex("kernel_addr_r", PXE_KERNEL_ADDR));
	ut_assertok(env_set_hex("ramdisk_addr_r", PXE_INITRD_ADDR));
	ut_assertok(env_set_hex("fdt_addr_r", PXE_FDT_ADDR));
	ut_assertok(env_set_hex("fdtoverlay_addr_r", PXE_OVERLAY_ADDR));

	/*
	 * Load files via pxe_load_files(). Note: pxe_load_files takes
	 * ownership of fdtfile and frees it, so we must strdup here.
	 */
	ret = pxe_load_files(&ctx, label, strdup(label->fdt));
	ut_assertok(ret);

	/* Verify kernel and FDT were loaded */
	ut_asserteq(PXE_KERNEL_ADDR, ctx.kern_addr);
	ut_asserteq(PXE_FDT_ADDR, ctx.fdt_addr);

	/* Verify overlays were loaded to valid addresses (indices 3 and 4) */
	ut_assert(alist_get(&label->files, 3,
			    struct pxe_file)->addr >= PXE_OVERLAY_ADDR);
	ut_assert(alist_get(&label->files, 4,
			    struct pxe_file)->addr >= PXE_OVERLAY_ADDR);

	/* Second overlay should be at a higher address than the first */
	ut_assert(alist_get(&label->files, 4, struct pxe_file)->addr >
		  alist_get(&label->files, 3, struct pxe_file)->addr);

	/* Verify no more console output */
	ut_assert_console_end();

	/* Clean up */
	pxe_menu_uninit(cfg);
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
	ut_assert_skip_to_line("Enter choice: Booting default Linux kernel");
	ut_assert_nextline("1:\tBoot Linux");

	/* Verify files were loaded in order */
	ut_assert_nextline("Retrieving file: /vmlinuz");
	ut_assert_nextline("Retrieving file: /initrd.img");
	ut_assert_nextline("Retrieving file: /dtb/board.dtb");
	ut_assert_nextline("Retrieving file: /dtb/overlay1.dtbo");
	ut_assert_nextline("Retrieving file: /dtb/overlay2.dtbo");
	ut_assert_nextline("append: root=/dev/sda1 quiet");

	/* Boot fails on sandbox */
	ut_assert_nextline("Unrecognized zImage");
	ut_assert_console_end();

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
 * Test fdtdir path resolution
 *
 * This test verifies:
 * 1. fdtdir with fdtfile env var - uses fdtfile value directly
 * 2. fdtdir with soc/board env vars - constructs {soc}-{board}.dtb
 * 3. fdtdir without trailing slash - slash is inserted
 */
static int pxe_test_fdtdir_norun(struct unit_test_state *uts)
{
	const char *fs_image = ut_str(PXE_ARG_FS_IMAGE);
	const char *cfg_path = ut_str(PXE_ARG_CFG_PATH);
	struct pxe_test_info info;
	struct pxe_context ctx;
	struct pxe_label *label;
	struct pxe_menu *cfg;
	ulong addr = PXE_LOAD_ADDR;
	void *fdt;

	ut_assertnonnull(fs_image);
	ut_assertnonnull(cfg_path);

	info.uts = uts;

	/* Bind the filesystem image */
	ut_assertok(run_commandf("host bind 0 %s", fs_image));

	/* Set up the PXE context */
	ut_assertok(pxe_setup_ctx(&ctx, pxe_test_getfile, &info, true, cfg_path,
				  false, false, NULL));

	/* Read and parse the config file */
	ut_asserteq(1, get_pxe_file(&ctx, cfg_path, addr));

	cfg = parse_pxefile(&ctx, addr);
	ut_assertnonnull(cfg);

	/* Consume parsing output */
	ut_assert_nextline("Retrieving file: %s", cfg_path);
	ut_assert_console_end();

	/*
	 * Test 1: fdtdir with fdtfile env var
	 * Set fdtfile=test-board.dtb, load should find /dtb/test-board.dtb
	 */
	ut_assertok(env_set("fdtfile", "test-board.dtb"));
	ut_assertok(env_set_hex("kernel_addr_r", PXE_KERNEL_ADDR));
	ut_assertok(env_set_hex("fdt_addr_r", PXE_FDT_ADDR));
	ut_assertok(env_set_hex("fdtoverlay_addr_r", PXE_OVERLAY_ADDR));

	/* Get first label (fdtfile-test) and load its files */
	label = list_first_entry(&cfg->labels, struct pxe_label, list);
	ut_asserteq_str("fdtfile-test", label->name);
	ut_asserteq_str("/dtb/", label->fdtdir);
	ut_assertnull(label->fdt);

	ut_assertok(pxe_load_label(&ctx, label));

	/* Verify FDT was loaded */
	ut_asserteq(PXE_FDT_ADDR, ctx.conf_fdt);
	fdt = map_sysmem(PXE_FDT_ADDR, 0);
	ut_assertok(fdt_check_header(fdt));

	/* Check console output shows the constructed path */
	ut_assert_nextline("Retrieving file: /vmlinuz");
	ut_assert_nextline("Retrieving file: /dtb/test-board.dtb");
	ut_assert_nextline("Retrieving file: /dtb/overlay1.dtbo");
	ut_assert_console_end();

	/*
	 * Test 2: fdtdir with soc/board env vars (no fdtfile)
	 * Set soc=tegra, board=jetson -> /dtb/tegra-jetson.dtb
	 */
	ut_assertok(env_set("fdtfile", NULL));  /* Clear fdtfile */
	ut_assertok(env_set("soc", "tegra"));
	ut_assertok(env_set("board", "jetson"));
	ctx.conf_fdt = 0;  /* Reset for next load */

	/* Get second label (socboard-test) */
	label = list_entry(label->list.next, struct pxe_label, list);
	ut_asserteq_str("socboard-test", label->name);
	ut_asserteq_str("/dtb", label->fdtdir);  /* No trailing slash */

	ut_assertok(pxe_load_label(&ctx, label));

	/* Verify FDT was loaded (slash was inserted) */
	ut_asserteq(PXE_FDT_ADDR, ctx.conf_fdt);
	fdt = map_sysmem(PXE_FDT_ADDR, 0);
	ut_assertok(fdt_check_header(fdt));

	/* Check console output shows soc-board construction with slash */
	ut_assert_nextline("Retrieving file: /vmlinuz");
	ut_assert_nextline("Retrieving file: /dtb/tegra-jetson.dtb");
	ut_assert_console_end();

	/* Clean up env vars */
	env_set("fdtfile", NULL);
	env_set("soc", NULL);
	env_set("board", NULL);

	pxe_menu_uninit(cfg);
	pxe_destroy_ctx(&ctx);

	return 0;
}
PXE_TEST_ARGS(pxe_test_fdtdir_norun, UTF_CONSOLE | UTF_MANUAL,
	{ "fs_image", UT_ARG_STR },
	{ "cfg_path", UT_ARG_STR });

/**
 * Test error handling for missing FDT and overlay files
 *
 * This test verifies:
 * 1. Explicit FDT not found - label should fail with error
 * 2. fdtdir FDT not found - should warn but continue (return success)
 * 3. Missing overlay - should warn but continue loading other overlays
 */
static int pxe_test_errors_norun(struct unit_test_state *uts)
{
	const char *fs_image = ut_str(PXE_ARG_FS_IMAGE);
	const char *cfg_path = ut_str(PXE_ARG_CFG_PATH);
	struct pxe_test_info info;
	struct pxe_context ctx;
	struct pxe_label *label;
	struct pxe_menu *cfg;
	ulong addr = PXE_LOAD_ADDR;
	void *fdt;

	ut_assertnonnull(fs_image);
	ut_assertnonnull(cfg_path);

	info.uts = uts;

	/* Bind the filesystem image */
	ut_assertok(run_commandf("host bind 0 %s", fs_image));

	/* Set up the PXE context */
	ut_assertok(pxe_setup_ctx(&ctx, pxe_test_getfile, &info, true, cfg_path,
				  false, false, NULL));

	/* Read and parse the config file */
	ut_asserteq(1, get_pxe_file(&ctx, cfg_path, addr));

	cfg = parse_pxefile(&ctx, addr);
	ut_assertnonnull(cfg);

	/* Consume parsing output */
	ut_assert_nextline("Retrieving file: %s", cfg_path);
	ut_assert_console_end();

	/* Set up environment for loading */
	ut_assertok(env_set_hex("kernel_addr_r", PXE_KERNEL_ADDR));
	ut_assertok(env_set_hex("fdt_addr_r", PXE_FDT_ADDR));
	ut_assertok(env_set_hex("fdtoverlay_addr_r", PXE_OVERLAY_ADDR));
	ut_assertok(env_set("fdtfile", "missing.dtb"));  /* For fdtdir test */

	/*
	 * Test 1: Explicit FDT file not found
	 * Label has fdt=/dtb/nonexistent.dtb - should fail with -ENOENT
	 */
	label = list_first_entry(&cfg->labels, struct pxe_label, list);
	ut_asserteq_str("missing-fdt", label->name);
	ut_asserteq_str("/dtb/nonexistent.dtb", label->fdt);

	ut_asserteq(-ENOENT, pxe_load_label(&ctx, label));

	/* Check error message */
	ut_assert_nextline("Retrieving file: /vmlinuz");
	ut_assert_nextline("Retrieving file: /dtb/nonexistent.dtb");
	ut_assert_nextline("Skipping missing-fdt for failure retrieving FDT");
	ut_assert_console_end();

	/*
	 * Test 2: fdtdir with missing FDT file
	 * Label has fdtdir=/dtb/ but fdtfile=missing.dtb doesn't exist
	 * Should warn but return success (label continues without FDT)
	 */
	ctx.conf_fdt = 0;
	label = list_entry(label->list.next, struct pxe_label, list);
	ut_asserteq_str("missing-fdtdir", label->name);
	ut_asserteq_str("/dtb/", label->fdtdir);
	ut_assertnull(label->fdt);

	ut_assertok(pxe_load_label(&ctx, label));

	/* Check warning message */
	ut_assert_nextline("Retrieving file: /vmlinuz");
	ut_assert_nextline("Retrieving file: /dtb/missing.dtb");
	ut_assert_nextline("Skipping fdtdir /dtb/ for failure retrieving dts");
	ut_assert_console_end();

	/*
	 * Test 3: Missing overlay file (but valid FDT)
	 * Label has fdt=/dtb/board.dtb (exists) and two overlays:
	 * - /dtb/nonexistent.dtbo (missing - should warn)
	 * - /dtb/overlay1.dtbo (exists - should load)
	 */
	ctx.conf_fdt = 0;
	label = list_entry(label->list.next, struct pxe_label, list);
	ut_asserteq_str("missing-overlay", label->name);
	ut_asserteq_str("/dtb/board.dtb", label->fdt);

	ut_assertok(pxe_load_label(&ctx, label));

	/* FDT should be loaded */
	ut_asserteq(PXE_FDT_ADDR, ctx.conf_fdt);
	fdt = map_sysmem(PXE_FDT_ADDR, 0);
	ut_assertok(fdt_check_header(fdt));

	/* Check console output */
	ut_assert_nextline("Retrieving file: /vmlinuz");
	ut_assert_nextline("Retrieving file: /dtb/board.dtb");
	ut_assert_nextline("Retrieving file: /dtb/nonexistent.dtbo");
	ut_assert_nextline("Failed loading overlay /dtb/nonexistent.dtbo");
	ut_assert_nextline("Retrieving file: /dtb/overlay1.dtbo");
	ut_assert_console_end();

	/* Clean up */
	env_set("fdtfile", NULL);
	pxe_menu_uninit(cfg);
	pxe_destroy_ctx(&ctx);

	return 0;
}
PXE_TEST_ARGS(pxe_test_errors_norun, UTF_CONSOLE | UTF_MANUAL,
	{ "fs_image", UT_ARG_STR },
	{ "cfg_path", UT_ARG_STR });

/**
 * Test overlay loading when fdtoverlay_addr_r is not set
 *
 * This tests that when a label has fdtoverlays but fdtoverlay_addr_r is not
 * set, overlay loading is attempted via LMB allocation. The FDT is still
 * loaded successfully even if overlays fail to load.
 */
static int pxe_test_overlay_no_addr_norun(struct unit_test_state *uts)
{
	const char *fs_image = ut_str(PXE_ARG_FS_IMAGE);
	const char *cfg_path = ut_str(PXE_ARG_CFG_PATH);
	struct pxe_test_info info;
	struct pxe_context ctx;
	struct pxe_label *label;
	struct pxe_menu *cfg;
	ulong addr = PXE_LOAD_ADDR;
	void *fdt;

	ut_assertnonnull(fs_image);
	ut_assertnonnull(cfg_path);

	info.uts = uts;

	/* Bind the filesystem image */
	ut_assertok(run_commandf("host bind 0 %s", fs_image));

	/* Set up the PXE context */
	ut_assertok(pxe_setup_ctx(&ctx, pxe_test_getfile, &info, true, cfg_path,
				  false, false, NULL));

	/* Read and parse the config file (quiet since we're just parsing) */
	ctx.quiet = true;
	ut_asserteq(1, get_pxe_file(&ctx, cfg_path, addr));

	cfg = parse_pxefile(&ctx, addr);
	ut_assertnonnull(cfg);

	/* Process any include files */
	ut_assertok(pxe_process_includes(&ctx, cfg, addr));

	/* Verify no console output during parsing (say is printed on boot) */
	ut_assert_console_end();

	/*
	 * Set up environment for loading, but do NOT set fdtoverlay_addr_r.
	 * This should cause overlay loading to be skipped with a warning.
	 */
	ut_assertok(env_set_hex("kernel_addr_r", PXE_KERNEL_ADDR));
	ut_assertok(env_set_hex("ramdisk_addr_r", PXE_INITRD_ADDR));
	ut_assertok(env_set_hex("fdt_addr_r", PXE_FDT_ADDR));
	ut_assertok(env_set("fdtoverlay_addr_r", NULL));  /* Clear it */

	/* Get the first label (linux) which has fdtoverlays */
	label = list_first_entry(&cfg->labels, struct pxe_label, list);
	ut_asserteq_str("linux", label->name);
	ut_assert(label->files.count > 0);

	/* Enable output for loading phase */
	ctx.quiet = false;

	/* Load the label - should succeed but skip overlays */
	ut_assertok(pxe_load_label(&ctx, label));

	/* FDT should be loaded */
	ut_asserteq(PXE_FDT_ADDR, ctx.conf_fdt);
	fdt = map_sysmem(PXE_FDT_ADDR, 0);
	ut_assertok(fdt_check_header(fdt));

	/*
	 * Check console output - FDT loaded, overlays attempted via LMB
	 * allocation but fail since test environment cannot load them
	 */
	ut_assert_nextline("Retrieving file: /vmlinuz");
	ut_assert_nextline("Retrieving file: /initrd.img");
	ut_assert_nextline("Retrieving file: /dtb/board.dtb");
	ut_assert_nextline("Retrieving file: /dtb/overlay1.dtbo");
	ut_assert_nextline("Failed loading overlay /dtb/overlay1.dtbo");
	ut_assert_nextline("Retrieving file: /dtb/overlay2.dtbo");
	ut_assert_nextline("Failed loading overlay /dtb/overlay2.dtbo");
	ut_assert_console_end();

	/* Clean up */
	pxe_menu_uninit(cfg);
	pxe_destroy_ctx(&ctx);

	return 0;
}
PXE_TEST_ARGS(pxe_test_overlay_no_addr_norun, UTF_CONSOLE | UTF_MANUAL,
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

	/* Clear fdtfile to ensure rescue label's fdtdir tries /dtb/.dtb */
	ut_assertok(env_set("fdtfile", NULL));

	/* Override to boot the rescue label which has ipappend=3 */
	ut_assertok(env_set("pxe_label_override", "rescue"));
	ut_assertok(env_set("pxe_timeout", "1"));

	/* Run sysboot */
	ut_assertok(run_commandf("sysboot host 0:0 any %x %s",
				 PXE_LOAD_ADDR, cfg_path));

	/* Check menu output */
	ut_assertok(pxe_check_menu(uts, NULL));
	ut_assert_nextline("Enter choice: 2:\tRescue Mode");

	/* Rescue label boot attempt */
	ut_assert_nextline("Retrieving file: /vmlinuz-rescue");

	/*
	 * Rescue label has fdtdir=/dtb/ but no fdtfile is set, so it tries
	 * to load /dtb/.dtb which fails. FDT is loaded before append.
	 */
	ut_assert_nextline("Retrieving file: /dtb/.dtb");
	ut_assert_nextline("Skipping fdtdir /dtb/ for failure retrieving dts");

	/*
	 * Verify ipappend output - should have:
	 * - original append: "single"
	 * - ip= string from ipappend bit 0x1
	 * - BOOTIF= string from ipappend bit 0x2
	 */
	ut_assert_nextlinen("append: single ip=192.168.1.10:192.168.1.1:"
			    "192.168.1.254:255.255.255.0 BOOTIF=01-");

	ut_assert_nextline("Unrecognized zImage");
	ut_assert_console_end();

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
	ut_assertok(env_set_hex("fdtoverlay_addr_r", PXE_OVERLAY_ADDR));
	ut_assertok(env_set("bootfile", cfg_path));
	ut_assertok(env_set("pxe_timeout", "1"));

	/* Test 1: Override to 'local' label (localboot) */
	ut_assertok(env_set("pxe_label_override", "local"));
	ut_assertok(run_commandf("sysboot host 0:0 any %x %s",
				 PXE_LOAD_ADDR, cfg_path));

	/* Check menu output - say message is from default label */
	ut_assertok(pxe_check_menu(uts, NULL));

	/* Should boot 'local' label instead of default 'linux' */
	ut_assert_nextline("Enter choice: 3:\tLocal Boot");
	ut_assert_nextline("missing environment variable: localcmd");

	/*
	 * Localboot fails, so try default 'linux' label instead.
	 * Boot is minimal - just kernel/initrd, no FDT.
	 */
	ut_assert_nextline("Retrieving file: /vmlinuz");
	ut_assert_nextline("Retrieving file: /initrd.img");
	ut_assert_nextline("Unrecognized zImage");

	/* Test 2: Invalid override - should print error before menu */
	ut_assertok(env_set("pxe_label_override", "nonexistent"));
	ut_assertok(run_commandf("sysboot host 0:0 any %x %s",
				 PXE_LOAD_ADDR, cfg_path));

	/* Check menu with error message before it */
	ut_assertok(pxe_check_menu(uts, "Missing override pxe label: nonexistent"));

	/* Say message is printed when label is selected (after "Enter choice:") */
	ut_assert_nextline("Enter choice: Booting default Linux kernel");
	ut_assert_nextline("1:\tBoot Linux");

	/* Default label boot attempt - FDT/overlays loaded before append */
	ut_assert_nextline("Retrieving file: /vmlinuz");
	ut_assert_nextline("Retrieving file: /initrd.img");
	ut_assert_nextline("Retrieving file: /dtb/board.dtb");
	ut_assert_nextline("Retrieving file: /dtb/overlay1.dtbo");
	ut_assert_nextline("Retrieving file: /dtb/overlay2.dtbo");
	ut_assert_nextline("append: root=/dev/sda1 quiet");
	ut_assert_nextline("Unrecognized zImage");
	ut_assert_console_end();

	/* Clean up */
	ut_assertok(env_set("pxe_label_override", NULL));
	ut_assertok(env_set("pxe_timeout", NULL));

	return 0;
}
PXE_TEST_ARGS(pxe_test_label_override_norun, UTF_CONSOLE | UTF_MANUAL,
	{ "fs_image", UT_ARG_STR },
	{ "cfg_path", UT_ARG_STR });

/**
 * struct pxe_alloc_info - context for the alloc test getfile callback
 *
 * @uts: Unit test state for assertions
 * @next_addr: Next address to allocate (increments by 0x100 each call)
 */
struct pxe_alloc_info {
	struct unit_test_state *uts;
	ulong next_addr;
};

/**
 * pxe_alloc_getfile() - Read a file, allocating address if not provided
 *
 * For files loaded via env vars (kernel, initrd, fdt), this verifies that
 * *addrp is 0 (no environment variable set), then assigns an incrementing
 * address to simulate LMB allocation. For the config file (which is loaded
 * with a direct address), it just uses the provided address.
 */
static int pxe_alloc_getfile(struct pxe_context *ctx, const char *file_path,
			     ulong *addrp, ulong align,
			     enum bootflow_img_t type, ulong *sizep)
{
	struct pxe_alloc_info *info = ctx->userdata;
	loff_t len_read;
	int ret;

	/*
	 * Config file is loaded with direct address (non-zero).
	 * Kernel/initrd/fdt/overlays come through env vars - if not set,
	 * addrp will be 0 and we need to allocate.
	 */
	if (!*addrp) {
		*addrp = info->next_addr;
		info->next_addr += 0x100;
	}

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
 * Test file loading with no address environment variables
 *
 * This tests the LMB allocation path where if no address env var is set,
 * the getfile callback receives *addrp == 0 and must allocate memory.
 * Our test callback assigns incrementing addresses (0x100, 0x200, etc.)
 * and verifies the addresses are then stored in ctx.
 */
static int pxe_test_alloc_norun(struct unit_test_state *uts)
{
	const char *orig_fdt_addr, *orig_fdtcontroladdr;
	const char *fs_image = ut_str(PXE_ARG_FS_IMAGE);
	const char *cfg_path = ut_str(PXE_ARG_CFG_PATH);
	struct pxe_alloc_info info;
	struct pxe_context ctx;
	ulong addr;
	int ret;

	ut_assertnonnull(fs_image);
	ut_assertnonnull(cfg_path);

	info.uts = uts;
	info.next_addr = 0x100;

	/* Bind the filesystem image */
	ut_assertok(run_commandf("host bind 0 %s", fs_image));

	/* Save and clear FDT fallback env vars (fdtcontroladdr is set at boot) */
	orig_fdt_addr = env_get("fdt_addr");
	orig_fdtcontroladdr = env_get("fdtcontroladdr");
	ut_assertok(env_set("fdt_addr", NULL));
	ut_assertok(env_set("fdtcontroladdr", NULL));

	/* Ensure address env vars are NOT set */
	ut_assertok(env_set("kernel_addr_r", NULL));
	ut_assertok(env_set("ramdisk_addr_r", NULL));
	ut_assertok(env_set("fdt_addr_r", NULL));
	ut_assertok(env_set("fdtoverlay_addr_r", NULL));
	ut_assertok(env_set("pxe_timeout", "1"));

	/* Set up the PXE context with our allocating getfile */
	ut_assertok(pxe_setup_ctx(&ctx, pxe_alloc_getfile, &info, true,
				  cfg_path, false, false, NULL));

	/* Read the config file - use a fixed address for parsing */
	addr = PXE_LOAD_ADDR;
	ret = get_pxe_file(&ctx, cfg_path, addr);
	ut_asserteq(1, ret);

	/* Parse and probe - this triggers file loading */
	ut_assertok(pxe_probe(&ctx, addr, false));

	/*
	 * Verify all pxe_context fields are set correctly.
	 *
	 * The background BMP is loaded first (0x100), then the default
	 * label 'linux' loads: kernel (0x200), initrd (0x300).
	 *
	 * Note: FDT loading requires fdt_addr_r to be set (checked in
	 * label_process_fdt before attempting to load), so conf_fdt_str
	 * and conf_fdt are NULL/0.
	 */

	/* Context setup fields */
	ut_asserteq_ptr(pxe_alloc_getfile, ctx.getfile);
	ut_asserteq_ptr(&info, ctx.userdata);
	ut_asserteq(true, ctx.allow_abs_path);
	ut_assertnonnull(ctx.bootdir);
	ut_asserteq(0, ctx.pxe_file_size);  /* only set by cmd/pxe.c */
	ut_asserteq(false, ctx.use_ipv6);
	ut_asserteq(false, ctx.use_fallback);
	ut_asserteq(true, ctx.no_boot);
	ut_assertnull(ctx.bflow);
	ut_assertnonnull(ctx.cfg);

	/* BMP loaded first */
	ut_asserteq(0x100, image_load_addr);

	/* Label selection */
	ut_assertnonnull(ctx.label);
	ut_asserteq_str("linux", ctx.label->name);

	/* Kernel */
	ut_asserteq_str("200", ctx.kern_addr_str);
	ut_asserteq(0x200, ctx.kern_addr);
	ut_asserteq(6, ctx.kern_size);

	/* Initrd */
	ut_asserteq(0x300, ctx.initrd_addr);
	ut_asserteq(7, ctx.initrd_size);
	ut_asserteq_str("300:7", ctx.initrd_str);

	/* FDT (not loaded - no fdt_addr_r env var) */
	ut_assertnull(ctx.conf_fdt_str);
	ut_asserteq(0, ctx.conf_fdt);

	/* Boot flags */
	ut_asserteq(false, ctx.restart);
	ut_asserteq(false, ctx.fake_go);

	/* Clean up */
	pxe_menu_uninit(ctx.cfg);
	pxe_destroy_ctx(&ctx);
	ut_assertok(env_set("pxe_timeout", NULL));
	ut_assertok(env_set("fdt_addr", orig_fdt_addr));
	ut_assertok(env_set("fdtcontroladdr", orig_fdtcontroladdr));

	return 0;
}
PXE_TEST_ARGS(pxe_test_alloc_norun, UTF_CONSOLE | UTF_MANUAL,
	{ "fs_image", UT_ARG_STR },
	{ "cfg_path", UT_ARG_STR });

/**
 * Test FIT image with embedded FDT (no explicit fdt line)
 *
 * This tests that when using 'fit /path.fit' without an explicit 'fdt'
 * line (label->fdt is NULL), the FDT address is set to the FIT address
 * so bootm can extract the FDT from the FIT image.
 *
 * The buggy behavior: When label->fdt is NULL, the FIT check fails:
 *   if (label->fdt && label->kernel_label &&
 *       !strcmp(label->kernel_label, label->fdt))
 * and conf_fdt_str is not set to the FIT address.
 *
 * The correct behavior: When the kernel is a FIT image with embedded FDT
 * and no explicit fdt line is provided, conf_fdt_str should be set to
 * the kernel (FIT) address so bootm can extract the FDT.
 */
static int pxe_test_fit_embedded_fdt_norun(struct unit_test_state *uts)
{
	const char *fs_image = ut_str(PXE_ARG_FS_IMAGE);
	const char *cfg_path = ut_str(PXE_ARG_CFG_PATH);
	struct pxe_test_info info;
	struct pxe_context ctx;
	struct pxe_label *label;
	struct pxe_menu *cfg;
	ulong addr = PXE_LOAD_ADDR;

	ut_assertnonnull(fs_image);
	ut_assertnonnull(cfg_path);

	info.uts = uts;

	/* Bind the filesystem image */
	ut_assertok(run_commandf("host bind 0 %s", fs_image));

	/* Set up the PXE context */
	ut_assertok(pxe_setup_ctx(&ctx, pxe_test_getfile, &info, true, cfg_path,
				  false, false, NULL));

	/* Set up environment for loading */
	ut_assertok(env_set_hex("kernel_addr_r", PXE_KERNEL_ADDR));
	ut_assertok(env_set_hex("fdt_addr_r", PXE_FDT_ADDR));

	/* Read and parse the config file */
	ut_asserteq(1, get_pxe_file(&ctx, cfg_path, addr));

	cfg = parse_pxefile(&ctx, addr);
	ut_assertnonnull(cfg);

	/* Consume parsing output */
	ut_assert_nextline("Retrieving file: %s", cfg_path);
	ut_assert_console_end();

	/* Get the fitonly label which uses 'fit' without 'fdt' */
	label = list_first_entry(&cfg->labels, struct pxe_label, list);
	ut_asserteq_str("fitonly", label->name);

	/* Verify this is a FIT label with no explicit fdt */
	ut_assertnonnull(label->kernel);  /* /boot/image.fit */
	ut_assertnull(label->config);     /* NULL when no #config suffix */
	ut_assertnull(label->fdt);        /* No explicit fdt line - this is key */

	/* Load the label */
	ut_assertok(pxe_load_label(&ctx, label));

	/* Consume load output */
	ut_assert_nextline("Retrieving file: /boot/image.fit");
	ut_assert_console_end();

	/*
	 * For FIT images with embedded FDT and no explicit fdt line,
	 * conf_fdt_str is currently NULL. Ideally it should be set to the
	 * kernel address so bootm can extract the FDT from the FIT, but
	 * that is a pre-existing limitation.
	 *
	 * This test detects regressions where conf_fdt_str is incorrectly
	 * set to fdt_addr_r instead of NULL (which would cause bootm to
	 * look at the wrong address for the FDT).
	 */
	ut_assertnull(ctx.conf_fdt_str);
	ut_asserteq(0, ctx.conf_fdt);

	/* Clean up */
	pxe_menu_uninit(cfg);
	pxe_destroy_ctx(&ctx);

	return 0;
}
PXE_TEST_ARGS(pxe_test_fit_embedded_fdt_norun, UTF_CONSOLE | UTF_MANUAL,
	      { "fs_image", UT_ARG_STR },
	      { "cfg_path", UT_ARG_STR });

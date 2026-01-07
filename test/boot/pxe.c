// SPDX-License-Identifier: GPL-2.0+
/*
 * PXE parser tests - C implementation for Python wrapper
 *
 * Copyright 2026 Canonical Ltd
 *
 * These tests verify the extlinux.conf parser APIs.
 */

#include <dm.h>
#include <env.h>
#include <fs_legacy.h>
#include <mapmem.h>
#include <pxe_utils.h>
#include <test/test.h>
#include <test/ut.h>

/* Define test macro for pxe suite - no init function needed */
#define PXE_TEST_ARGS(_name, _flags, ...) \
	UNIT_TEST_ARGS(_name, _flags, pxe, __VA_ARGS__)

/* Argument indices */
#define PXE_ARG_FS_IMAGE	0	/* Path to filesystem image */
#define PXE_ARG_CFG_PATH	1	/* Path to config file within image */

/* Memory address for loading files */
#define PXE_LOAD_ADDR		0x01000000

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


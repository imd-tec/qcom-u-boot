// SPDX-License-Identifier: GPL-2.0+
/*
 * Test for Boot Loader Specification (BLS) parser
 *
 * Copyright 2026 Canonical Ltd
 * Written by Simon Glass <simon.glass@canonical.com>
 */

#include <bls.h>
#include <test/ut.h>

/* Test basic BLS entry parsing */
static int bls_test_parse_basic(struct unit_test_state *uts)
{
	struct bls_entry entry;
	char buf[] =
		"title Test Entry\n"
		"version 1.0\n"
		"linux /vmlinuz\n"
		"options root=/dev/sda\n"
		"initrd /initrd.img\n"
		"devicetree /test.dtb\n";

	ut_assertok(bls_parse_entry(buf, sizeof(buf) - 1, &entry));
	ut_asserteq_str("Test Entry", entry.title);
	ut_asserteq_str("1.0", entry.version);
	ut_asserteq_str("/vmlinuz", entry.kernel);
	ut_asserteq_str("root=/dev/sda", entry.options);
	ut_asserteq(1, entry.initrds.count);
	ut_asserteq_str("/test.dtb", entry.devicetree);

	bls_entry_uninit(&entry);

	return 0;
}
UNIT_TEST(bls_test_parse_basic, 0, bootstd);

/* Test multiple options lines are concatenated */
static int bls_test_parse_multi_options(struct unit_test_state *uts)
{
	struct bls_entry entry;
	char buf[] =
		"title Test\n"
		"linux /vmlinuz\n"
		"options root=/dev/sda\n"
		"options quiet splash\n";

	ut_assertok(bls_parse_entry(buf, sizeof(buf) - 1, &entry));
	ut_asserteq_str("root=/dev/sda quiet splash", entry.options);

	bls_entry_uninit(&entry);

	return 0;
}
UNIT_TEST(bls_test_parse_multi_options, 0, bootstd);

/* Test multiple initrd lines */
static int bls_test_parse_multi_initrd(struct unit_test_state *uts)
{
	struct bls_entry entry;
	char buf[] =
		"title Test\n"
		"linux /vmlinuz\n"
		"initrd /initrd1.img\n"
		"initrd /initrd2.img\n";
	const char **initrd;

	ut_assertok(bls_parse_entry(buf, sizeof(buf) - 1, &entry));
	ut_asserteq(2, entry.initrds.count);

	initrd = alist_get(&entry.initrds, 0, char *);
	ut_asserteq_str("/initrd1.img", *initrd);

	initrd = alist_get(&entry.initrds, 1, char *);
	ut_asserteq_str("/initrd2.img", *initrd);

	bls_entry_uninit(&entry);

	return 0;
}
UNIT_TEST(bls_test_parse_multi_initrd, 0, bootstd);

/* Test comments and blank lines are ignored */
static int bls_test_parse_comments(struct unit_test_state *uts)
{
	struct bls_entry entry;
	char buf[] =
		"# This is a comment\n"
		"title Test\n"
		"\n"
		"# Another comment\n"
		"linux /vmlinuz\n"
		"\n";

	ut_assertok(bls_parse_entry(buf, sizeof(buf) - 1, &entry));
	ut_asserteq_str("Test", entry.title);
	ut_asserteq_str("/vmlinuz", entry.kernel);

	bls_entry_uninit(&entry);

	return 0;
}
UNIT_TEST(bls_test_parse_comments, 0, bootstd);

/* Test missing required field returns error */
static int bls_test_parse_missing_kernel(struct unit_test_state *uts)
{
	struct bls_entry entry;
	char buf[] =
		"title Test\n"
		"options root=/dev/sda\n";

	ut_asserteq(-EINVAL, bls_parse_entry(buf, sizeof(buf) - 1, &entry));

	bls_entry_uninit(&entry);

	return 0;
}
UNIT_TEST(bls_test_parse_missing_kernel, 0, bootstd);

/* Test unknown fields are ignored */
static int bls_test_parse_unknown_field(struct unit_test_state *uts)
{
	struct bls_entry entry;
	char buf[] =
		"title Test\n"
		"linux /vmlinuz\n"
		"unknown_field some_value\n"
		"another_unknown 123\n";

	ut_assertok(bls_parse_entry(buf, sizeof(buf) - 1, &entry));
	ut_asserteq_str("Test", entry.title);

	bls_entry_uninit(&entry);

	return 0;
}
UNIT_TEST(bls_test_parse_unknown_field, 0, bootstd);

/* Test FIT-only entry (no linux field) */
static int bls_test_parse_fit(struct unit_test_state *uts)
{
	struct bls_entry entry;
	char buf[] =
		"title FIT Test\n"
		"version 1.0\n"
		"fit /boot/image.fit\n"
		"options root=/dev/sda\n"
		"initrd /initrd.img\n";

	ut_assertok(bls_parse_entry(buf, sizeof(buf) - 1, &entry));
	ut_asserteq_str("FIT Test", entry.title);
	ut_assertnull(entry.kernel);
	ut_asserteq_str("/boot/image.fit", entry.fit);
	ut_asserteq_str("root=/dev/sda", entry.options);
	ut_asserteq(1, entry.initrds.count);

	bls_entry_uninit(&entry);

	return 0;
}
UNIT_TEST(bls_test_parse_fit, 0, bootstd);

/* Test all supported fields */
static int bls_test_parse_all_fields(struct unit_test_state *uts)
{
	struct bls_entry entry;
	char buf[] =
		"title Full Test\n"
		"version 2.0.1\n"
		"linux /boot/vmlinuz-2.0.1\n"
		"options root=/dev/sda1 ro\n"
		"options quiet splash\n"
		"initrd /boot/initrd-2.0.1.img\n"
		"devicetree /boot/dtb-2.0.1.dtb\n"
		"devicetree-overlay /boot/overlay.dtbo\n"
		"architecture x86_64\n"
		"machine-id abc123\n"
		"sort-key 001\n";
	const char **initrd;

	ut_assertok(bls_parse_entry(buf, sizeof(buf) - 1, &entry));
	ut_asserteq_str("Full Test", entry.title);
	ut_asserteq_str("2.0.1", entry.version);
	ut_asserteq_str("/boot/vmlinuz-2.0.1", entry.kernel);
	ut_asserteq_str("root=/dev/sda1 ro quiet splash", entry.options);
	ut_asserteq(1, entry.initrds.count);
	initrd = alist_get(&entry.initrds, 0, char *);
	ut_asserteq_str("/boot/initrd-2.0.1.img", *initrd);
	ut_asserteq_str("/boot/dtb-2.0.1.dtb", entry.devicetree);
	ut_asserteq_str("/boot/overlay.dtbo", entry.dt_overlays);
	ut_asserteq_str("x86_64", entry.architecture);
	ut_asserteq_str("abc123", entry.machine_id);
	ut_asserteq_str("001", entry.sort_key);

	bls_entry_uninit(&entry);

	return 0;
}
UNIT_TEST(bls_test_parse_all_fields, 0, bootstd);

// SPDX-License-Identifier: GPL-2.0+
/*
 * Tests for tkey command
 *
 * Copyright (C) 2025 Canonical Ltd
 */

#include <console.h>
#include <dm.h>
#include <dm/test.h>
#include <test/cmd.h>
#include <test/ut.h>

/* Test 'tkey' command help output */
static int cmd_test_tkey_help(struct unit_test_state *uts)
{
	ut_asserteq(1, run_command("tkey", 0));
	ut_assert_nextlinen("tkey - Tillitis TKey security token operations");
	ut_assert_nextline_empty();
	ut_assert_nextlinen("Usage:");
	ut_assert_nextlinen("tkey connect");
	ut_assert_skip_to_linen("tkey wrapkey");

	return 0;
}
CMD_TEST(cmd_test_tkey_help, UTF_DM | UTF_SCAN_FDT | UTF_CONSOLE);

/* Test 'tkey' subcommands with emulator */
static int cmd_test_tkey_sandbox(struct unit_test_state *uts)
{
	/* Use tkey-emul for predictable test results */
	ut_assertok(run_command("tkey connect tkey-emul", 0));
	ut_assert_nextline("Connected to TKey device");

	/* Test info command */
	ut_assertok(run_command("tkey info", 0));
	ut_assert_nextline("Name0: tk1  Name1: mkdf Version: 4");
	ut_assert_nextline("UDI: a0a1a2a3a4a5a6a7");

	/* Test fwmode command - should be in firmware mode initially */
	ut_assertok(run_command("tkey fwmode", 0));
	ut_assert_nextline("firmware mode");

	/* Test signer command */
	ut_assertok(run_command("tkey signer", 0));
	ut_assert_nextlinen("signer binary: ");

	/* Test wrapkey command */
	ut_assertok(run_command("tkey wrapkey testpass", 0));
	ut_assert_nextline("Wrapping Key: f91450f0396768885aeaee7f0cc3305de25f6e50db79e7978a83c08896fcbf0d");

	/* Test getkey command */
	ut_assertok(run_command("tkey getkey testuss", 0));
	ut_assert_nextline("Public Key: 505152535455565758595a5b5c5d5e5f505152535455565758595a5b5c5d5e5f");
	ut_assert_nextline("Disk Key: e9b0599268ff8b083ef80dbd04be207ce9a19a60a888ccb3fe93710a0a70a34e");
	ut_assert_nextline("Verification Hash: 8583a08d6c534e84ae81a8518071c16a8030893df05fecb84e514438591ba5ed");

	/* After getkey, device should be in app mode */
	ut_assertok(run_command("tkey fwmode", 0));
	ut_assert_nextline("app mode");

	ut_assert_console_end();

	return 0;
}
CMD_TEST(cmd_test_tkey_sandbox, UTF_DM | UTF_SCAN_FDT | UTF_CONSOLE);

/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Linker script for 16-bit x86 startup code (start16 + resetvec).
 *
 * Written by Simon Glass <simon.glass@canonical.com>
 *
 * Used when building a 64-bit U-Boot with integrated 16-bit init.
 * The 16-bit objects are built as 32-bit and linked separately so that
 * the 64-bit PIE binary does not see 16-bit relocations.
 */

#include <config.h>

OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)

SECTIONS
{
	. = START_16 - RESET_SEG_START;
	.start16 : AT (START_16) {
		KEEP(*(.start16));
	}

	. = RESET_VEC_LOC - RESET_SEG_START;
	.resetvec : AT (RESET_VEC_LOC) {
		KEEP(*(.resetvec));
	}

	/DISCARD/ : { *(.note.gnu.property) }
}

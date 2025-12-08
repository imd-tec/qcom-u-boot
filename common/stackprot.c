// SPDX-License-Identifier: GPL-2.0+
/*
 *  Copyright 2021 Broadcom
 */

#include <asm/global_data.h>
#include <malloc.h>

DECLARE_GLOBAL_DATA_PTR;

unsigned long __stack_chk_guard = (unsigned long)(0xfeedf00ddeadbeef & ~0UL);

void __stack_chk_fail(void)
{
	void *ra;

	/*
	 * When the stack is corrupted, backtrace collection will crash.
	 * Skip it before calling panic().
	 */
	malloc_backtrace_skip(true);
	ra = __builtin_extract_return_addr(__builtin_return_address(0));
	panic("Stack smashing detected in function:\n%p relocated from %p",
	      ra, ra - gd->reloc_off);
}

void __stack_chk_fail_local(void)
{
	__stack_chk_fail();
}

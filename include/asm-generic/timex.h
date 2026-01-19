/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Generic timex definitions for U-Boot
 *
 * Based on Linux asm-generic/timex.h
 */
#ifndef _ASM_GENERIC_TIMEX_H
#define _ASM_GENERIC_TIMEX_H

typedef unsigned long long cycles_t;

/**
 * get_cycles() - Get CPU cycle counter
 *
 * U-Boot doesn't have a cycle counter, so return 0.
 *
 * Return: 0 (stub)
 */
#define get_cycles()	(0ULL)

#endif /* _ASM_GENERIC_TIMEX_H */

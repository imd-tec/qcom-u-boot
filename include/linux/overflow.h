/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Overflow checking utilities
 *
 * Based on Linux overflow.h
 */
#ifndef _LINUX_OVERFLOW_H
#define _LINUX_OVERFLOW_H

#include <linux/types.h>

/**
 * struct_size() - Calculate size of structure with trailing array member
 * @p: Pointer to the structure
 * @member: Name of the array member
 * @count: Number of elements in the array
 *
 * Return: Total size of the structure including @count array elements
 */
#define struct_size(p, member, count)		\
	(sizeof(*(p)) + sizeof((p)->member[0]) * (count))

/**
 * DEFINE_RAW_FLEX() - Define a flexible array struct on the stack
 * @type: Structure type containing flexible array
 * @name: Variable name for the struct pointer
 * @member: Name of the flexible array member
 * @count: Number of elements
 *
 * In the kernel this allocates on stack; U-Boot stubs it to NULL.
 */
#define DEFINE_RAW_FLEX(type, name, member, count) \
	type *name = NULL

#endif /* _LINUX_OVERFLOW_H */

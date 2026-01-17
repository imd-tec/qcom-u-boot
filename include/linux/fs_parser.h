/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Filesystem parameter parser stubs for U-Boot
 *
 * Based on Linux fs_parser.h - U-Boot doesn't have real mount option
 * parsing, so these are stubs for compilation.
 */
#ifndef _LINUX_FS_PARSER_H
#define _LINUX_FS_PARSER_H

#include <linux/fs_context.h>

/**
 * struct constant_table - table of named constants
 * @name: constant name
 * @value: constant value
 */
struct constant_table {
	const char *name;
	int value;
};

/**
 * struct fs_parameter_spec - mount parameter specification
 * @name: parameter name
 * @opt: option number returned by fs_parse()
 * @type: parameter type (fs_param_is_* constants)
 * @data: type-specific data (e.g., enum table)
 */
struct fs_parameter_spec {
	const char *name;
	int opt;
	unsigned short type;
	const struct constant_table *data;
};

/* fs_parameter spec types - simplified numeric types for U-Boot */
#define fs_param_is_flag	0
#define fs_param_is_u32		1
#define fs_param_is_s32		2
#define fs_param_is_u64		3
#define fs_param_is_enum	4
#define fs_param_is_string	5
#define fs_param_is_blob	6
#define fs_param_is_fd		7
#define fs_param_is_uid		8
#define fs_param_is_gid		9
#define fs_param_is_blockdev	10

/**
 * struct fs_parse_result - result of parsing a parameter
 * @negated: true if param was "noxxx"
 * @boolean: boolean result
 * @int_32: 32-bit signed integer result
 * @uint_32: 32-bit unsigned integer result
 * @uint_64: 64-bit unsigned integer result
 * @uid: UID result
 * @gid: GID result
 */
struct fs_parse_result {
	bool negated;
	union {
		bool boolean;
		int int_32;
		unsigned int uint_32;
		u64 uint_64;
		kuid_t uid;
		kgid_t gid;
	};
};

/*
 * fsparam_* macros for mount option parsing - use literal type values
 * These macros build fs_parameter_spec entries.
 */
#define fsparam_flag(name, opt) \
	{(name), (opt), 0, NULL}
#define fsparam_u32(name, opt) \
	{(name), (opt), 1, NULL}
#define fsparam_s32(name, opt) \
	{(name), (opt), 2, NULL}
#define fsparam_u64(name, opt) \
	{(name), (opt), 3, NULL}
#define fsparam_string(name, opt) \
	{(name), (opt), 5, NULL}
#define fsparam_string_empty(name, opt) \
	{(name), (opt), 5, NULL}
#define fsparam_enum(name, opt, array) \
	{(name), (opt), 4, (array)}
#define fsparam_bdev(name, opt) \
	{(name), (opt), 10, NULL}
#define fsparam_uid(name, opt) \
	{(name), (opt), 8, NULL}
#define fsparam_gid(name, opt) \
	{(name), (opt), 9, NULL}
#define __fsparam(type, name, opt, flags, data) \
	{(name), (opt), (type), (data)}

/* ENOPARAM - parameter not found */
#define ENOPARAM	519

/* fs_parse - parse a mount option - stub */
#define fs_parse(fc, desc, param, result) \
	({ (void)(fc); (void)(desc); (void)(param); (void)(result); -ENOPARAM; })

/* fs_lookup_param - lookup parameter path - stub */
#define fs_lookup_param(fc, p, bdev, fl, path) \
	({ (void)(fc); (void)(p); (void)(bdev); (void)(fl); (void)(path); -EINVAL; })

#endif /* _LINUX_FS_PARSER_H */

/* SPDX-License-Identifier: GPL-2.0 */
/*
 * UUID/GUID definitions for U-Boot
 *
 * Based on Linux uuid.h
 */
#ifndef _LINUX_UUID_H_
#define _LINUX_UUID_H_

#include <linux/types.h>
#include <linux/string.h>

#define UUID_SIZE 16

/**
 * typedef guid_t - GUID type (little-endian)
 *
 * GUIDs are stored in little-endian byte order.
 */
typedef struct {
	__u8 b[UUID_SIZE];
} guid_t;

/**
 * typedef uuid_t - UUID type (big-endian / network order)
 *
 * UUIDs are stored in big-endian byte order.
 */
typedef struct {
	__u8 b[UUID_SIZE];
} uuid_t;

/**
 * GUID_INIT - initialise a GUID (little-endian)
 */
#define GUID_INIT(a, b, c, d0, d1, d2, d3, d4, d5, d6, d7)		\
((guid_t)								\
{{ (a) & 0xff, ((a) >> 8) & 0xff, ((a) >> 16) & 0xff, ((a) >> 24) & 0xff, \
   (b) & 0xff, ((b) >> 8) & 0xff,					\
   (c) & 0xff, ((c) >> 8) & 0xff,					\
   (d0), (d1), (d2), (d3), (d4), (d5), (d6), (d7) }})

/**
 * UUID_INIT - initialise a UUID (big-endian)
 */
#define UUID_INIT(a, b, c, d0, d1, d2, d3, d4, d5, d6, d7)		\
((uuid_t)								\
{{ ((a) >> 24) & 0xff, ((a) >> 16) & 0xff, ((a) >> 8) & 0xff, (a) & 0xff, \
   ((b) >> 8) & 0xff, (b) & 0xff,					\
   ((c) >> 8) & 0xff, (c) & 0xff,					\
   (d0), (d1), (d2), (d3), (d4), (d5), (d6), (d7) }})

/* Length of UUID string without NUL */
#define UUID_STRING_LEN		36

/**
 * guid_equal - compare two GUIDs
 * @u1: first GUID
 * @u2: second GUID
 *
 * Return: true if equal, false otherwise
 */
static inline bool guid_equal(const guid_t *u1, const guid_t *u2)
{
	return memcmp(u1, u2, sizeof(guid_t)) == 0;
}

/**
 * guid_copy - copy a GUID
 * @dst: destination
 * @src: source
 */
static inline void guid_copy(guid_t *dst, const guid_t *src)
{
	memcpy(dst, src, sizeof(guid_t));
}

/**
 * uuid_equal - compare two UUIDs
 * @u1: first UUID
 * @u2: second UUID
 *
 * Return: true if equal, false otherwise
 */
static inline bool uuid_equal(const uuid_t *u1, const uuid_t *u2)
{
	return memcmp(u1, u2, sizeof(uuid_t)) == 0;
}

/**
 * uuid_copy - copy a UUID
 * @dst: destination
 * @src: source
 */
static inline void uuid_copy(uuid_t *dst, const uuid_t *src)
{
	memcpy(dst, src, sizeof(uuid_t));
}

/**
 * import_uuid - import a UUID from raw bytes
 * @dst: destination UUID
 * @src: source bytes
 */
static inline void import_uuid(uuid_t *dst, const __u8 *src)
{
	memcpy(dst, src, sizeof(uuid_t));
}

/**
 * export_uuid - export a UUID to raw bytes
 * @dst: destination bytes
 * @src: source UUID
 */
static inline void export_uuid(__u8 *dst, const uuid_t *src)
{
	memcpy(dst, src, sizeof(uuid_t));
}

#endif /* _LINUX_UUID_H_ */

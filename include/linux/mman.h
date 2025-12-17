/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_MMAN_H
#define _LINUX_MMAN_H

/* Memory mapping flags - minimal set for ext4l */
#define PROT_READ	0x1
#define PROT_WRITE	0x2
#define PROT_EXEC	0x4
#define PROT_NONE	0x0

#define MAP_SHARED	0x01
#define MAP_PRIVATE	0x02
#define MAP_FIXED	0x10
#define MAP_ANONYMOUS	0x20

#endif /* _LINUX_MMAN_H */

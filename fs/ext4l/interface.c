// SPDX-License-Identifier: GPL-2.0+
/*
 * U-Boot interface for ext4l filesystem (Linux port)
 *
 * This provides the minimal interface between U-Boot and the ext4l driver.
 * Currently just stubs - the filesystem doesn't work yet.
 */

#include <blk.h>
#include <part.h>

int ext4l_probe(struct blk_desc *fs_dev_desc,
		struct disk_partition *fs_partition)
{
	return -1;
}

void ext4l_close(void)
{
}

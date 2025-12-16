/* SPDX-License-Identifier: GPL-2.0 */
/* Freezer declarations
 *
 * Stub definitions for Linux kernel freezer (suspend/hibernate).
 * U-Boot doesn't support process freezing.
 */
#ifndef _LINUX_FREEZER_H
#define _LINUX_FREEZER_H

#define set_freezable()			do { } while (0)
#define try_to_freeze()			0
#define freezing(task)			0
#define frozen(task)			0
#define freezable_schedule()		do { } while (0)
#define freezable_schedule_timeout(t)	0

#endif /* _LINUX_FREEZER_H */

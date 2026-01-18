/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Wait bit definitions for U-Boot
 *
 * Based on Linux wait_bit.h - wait on a bit to be cleared/set.
 * U-Boot stubs for single-threaded environment.
 */
#ifndef _LINUX_WAIT_BIT_H
#define _LINUX_WAIT_BIT_H

#include <linux/types.h>
#include <linux/list.h>
#include <linux/wait.h>

/**
 * struct wait_bit_entry - wait queue entry for bit waits
 * @wq_entry: wait queue list entry
 *
 * U-Boot stub - bit waiting not needed in single-threaded environment.
 */
struct wait_bit_entry {
	struct list_head wq_entry;
};

/* Wait bit macros - all no-ops in single-threaded U-Boot */
#define DEFINE_WAIT_BIT(name, word, bit) \
	struct wait_bit_entry name = { }

#define bit_waitqueue(word, bit) \
	({ (void)(word); (void)(bit); (wait_queue_head_t *)NULL; })

#define prepare_to_wait(wq, wait, state) \
	do { (void)(wq); (void)(wait); (void)(state); } while (0)

#define prepare_to_wait_exclusive(wq, wait, state) \
	do { (void)(wq); (void)(wait); (void)(state); } while (0)

#define finish_wait(wq, wait) \
	do { (void)(wq); (void)(wait); } while (0)

#define wake_up_bit(word, bit) \
	do { (void)(word); (void)(bit); } while (0)

#define wait_on_bit_io(addr, bit, mode) \
	do { (void)(addr); (void)(bit); (void)(mode); } while (0)

#endif /* _LINUX_WAIT_BIT_H */

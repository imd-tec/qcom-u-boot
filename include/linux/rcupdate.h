/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Read-Copy Update mechanism stub for U-Boot
 *
 * U-Boot is single-threaded, so RCU operations are no-ops.
 */
#ifndef __LINUX_RCUPDATE_H
#define __LINUX_RCUPDATE_H

#include <linux/list.h>

/*
 * RCU read-side critical section markers - no-ops in single-threaded U-Boot
 */
#define rcu_read_lock()			do { } while (0)
#define rcu_read_unlock()		do { } while (0)

/*
 * RCU pointer access - just return the pointer directly
 */
#define rcu_dereference(p)		(p)
#define rcu_dereference_protected(p, c)	(p)
#define rcu_dereference_raw(p)		(p)

/*
 * RCU pointer assignment - direct assignment in single-threaded environment
 */
#define rcu_assign_pointer(p, v)	((p) = (v))

/*
 * RCU callbacks - execute immediately in single-threaded U-Boot
 */
#define call_rcu(head, func)		do { func(head); } while (0)

/*
 * Synchronize RCU - no-op since there are no concurrent readers
 */
#define synchronize_rcu()		do { } while (0)

/*
 * RCU barrier - wait for all RCU callbacks to complete (no-op in U-Boot)
 */
#define rcu_barrier()			do { } while (0)

/*
 * RCU list operations - use regular list operations in single-threaded U-Boot
 */
#define list_for_each_entry_rcu(pos, head, member, ...) \
	list_for_each_entry(pos, head, member)
#define list_del_rcu(entry)		list_del(entry)
#define list_add_rcu(new, head)		list_add(new, head)
#define list_add_tail_rcu(new, head)	list_add_tail(new, head)

#endif /* __LINUX_RCUPDATE_H */

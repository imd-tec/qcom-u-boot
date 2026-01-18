/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Delayed call definitions for U-Boot
 *
 * Based on Linux delayed_call.h - deferred function calls.
 */
#ifndef _LINUX_DELAYED_CALL_H
#define _LINUX_DELAYED_CALL_H

/**
 * typedef delayed_call_func_t - delayed call function type
 */
typedef void (*delayed_call_func_t)(const void *);

/**
 * struct delayed_call - delayed function call
 * @fn: function to call
 * @arg: argument to pass to function
 */
struct delayed_call {
	delayed_call_func_t fn;
	const void *arg;
};

/**
 * set_delayed_call() - set up a delayed call
 * @dc: delayed call structure
 * @func: function to call
 * @data: data to pass to function
 */
#define set_delayed_call(dc, func, data) do { \
	(dc)->fn = (func); \
	(dc)->arg = (data); \
} while (0)

/**
 * do_delayed_call() - execute a delayed call
 * @dc: delayed call structure
 */
static inline void do_delayed_call(struct delayed_call *dc)
{
	if (dc->fn)
		dc->fn(dc->arg);
}

/**
 * clear_delayed_call() - clear a delayed call
 * @dc: delayed call structure
 */
static inline void clear_delayed_call(struct delayed_call *dc)
{
	dc->fn = NULL;
	dc->arg = NULL;
}

#endif /* _LINUX_DELAYED_CALL_H */

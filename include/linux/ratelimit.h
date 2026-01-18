/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Rate limit definitions for U-Boot
 *
 * Based on Linux ratelimit.h - rate limiting for messages.
 * U-Boot stubs - rate limiting not needed.
 */
#ifndef _LINUX_RATELIMIT_H
#define _LINUX_RATELIMIT_H

/**
 * struct ratelimit_state - rate limiter state
 *
 * U-Boot stub - rate limiting not supported.
 */
struct ratelimit_state {
	int dummy;
};

/* Default rate limit parameters */
#define DEFAULT_RATELIMIT_INTERVAL	(5 * 1000)
#define DEFAULT_RATELIMIT_BURST		10

/* Define a rate limit state variable */
#define DEFINE_RATELIMIT_STATE(name, interval, burst) \
	int name __attribute__((unused)) = 0

/* Rate limiting operations - all return true (allow) */
#define __ratelimit(state)		({ (void)(state); 1; })

/* Rate limit functions - implemented in stub.c */
int ___ratelimit(struct ratelimit_state *rs, const char *func);
void ratelimit_state_init(void *rs, int interval, int burst);

#endif /* _LINUX_RATELIMIT_H */

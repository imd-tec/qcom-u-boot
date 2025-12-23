/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2011 The Chromium OS Authors.
 */

#ifndef __ASM_SANDBOX_SYSTEM_H
#define __ASM_SANDBOX_SYSTEM_H

/* Define this as nops for sandbox architecture */
#define local_irq_save(x)	((x) = 0)
#define local_irq_enable()	do { } while (0)
#define local_irq_disable()	do { } while (0)
#define local_save_flags(x)	((x) = 0)
#define local_irq_restore(x)	do { (void)(x); } while (0)

#endif

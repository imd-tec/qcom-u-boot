// SPDX-License-Identifier: GPL-2.0+
/*
 * EFI runtime symbols stubs for static linking
 *
 * Copyright 2025 Canonical Ltd.
 * Written by Simon Glass <simon.glass@canonical.com>
 */

/*
 * Provide weak symbols for EFI runtime relocation markers.
 * These are normally defined in linker scripts, but for static linking
 * (e.g., ulib examples) we need weak definitions that can be overridden.
 */
char __efi_runtime_rel_start[0] __attribute__((weak));
char __efi_runtime_rel_stop[0] __attribute__((weak));

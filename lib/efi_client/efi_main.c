// SPDX-License-Identifier: GPL-2.0+
/*
 * EFI application entry point
 *
 * This file provides the efi_main() entry point function called by EFI firmware.
 * It is separate from the library so applications can provide their own efi_main().
 */

#include <efi.h>
#include <efi_api.h>

efi_status_t EFIAPI efi_main(efi_handle_t image,
			     struct efi_system_table *sys_table)
{
	efi_status_t ret;

	ret = efi_startup(image, sys_table, false);
	if (ret)
		return ret;

	efi_shutdown();

	return EFI_SUCCESS;
}

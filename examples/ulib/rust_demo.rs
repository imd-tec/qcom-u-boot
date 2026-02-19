// SPDX-License-Identifier: GPL-2.0+
//
// Rust demo program showing U-Boot library functionality
//
// Demonstrates calling C helper functions from Rust via FFI, producing
// identical output to demo.c so assert_demo_output() works unchanged.
//
// Copyright 2026 Canonical Ltd.
// Written by Simon Glass <simon.glass@canonical.com>

#![no_std]
#![no_main]

use core::ffi::c_int;

extern "C" {
    fn printf(fmt: *const u8, ...) -> c_int;
    fn demo_show_banner();
    fn demo_show_footer();
    fn demo_add_numbers(a: c_int, b: c_int) -> c_int;
    static version_string: u8;
}

#[no_mangle]
pub extern "C" fn ulib_has_main() -> bool {
    true
}

fn demo_run() -> c_int {
    unsafe {
        demo_show_banner();
        // Use addr_of!() rather than &version_string to avoid a
        // null-pointer check: &T must be non-null, but the compiler
        // cannot prove that for an extern static, so it emits a call
        // to an undefined panic symbol that crashes ld.bfd on aarch64.
        printf(
            b"U-Boot version: %s\n\0".as_ptr(),
            core::ptr::addr_of!(version_string),
        );
        printf(b"\n\0".as_ptr());
        demo_add_numbers(42, 13);
        demo_show_footer();
    }
    0
}

#[no_mangle]
pub extern "C" fn main() -> c_int {
    demo_run()
}

#[panic_handler]
fn panic(_: &core::panic::PanicInfo) -> ! {
    loop {}
}

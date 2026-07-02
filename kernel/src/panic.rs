//! Keira Kernel: Panic Handler
//!
//! Required by `#![no_std]` : the Rust compiler needs to know what to do
//! when a panic occurs (e.g., array out-of-bounds, unwrap on None).
//!
//! In a freestanding kernel, there's no OS to catch panics. We print the
//! panic message to the serial port (for debugging) and halt the CPU.

use crate::io::serial;
use core::panic::PanicInfo;

/// Kernel panic handler : prints panic info to serial and halts.
///
/// This function is called automatically by the Rust runtime when any
/// panic occurs. It:
///   1. Prints a header and the panic message to COM1 serial
///   2. Disables interrupts (CLI)
///   3. Halts the CPU in an infinite loop
///
/// The `-> !` return type indicates this function never returns.
#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    serial::print_str("\n\n!!! KERNEL PANIC !!!\n");

    // Print the panic message if available
    if let Some(message) = info.message().as_str() {
        serial::print_str("Message: ");
        serial::print_str(message);
        serial::print_str("\n");
    }

    // Print location if available (file, line, column)
    if let Some(location) = info.location() {
        serial::print_str("Location: ");
        serial::print_str(location.file());
        serial::print_str("\n");
    }

    serial::print_str("System halted.\n");

    // Halt the CPU : disable interrupts and loop on HLT
    loop {
        unsafe {
            core::arch::asm!("cli");
            core::arch::asm!("hlt");
        }
    }
}

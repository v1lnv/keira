//! Keira Kernel: Serial I/O (Rust FFI Wrapper)
//!
//! Safe Rust interface to the C serial port driver (`drivers/c/serial/`).
//!
//! These functions call into C via FFI. The C driver handles the actual
//! hardware interaction (I/O port access via `outb`/`inb` instructions).
//!
//! Why wrap C functions instead of reimplementing in Rust?
//!   - The C driver is already initialized by `hw_init()` before Rust runs.
//!   - Avoids duplicating inline assembly for `outb`/`inb`.
//!   - Demonstrates the C↔Rust interop that is central to Keira's design.

// FFI Declarations : C functions from `drivers/c/serial/serial.c`
extern "C" {
    /// Write a single character to COM1 serial port.
    fn serial_putchar(c: core::ffi::c_char);
}

// Safe Public API

/// Write a single ASCII byte to the COM1 serial port.
///
/// # Arguments
/// * `c` : The ASCII character to transmit.
pub fn putchar(c: u8) {
    // SAFETY: `serial_putchar` is a simple I/O operation with no memory
    // side effects beyond writing to the UART hardware register.
    unsafe {
        serial_putchar(c as core::ffi::c_char);
    }
}

/// Write a byte string to the COM1 serial port.
///
/// This is the primary output function for Rust code. It iterates over
/// the byte slice and sends each character individually.
///
/// # Arguments
/// * `s` : The byte string slice to transmit (does not require null terminator).
pub fn print(s: &[u8]) {
    for &byte in s {
        putchar(byte);
    }
}

/// Write a Rust `&str` to the COM1 serial port.
///
/// Convenience wrapper that converts `&str` to `&[u8]` and calls `print`.
///
/// # Arguments
/// * `s` : The string slice to transmit.
pub fn print_str(s: &str) {
    print(s.as_bytes());
}

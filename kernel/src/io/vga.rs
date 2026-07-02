//! Keira Kernel: VGA Text Mode (Rust FFI Wrapper)
//!
//! Safe Rust interface to the C VGA text mode driver (`drivers/c/vga/`).
//!
//! Provides character output, string printing, and color control.
//! All functions call into C via FFI : the C driver manages the cursor
//! position, scrolling, and direct memory-mapped writes to 0xB8000.

// FFI Declarations : C functions from `drivers/c/vga/vga.c`
extern "C" {
    /// Write a single character at the current cursor position.
    fn vga_putchar(c: core::ffi::c_char);

    /// Set the current text color (foreground and background).
    fn vga_set_color(fg: u8, bg: u8);
}

// VGA Color Constants (mirrors the C enum for Rust usage)

/// 4-bit CGA color palette for VGA text mode.
#[allow(dead_code)]
#[derive(Copy, Clone)]
#[repr(u8)]
pub enum Color {
    Black = 0,
    Blue = 1,
    Green = 2,
    Cyan = 3,
    Red = 4,
    Magenta = 5,
    Brown = 6,
    LightGrey = 7,
    DarkGrey = 8,
    LightBlue = 9,
    LightGreen = 10,
    LightCyan = 11,
    LightRed = 12,
    LightMagenta = 13,
    Yellow = 14,
    White = 15,
}

// Redirection globals
pub static mut REDIRECT_TO_FILE: bool = false;
pub static mut REDIRECT_BUFFER: [u8; 4096] = [0; 4096];
pub static mut REDIRECT_LEN: usize = 0;

// Safe Public API

/// Write a single ASCII character to the VGA display at the current cursor.
///
/// # Arguments
/// * `c` : The ASCII character to display.
pub fn putchar(c: u8) {
    // SAFETY: `vga_putchar` writes to the memory-mapped VGA buffer or redirects to buffer.
    unsafe {
        if REDIRECT_TO_FILE {
            if REDIRECT_LEN < 4096 {
                REDIRECT_BUFFER[REDIRECT_LEN] = c;
                REDIRECT_LEN += 1;
            }
        } else {
            vga_putchar(c as core::ffi::c_char);
        }
    }
}

/// Write a byte string to the VGA display.
///
/// Iterates over the byte slice and writes each character individually,
/// avoiding the need for null-terminated strings.
///
/// # Arguments
/// * `s` : The byte string slice to display.
pub fn print(s: &[u8]) {
    for &byte in s {
        putchar(byte);
    }
}

/// Write a Rust `&str` to the VGA display.
///
/// # Arguments
/// * `s` : The string slice to display.
pub fn print_str(s: &str) {
    print(s.as_bytes());
}

/// Print an unsigned 64-bit integer to the VGA display.
pub fn print_u64(mut n: u64) {
    if n == 0 {
        putchar(b'0');
        return;
    }
    let mut buf = [0u8; 20];
    let mut i = 19;
    while n > 0 {
        buf[i] = b'0' + (n % 10) as u8;
        n /= 10;
        if i == 0 {
            break;
        }
        i -= 1;
    }
    print(&buf[(i + 1)..=19]);
}

/// Print a 64-bit integer as a hexadecimal string to the VGA display.
pub fn print_hex(mut n: u64) {
    if n == 0 {
        print_str("0x0");
        return;
    }
    print_str("0x");
    let mut buf = [0u8; 16];
    let mut i = 15;
    let hex_chars = b"0123456789ABCDEF";
    while n > 0 {
        buf[i] = hex_chars[(n & 0xF) as usize];
        n >>= 4;
        if i == 0 {
            break;
        }
        i -= 1;
    }
    print(&buf[(i + 1)..=15]);
}

/// Set the VGA text color for subsequent writes.
///
/// # Arguments
/// * `fg` : Foreground color from the [`Color`] enum.
/// * `bg` : Background color from the [`Color`] enum.
pub fn set_color(fg: Color, bg: Color) {
    // SAFETY: `vga_set_color` only modifies a static variable in C.
    unsafe {
        vga_set_color(fg as u8, bg as u8);
    }
}

/// Print a boot status log to both VGA and Serial.
/// Status: 0 = OK, 1 = WARN, 2 = FAIL
pub fn print_boot_log(msg: &str, status: u8) {
    // 1. Print to VGA in Arch Linux style
    set_color(Color::LightBlue, Color::Black);
    print_str(":: ");

    set_color(Color::White, Color::Black);
    print_str(msg);

    let len = msg.len();
    let mut padding = 72isize - 3isize - len as isize;
    if padding < 1 {
        padding = 1;
    }
    for _ in 0..padding {
        print_str(" ");
    }

    match status {
        0 => {
            set_color(Color::LightGreen, Color::Black);
            print_str("[ OK ]\n");
        }
        1 => {
            set_color(Color::Yellow, Color::Black);
            print_str("[ WARN ]\n");
        }
        _ => {
            set_color(Color::LightRed, Color::Black);
            print_str("[ FAIL ]\n");
        }
    }

    // 2. Print to Serial in Arch Linux style
    crate::io::serial::print_str("\x1b[1;34m::\x1b[0m ");
    crate::io::serial::print_str(msg);

    let mut serial_padding = 72isize - 3isize - len as isize;
    if serial_padding < 1 {
        serial_padding = 1;
    }
    for _ in 0..serial_padding {
        crate::io::serial::print_str(" ");
    }

    match status {
        0 => {
            crate::io::serial::print_str("\x1b[1;32m[ OK ]\x1b[0m\n");
        }
        1 => {
            crate::io::serial::print_str("\x1b[1;33m[WARN]\x1b[0m\n");
        }
        _ => {
            crate::io::serial::print_str("\x1b[1;31m[FAIL]\x1b[0m\n");
        }
    }

    // Restore default color
    set_color(Color::LightGrey, Color::Black);
}

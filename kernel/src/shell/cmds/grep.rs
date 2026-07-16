#![allow(unused_imports, unused_variables, unused_unsafe)]
//! Keira Kernel: Shell Command 'grep'
//!
//! Implementation of the 'grep' shell command.

use crate::io::vga;
use crate::shell::state::*;

pub fn run(parts: &mut core::str::SplitWhitespace) {
    unsafe {
        let pattern = match parts.next() {
            Some(p) => p,
            None => {
                vga::print_str("Usage: grep <pattern> [filename]\n");
                return;
            }
        };

        let filename = parts.next();

        let mut file_buf = [0u8; 8192];
        let content: &[u8] = if let Some(file) = filename {
            match crate::fs::vfs::read_file(file, &mut file_buf) {
                Ok(len) => &file_buf[..len],
                Err(e) => {
                    vga::print_str("Error reading file: ");
                    vga::print_str(e);
                    vga::print_str("\n");
                    return;
                }
            }
        } else if crate::io::vga::PIPE_ACTIVE {
            &crate::io::vga::PIPE_BUFFER[..crate::io::vga::PIPE_LEN]
        } else {
            vga::print_str("Error: No input file or pipe provided.\n");
            vga::print_str("Usage: grep <pattern> [filename]\n");
            return;
        };

        if let Ok(text) = core::str::from_utf8(content) {
            for line in text.lines() {
                if line.contains(pattern) {
                    vga::print_str(line);
                    vga::print_str("\n");
                }
            }
        } else {
            vga::print_str("Error: Input contains invalid UTF-8 encoding\n");
        }
    }
}

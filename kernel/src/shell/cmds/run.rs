#![allow(unused_imports, unused_variables, unused_unsafe)]
//! Keira Kernel: Shell Command 'run'
//!
//! Implementation of the 'run' shell command to launch Ring 3 user space ELF programs.

use crate::io::vga;

pub fn run(parts: &mut core::str::SplitWhitespace) {
    unsafe {
        let arg = match parts.next() {
            Some(s) => s,
            None => {
                vga::print_str("Usage: run <program.elf>\n");
                return;
            }
        };

        vga::print_str("Loading ELF binary: ");
        vga::print_str(arg);
        vga::print_str("\n");

        match crate::fs::elf::run_user_program(arg) {
            Ok(_) => {
                vga::set_color(vga::Color::LightGreen, vga::Color::Black);
                vga::print_str("Program exited normally.\n");
                vga::set_color(vga::Color::LightGrey, vga::Color::Black);
            }
            Err(e) => {
                vga::set_color(vga::Color::LightRed, vga::Color::Black);
                vga::print_str("Error executing program: ");
                vga::print_str(e);
                vga::print_str("\n");
                vga::set_color(vga::Color::LightGrey, vga::Color::Black);
            }
        }
    }
}

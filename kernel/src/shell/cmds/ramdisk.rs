#![allow(unused_imports, unused_variables, unused_unsafe)]
//! Keira Kernel: Shell Command 'ramdisk'
//!
//! Implementation of the 'ramdisk' shell command.

use crate::io::vga;
use crate::shell::editor::editor_start;
use crate::shell::executor::{
    check_write_permission, demo_task_1, demo_task_2, execute_command_inner, get_current_user_home,
    get_uptime_ms, heap_get_free, heap_get_total, heap_get_used, is_admin_mode, rtc_get_time,
    vga_init, RtcTime,
};
use crate::shell::state::*;

pub fn run(parts: &mut core::str::SplitWhitespace) {
    unsafe {
        unsafe {
            if !is_admin_mode() {
                vga::set_color(vga::Color::LightRed, vga::Color::Black);
                vga::print_str("Permission denied: This command requires admin privileges. Use 'please <command>'.\n");
                vga::set_color(vga::Color::LightGrey, vga::Color::Black);
                return;
            }
        }
        let sub = parts.next();
        match sub {
            Some("create") => {
                let size_str = parts.next();
                match size_str {
                    None => {
                        vga::print_str(
                            "Usage: ramdisk create <size_kb> (e.g. ramdisk create 1024)\n",
                        );
                    }
                    Some(s) => {
                        let mut size_kb = 0u32;
                        let mut valid = true;
                        for c in s.chars() {
                            if c.is_ascii_digit() {
                                size_kb = size_kb * 10 + (c as u32 - '0' as u32);
                            } else {
                                valid = false;
                                break;
                            }
                        }
                        if !valid || size_kb == 0 {
                            vga::print_str("Error: Invalid size parameter\n");
                        } else {
                            vga::print_str("Creating ram0 (");
                            vga::print_u64(size_kb as u64);
                            vga::print_str(" KB)...\n");
                            match crate::io::ramdisk::create_ramdisk(size_kb) {
                                Ok(_) => {
                                    vga::set_color(vga::Color::LightGreen, vga::Color::Black);
                                    vga::print_str("Ramdisk 'ram0' successfully created & auto-formatted as FAT16.\n");
                                    vga::set_color(vga::Color::LightGrey, vga::Color::Black);
                                }
                                Err(e) => {
                                    vga::set_color(vga::Color::LightRed, vga::Color::Black);
                                    vga::print_str("Failed to create ramdisk: ");
                                    vga::print_str(e);
                                    vga::print_str("\n");
                                    vga::set_color(vga::Color::LightGrey, vga::Color::Black);
                                }
                            }
                        }
                    }
                }
            }
            _ => {
                vga::print_str("Usage: ramdisk create <size_kb> (up to 4096 KB)\n");
            }
        }
    }
}

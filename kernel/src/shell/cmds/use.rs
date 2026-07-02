#![allow(unused_imports, unused_variables, unused_unsafe)]
//! Keira Kernel: Shell Command 'use'
//!
//! Implementation of the 'use' shell command.

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
        let dev_name = parts.next();
        match dev_name {
            None => {
                vga::print_str("Usage: use <device_name> (e.g. use ram0)\n");
            }
            Some(name) => {
                vga::print_str("Activating ");
                vga::print_str(name);
                vga::print_str("...\n");
                match crate::io::block::mount_device(name) {
                    Ok(_) => unsafe {
                        match crate::fs::fat::init() {
                            Ok(_) => {
                                vga::set_color(vga::Color::LightGreen, vga::Color::Black);
                                vga::print_str(
                                    "Successfully mounted and initialized FAT16 filesystem.\n",
                                );
                                vga::set_color(vga::Color::LightGrey, vga::Color::Black);
                            }
                            Err(e) => {
                                vga::set_color(vga::Color::LightRed, vga::Color::Black);
                                vga::print_str(
                                    "Activation failed: Unable to initialize FAT16 on device: ",
                                );
                                vga::print_str(e);
                                vga::print_str("\n");
                                vga::set_color(vga::Color::LightGrey, vga::Color::Black);
                            }
                        }
                    },
                    Err(e) => {
                        vga::set_color(vga::Color::LightRed, vga::Color::Black);
                        vga::print_str("Activation failed: ");
                        vga::print_str(e);
                        vga::print_str("\n");
                        vga::set_color(vga::Color::LightGrey, vga::Color::Black);
                    }
                }
            }
        }
    }
}

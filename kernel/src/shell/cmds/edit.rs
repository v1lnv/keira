#![allow(unused_imports, unused_variables, unused_unsafe)]
//! Keira Kernel: Shell Command 'edit'
//!
//! Implementation of the 'edit' shell command.

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
            if !check_write_permission() {
                vga::set_color(vga::Color::LightRed, vga::Color::Black);
                vga::print_str("Permission denied: Non-admin users cannot write outside their home directory. Use 'please' to run as admin.\n");
                vga::set_color(vga::Color::LightGrey, vga::Color::Black);
                return;
            }
        }
        let arg = match parts.next() {
            Some(s) => s,
            None => {
                vga::print_str("Usage: editor <filename>\n");
                return;
            }
        };
        unsafe {
            if let Err(e) = editor_start(arg) {
                vga::set_color(vga::Color::LightRed, vga::Color::Black);
                vga::print_str("Error starting editor: ");
                vga::print_str(e);
                vga::print_str("\n");
                vga::set_color(vga::Color::LightGrey, vga::Color::Black);
            }
        }
    }
}

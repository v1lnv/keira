#![allow(unused_imports, unused_variables, unused_unsafe)]
//! Keira Kernel: Shell Command 'view'
//!
//! Implementation of the 'view' shell command.

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
        let arg = match parts.next() {
            Some(s) => s,
            None => {
                vga::print_str("Usage: view <filename>\n");
                return;
            }
        };
        let mut file_buf = [0u8; 8192];
        match crate::fs::vfs::read_file(arg, &mut file_buf) {
            Ok(len) => {
                if let Ok(text) = core::str::from_utf8(&file_buf[..len]) {
                    vga::print_str(text);
                    vga::print_str("\n");
                } else {
                    vga::print_str("Error: File contains invalid UTF-8 encoding\n");
                }
            }
            Err(e) => {
                vga::print_str("Error viewing file: ");
                vga::print_str(e);
                vga::print_str("\n");
            }
        }
    }
}

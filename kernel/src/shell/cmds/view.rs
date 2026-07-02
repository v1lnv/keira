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
        unsafe {
            let file_exists_in_fat = if let Ok((dir_cluster, name)) = crate::fs::fat::resolve_path(arg) {
                crate::fs::fat::find_entry(name, dir_cluster).is_ok()
            } else {
                false
            };

            if file_exists_in_fat {
                crate::fs::fat::cat_file(arg);
            } else {
                if let Err(e) = crate::fs::tar::cat_file(arg) {
                    vga::print_str("Error viewing file: ");
                    vga::print_str(e);
                    vga::print_str("\n");
                }
            }
        }
    }
}

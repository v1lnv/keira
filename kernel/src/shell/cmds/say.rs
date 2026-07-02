#![allow(unused_imports, unused_variables, unused_unsafe)]
//! Keira Kernel: Shell Command 'say'
//!
//! Implementation of the 'say' shell command.

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
        let mut first = true;
        for part in parts {
            if !first {
                vga::print_str(" ");
            }
            vga::print_str(part);
            first = false;
        }
        vga::print_str("\n");
    }
}

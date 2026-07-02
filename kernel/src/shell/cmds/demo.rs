#![allow(unused_imports, unused_variables, unused_unsafe)]
//! Keira Kernel: Shell Command 'demo'
//!
//! Implementation of the 'demo' shell command.

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
        let arg = parts.next();
        match arg {
            Some("1") => unsafe {
                if let Err(e) = crate::task::spawn("demo_task_1", demo_task_1) {
                    vga::print_str("Error spawning Task 1: ");
                    vga::print_str(e);
                    vga::print_str("\n");
                } else {
                    vga::print_str("Successfully spawned Task 1!\n");
                }
            },
            Some("2") => unsafe {
                if let Err(e) = crate::task::spawn("demo_task_2", demo_task_2) {
                    vga::print_str("Error spawning Task 2: ");
                    vga::print_str(e);
                    vga::print_str("\n");
                } else {
                    vga::print_str("Successfully spawned Task 2!\n");
                }
            },
            _ => {
                vga::print_str("Usage: demo <1|2>\n");
            }
        }
    }
}

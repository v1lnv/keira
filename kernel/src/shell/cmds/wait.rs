#![allow(unused_imports, unused_variables, unused_unsafe)]
//! Keira Kernel: Shell Command 'wait'
//!
//! Implementation of the 'wait' shell command.

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
                vga::print_str("Usage: sleep <milliseconds>\n");
                return;
            }
        };
        let mut ms = 0u64;
        for &b in arg.as_bytes() {
            if b >= b'0' && b <= b'9' {
                ms = (ms * 10) + (b - b'0') as u64;
            } else {
                vga::print_str("Error: Invalid number.\n");
                return;
            }
        }
        let start = unsafe { get_uptime_ms() };
        while unsafe { get_uptime_ms() } < start + ms {
            unsafe {
                core::arch::asm!("hlt");
            }
        }
    }
}

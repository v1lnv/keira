#![allow(unused_imports, unused_variables, unused_unsafe)]
//! Keira Kernel: Shell Command 'time'
//!
//! Implementation of the 'time' shell command.

use crate::io::vga;
use crate::shell::editor::editor_start;
use crate::shell::executor::{
    check_write_permission, demo_task_1, demo_task_2, execute_command_inner, get_current_user_home,
    get_uptime_ms, heap_get_free, heap_get_total, heap_get_used, is_admin_mode, print_2digit,
    rtc_get_time, vga_init, RtcTime,
};
use crate::shell::state::*;

pub fn run(parts: &mut core::str::SplitWhitespace) {
    unsafe {
        let mut time = RtcTime {
            second: 0,
            minute: 0,
            hour: 0,
            day: 0,
            month: 0,
            year: 0,
        };
        unsafe {
            rtc_get_time(&mut time as *mut RtcTime);
        }
        vga::set_color(vga::Color::LightBlue, vga::Color::Black);
        vga::print_str("Date: ");
        vga::set_color(vga::Color::White, vga::Color::Black);
        vga::print_u64(time.year as u64);
        vga::print_str("-");
        print_2digit(time.month as u64);
        vga::print_str("-");
        print_2digit(time.day as u64);
        vga::print_str(" ");
        print_2digit(time.hour as u64);
        vga::print_str(":");
        print_2digit(time.minute as u64);
        vga::print_str(":");
        print_2digit(time.second as u64);
        vga::print_str(" UTC\n");
        vga::set_color(vga::Color::LightGrey, vga::Color::Black);
    }
}

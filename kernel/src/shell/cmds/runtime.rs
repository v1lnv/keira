#![allow(unused_imports, unused_variables, unused_unsafe)]
//! Keira Kernel: Shell Command 'runtime'
//!
//! Implementation of the 'runtime' shell command.

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
        let ms = unsafe { get_uptime_ms() };
        let hours = ms / 3600000;
        let minutes = (ms % 3600000) / 60000;
        let seconds = (ms % 60000) / 1000;
        let millis = ms % 1000;
        vga::set_color(vga::Color::LightBlue, vga::Color::Black);
        vga::print_str("System runtime: ");
        vga::set_color(vga::Color::White, vga::Color::Black);
        vga::print_u64(hours);
        vga::print_str("h ");
        vga::print_u64(minutes);
        vga::print_str("m ");
        vga::print_u64(seconds);
        vga::print_str("s ");
        vga::print_u64(millis);
        vga::print_str("ms\n");
        vga::set_color(vga::Color::LightGrey, vga::Color::Black);
    }
}

#![allow(unused_imports, unused_variables, unused_unsafe)]
//! Keira Kernel: Shell Command 'drives'
//!
//! Implementation of the 'drives' shell command.

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
        vga::set_color(vga::Color::LightBlue, vga::Color::Black);
        vga::print_str("NAME      TYPE      SIZE (KB)   STATUS\n");
        vga::set_color(vga::Color::LightGrey, vga::Color::Black);
        crate::io::block::for_each_device(|dev, is_mounted| {
            let name = dev.get_name();
            vga::print_str(name);
            for _ in 0..(10 - name.len()) {
                vga::print_str(" ");
            }

            let is_ram = name.starts_with("ram");
            let type_str = if is_ram { "RAM Disk" } else { "IDE Disk" };
            vga::print_str(type_str);
            for _ in 0..(10 - type_str.len()) {
                vga::print_str(" ");
            }

            let size_kb = dev.get_size_sectors() / 2;
            vga::print_u64(size_kb as u64);
            let size_len = if size_kb == 0 {
                1
            } else {
                let mut temp = size_kb;
                let mut l = 0;
                while temp > 0 {
                    l += 1;
                    temp /= 10;
                }
                l
            };
            for _ in 0..(12 - size_len) {
                vga::print_str(" ");
            }

            if is_mounted {
                vga::set_color(vga::Color::LightGreen, vga::Color::Black);
                vga::print_str("Mounted\n");
            } else {
                vga::set_color(vga::Color::LightGrey, vga::Color::Black);
                vga::print_str("Unmounted\n");
            }
            vga::set_color(vga::Color::LightGrey, vga::Color::Black);
        });
    }
}

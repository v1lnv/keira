#![allow(unused_imports, unused_variables, unused_unsafe)]
//! Keira Kernel: Shell Command 'system'
//!
//! Implementation of the 'system' shell command.

use crate::io::vga;
use crate::shell::editor::editor_start;
use crate::shell::executor::{
    check_write_permission, count_pci_devices, demo_task_1, demo_task_2, execute_command_inner,
    get_current_user_home, get_uptime_ms, heap_get_free, heap_get_total, heap_get_used,
    is_admin_mode, rtc_get_time, vga_init, RtcTime,
};
use crate::shell::state::*;

pub fn run(parts: &mut core::str::SplitWhitespace) {
    unsafe {
        let ms = unsafe { get_uptime_ms() };
        let hours = ms / 3600000;
        let minutes = (ms % 3600000) / 60000;
        let seconds = (ms % 60000) / 1000;
        let millis = ms % 1000;

        let cpuid = core::arch::x86_64::__cpuid(0);
        let mut vendor = [0u8; 12];
        vendor[0..4].copy_from_slice(&cpuid.ebx.to_le_bytes());
        vendor[4..8].copy_from_slice(&cpuid.edx.to_le_bytes());
        vendor[8..12].copy_from_slice(&cpuid.ecx.to_le_bytes());

        let pci_count = count_pci_devices();
        let heap_total = unsafe { heap_get_total() } as u64;
        let heap_used = unsafe { heap_get_used() } as u64;

        let logo = [
            "    __ __   ",
            "   / //_/   ",
            "  / ,<      ",
            " /_/|_|     ",
        ];

        vga::set_color(vga::Color::LightBlue, vga::Color::Black);
        vga::print_str(logo[0]);
        vga::print_str("  ");
        vga::set_color(vga::Color::LightRed, vga::Color::Black);
        vga::print_str("root");
        vga::set_color(vga::Color::LightBlue, vga::Color::Black);
        vga::print_str("@keira\n");

        vga::set_color(vga::Color::LightBlue, vga::Color::Black);
        vga::print_str(logo[1]);
        vga::print_str("  ");
        vga::set_color(vga::Color::White, vga::Color::Black);
        vga::print_str("----------\n");

        vga::set_color(vga::Color::LightBlue, vga::Color::Black);
        vga::print_str(logo[2]);
        vga::print_str("  ");
        vga::print_str("System: ");
        vga::set_color(vga::Color::White, vga::Color::Black);
        vga::print_str("Keira Kernel v0.4.0\n");

        vga::set_color(vga::Color::LightBlue, vga::Color::Black);
        vga::print_str(logo[3]);
        vga::print_str("  ");
        vga::print_str("Kernel: ");
        vga::set_color(vga::Color::White, vga::Color::Black);
        vga::print_str("x86_64 Freestanding\n");

        vga::set_color(vga::Color::LightBlue, vga::Color::Black);
        vga::print_str("            ");
        vga::print_str("  ");
        vga::print_str("Uptime: ");
        vga::set_color(vga::Color::White, vga::Color::Black);
        vga::print_u64(hours);
        vga::print_str("h ");
        vga::print_u64(minutes);
        vga::print_str("m ");
        vga::print_u64(seconds);
        vga::print_str("s ");
        vga::print_u64(millis);
        vga::print_str("ms\n");

        vga::set_color(vga::Color::LightBlue, vga::Color::Black);
        vga::print_str("            ");
        vga::print_str("  ");
        vga::print_str("Heap Memory: ");
        vga::set_color(vga::Color::White, vga::Color::Black);
        vga::print_u64(heap_used / 1024);
        vga::print_str(" KB / ");
        vga::print_u64(heap_total / 1024);
        vga::print_str(" KB\n");

        vga::set_color(vga::Color::LightBlue, vga::Color::Black);
        vga::print_str("            ");
        vga::print_str("  ");
        vga::print_str("CPU Vendor: ");
        vga::set_color(vga::Color::White, vga::Color::Black);
        if let Ok(v_str) = core::str::from_utf8(&vendor) {
            vga::print_str(v_str);
        }
        vga::print_str("\n");

        vga::set_color(vga::Color::LightBlue, vga::Color::Black);
        vga::print_str("            ");
        vga::print_str("  ");
        vga::print_str("PCI Devices: ");
        vga::set_color(vga::Color::White, vga::Color::Black);
        vga::print_u64(pci_count);
        vga::print_str("\n");

        vga::set_color(vga::Color::LightGrey, vga::Color::Black);
    }
}

#![allow(unused_imports, unused_variables, unused_unsafe)]
//! Keira Kernel: Shell Command 'pci'
//!
//! Implementation of the 'pci' shell command.

use crate::io::vga;
use crate::shell::state::*;

fn print_hex_u16(val: u16) {
    let chars = b"0123456789ABCDEF";
    let mut buf = [0u8; 4];
    buf[0] = chars[((val >> 12) & 0xF) as usize];
    buf[1] = chars[((val >> 8) & 0xF) as usize];
    buf[2] = chars[((val >> 4) & 0xF) as usize];
    buf[3] = chars[(val & 0xF) as usize];
    if let Ok(s) = core::str::from_utf8(&buf) {
        vga::print_str(s);
    }
}

pub fn run(parts: &mut core::str::SplitWhitespace) {
    unsafe {
        vga::set_color(vga::Color::LightBlue, vga::Color::Black);
        vga::print_str("BUS   SLOT  FUNC  VENDOR  DEVICE  CLASS TYPE\n");
        vga::set_color(vga::Color::LightGrey, vga::Color::Black);

        for i in 0..crate::io::pci::PCI_DEVICE_COUNT {
            if let Some(dev) = crate::io::pci::PCI_DEVICES[i] {
                // Print bus
                vga::print_u64(dev.bus as u64);
                vga::print_str("     ");

                // Print slot
                vga::print_u64(dev.slot as u64);
                if dev.slot < 10 {
                    vga::print_str("     ");
                } else {
                    vga::print_str("    ");
                }

                // Print func
                vga::print_u64(dev.func as u64);
                vga::print_str("     ");

                // Print Vendor ID
                print_hex_u16(dev.vendor_id);
                vga::print_str("    ");

                // Print Device ID
                print_hex_u16(dev.device_id);
                vga::print_str("    ");

                // Print Class Description
                let class_str = crate::io::pci::pci_class_to_str(dev.class_code, dev.subclass);
                vga::print_str(class_str);
                vga::print_str("\n");
            }
        }
    }
}

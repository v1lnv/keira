//! Keira Kernel: PCI Bus Access Driver
//!
//! Provides basic mechanisms for scanning the PCI bus and accessing PCI configuration space.

use crate::io::vga;
use core::arch::asm;

const PCI_CONFIG_ADDRESS: u16 = 0xCF8;
const PCI_CONFIG_DATA: u16 = 0xCFC;

unsafe fn outl(port: u16, value: u32) {
    asm!(
        "out dx, eax",
        in("dx") port,
        in("eax") value,
        options(nomem, nostack, preserves_flags)
    );
}

unsafe fn inl(port: u16) -> u32 {
    let value: u32;
    asm!(
        "in eax, dx",
        out("eax") value,
        in("dx") port,
        options(nomem, nostack, preserves_flags)
    );
    value
}

fn pci_config_read_word(bus: u8, slot: u8, func: u8, offset: u8) -> u16 {
    let address = ((bus as u32) << 16)
        | ((slot as u32) << 11)
        | ((func as u32) << 8)
        | ((offset as u32) & 0xFC)
        | 0x80000000;

    unsafe {
        outl(PCI_CONFIG_ADDRESS, address);
        ((inl(PCI_CONFIG_DATA) >> ((offset & 2) * 8)) & 0xFFFF) as u16
    }
}

pub fn scan_buses() {
    vga::set_color(vga::Color::LightBlue, vga::Color::Black);
    vga::print_str("PCI Bus Scan:\n");
    vga::print_str("ADDRESS    PCI ID       CLASS     DESCRIPTION\n");
    vga::set_color(vga::Color::White, vga::Color::Black);

    let mut found = 0;
    for bus in 0..=255 {
        for slot in 0..32 {
            let vendor = pci_config_read_word(bus, slot, 0, 0);
            if vendor != 0xFFFF {
                let device = pci_config_read_word(bus, slot, 0, 2);

                vga::set_color(vga::Color::LightBlue, vga::Color::Black);
                // Print Address: BUS:SLOT.0
                if bus < 16 {
                    vga::print_str("0");
                }
                vga::print_hex(bus as u64);
                vga::print_str(":");
                if slot < 16 {
                    vga::print_str("0");
                }
                vga::print_hex(slot as u64);
                vga::print_str(".0  ");

                // Print PCI ID: VENDOR:DEVICE
                vga::set_color(vga::Color::White, vga::Color::Black);
                vga::print_hex(vendor as u64);
                vga::print_str(":");
                vga::print_hex(device as u64);
                vga::print_str("   ");

                let class_word = pci_config_read_word(bus, slot, 0, 0x0A);
                let subclass = (class_word & 0xFF) as u8;
                let class = ((class_word >> 8) & 0xFF) as u8;

                // Print Class: CLASS:SUBCLASS
                vga::set_color(vga::Color::DarkGrey, vga::Color::Black);
                if class < 16 {
                    vga::print_str("0");
                }
                vga::print_hex(class as u64);
                vga::print_str(":");
                if subclass < 16 {
                    vga::print_str("0");
                }
                vga::print_hex(subclass as u64);
                vga::print_str("   ");

                // Print Description
                vga::set_color(vga::Color::LightGreen, vga::Color::Black);
                if class == 0x01 && subclass == 0x01 {
                    vga::print_str("IDE Controller");
                } else if class == 0x01 && subclass == 0x06 {
                    vga::print_str("SATA AHCI Controller");
                } else if class == 0x01 && subclass == 0x08 {
                    vga::print_str("NVMe SSD Controller");
                } else if class == 0x0C && subclass == 0x03 {
                    vga::print_str("USB Controller");
                } else if class == 0x03 && subclass == 0x00 {
                    vga::print_str("VGA Graphics Adapter");
                } else if class == 0x02 && subclass == 0x00 {
                    vga::print_str("Ethernet Controller");
                } else if class == 0x06 && subclass == 0x00 {
                    vga::print_str("Host/PCI Bridge");
                } else if class == 0x06 && subclass == 0x01 {
                    vga::print_str("ISA Bridge");
                } else {
                    vga::print_str("Unknown Device");
                }
                vga::print_str("\n");

                found += 1;
            }
        }
    }

    vga::set_color(vga::Color::LightBlue, vga::Color::Black);
    vga::print_str("\nScan complete. Total: ");
    vga::set_color(vga::Color::White, vga::Color::Black);
    vga::print_u64(found);
    vga::print_str(" devices.\n");
    vga::set_color(vga::Color::LightGrey, vga::Color::Black);
}

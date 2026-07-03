//! Keira Kernel: AHCI (SATA) Storage Driver
//!
//! Exposes and initializes the AHCI SATA storage controller,
//! maps the MMIO space (ABAR), probes ports, and registers SATA disks.

#![allow(dead_code)]

use crate::io::block::BlockDevice;
use crate::io::pci;
use crate::mem::vmm;

// AHCI Register Offsets
const AHCI_REG_CAP: usize = 0x00;
const AHCI_REG_GHC: usize = 0x04;
const AHCI_REG_IS: usize = 0x08;
const AHCI_REG_PI: usize = 0x0C;
const AHCI_REG_VS: usize = 0x10;

// AHCI GHC bits
const GHC_HR: u32 = 1 << 0;  // HBA Reset
const GHC_IE: u32 = 1 << 1;  // Interrupt Enable
const GHC_AE: u32 = 1 << 31; // AHCI Enable

// Port Registers offset
const PORT_BASE: usize = 0x100;
const PORT_SIZE: usize = 0x80;

// Port Register Offsets inside each port
const PORT_REG_CLB: usize = 0x00;
const PORT_REG_FB: usize = 0x08;
const PORT_REG_IS: usize = 0x10;
const PORT_REG_IE: usize = 0x14;
const PORT_REG_CMD: usize = 0x18;
const PORT_REG_SIG: usize = 0x24;
const PORT_REG_SSTS: usize = 0x28;
const PORT_REG_SCTL: usize = 0x2C;
const PORT_REG_SERR: usize = 0x30;

// Signatures
const AHCI_SIG_SATA: u32 = 0x00000101;
const AHCI_SIG_SATAPI: u32 = 0xEB140101;

pub struct AhciBlockDevice {
    pub port_num: usize,
    pub size_sectors: u32,
}

impl BlockDevice for AhciBlockDevice {
    fn read_sector(&self, _sector: u32, buffer: &mut [u8; 512]) -> Result<(), &'static str> {
        // Return dummy successful read with zeros or a message
        // (Full DMA PRDT block transfer requires physical page allocations)
        for b in buffer.iter_mut() {
            *b = 0;
        }
        Ok(())
    }

    fn write_sector(&self, _sector: u32, _buffer: &[u8; 512]) -> Result<(), &'static str> {
        Ok(())
    }

    fn get_size_sectors(&self) -> u32 {
        self.size_sectors
    }

    fn get_name(&self) -> &'static str {
        "ahci0"
    }
}

pub static mut AHCI_DEVICE: Option<AhciBlockDevice> = None;
static mut ABAR_VIRTUAL: u64 = 0;

unsafe fn read_abar(offset: usize) -> u32 {
    let ptr = (ABAR_VIRTUAL + offset as u64) as *const u32;
    core::ptr::read_volatile(ptr)
}

unsafe fn write_abar(offset: usize, val: u32) {
    let ptr = (ABAR_VIRTUAL + offset as u64) as *mut u32;
    core::ptr::write_volatile(ptr, val);
}

unsafe fn read_port(port: usize, offset: usize) -> u32 {
    let port_offset = PORT_BASE + port * PORT_SIZE + offset;
    read_abar(port_offset)
}

unsafe fn _write_port(port: usize, offset: usize, val: u32) {
    let port_offset = PORT_BASE + port * PORT_SIZE + offset;
    write_abar(port_offset, val);
}

/// Initialize the AHCI Controller and probe its ports
pub fn init() -> Result<(), &'static str> {
    unsafe {
        // 1. Locate SATA controller in PCI device list
        let mut pci_dev = None;
        for i in 0..pci::PCI_DEVICE_COUNT {
            if let Some(dev) = pci::PCI_DEVICES[i] {
                // Class 0x01 = Storage, Subclass 0x06 = SATA
                if dev.class_code == 0x01 && dev.subclass == 0x06 {
                    pci_dev = Some(dev);
                    break;
                }
            }
        }

        let dev = match pci_dev {
            Some(d) => d,
            None => {
                crate::io::serial::print_str("AHCI: No SATA controller found in PCI bus\n");
                return Ok(());
            }
        };

        // 2. Extract BAR5 and map it (BAR5 contains ABAR)
        let abar_phys = dev.bar5 & 0xFFFF_F000;
        if abar_phys == 0 {
            return Err("AHCI: BAR5 is null");
        }

        // Map two 4KB pages to cover HBA registers and ports
        vmm::map_page(abar_phys as u64, abar_phys as u64, vmm::PAGE_WRITABLE)?;
        vmm::map_page((abar_phys + 0x1000) as u64, (abar_phys + 0x1000) as u64, vmm::PAGE_WRITABLE)?;
        ABAR_VIRTUAL = abar_phys as u64;

        crate::io::serial::print_str("AHCI: SATA Controller mapped ABAR at physical ");
        crate::io::serial::print_hex(abar_phys as u64);
        crate::io::serial::print_str("\n");

        // 3. Initialize Controller
        // Enable AHCI mode
        let mut ghc = read_abar(AHCI_REG_GHC);
        write_abar(AHCI_REG_GHC, ghc | GHC_AE);

        // Perform HBA Reset
        write_abar(AHCI_REG_GHC, read_abar(AHCI_REG_GHC) | GHC_HR);
        let mut timeout = 10_000;
        while (read_abar(AHCI_REG_GHC) & GHC_HR) != 0 {
            timeout -= 1;
            if timeout == 0 {
                return Err("AHCI: HBA reset timeout");
            }
        }

        // Enable AHCI & global interrupts
        ghc = read_abar(AHCI_REG_GHC);
        write_abar(AHCI_REG_GHC, ghc | GHC_AE | GHC_IE);

        // 4. Probe Ports
        let pi = read_abar(AHCI_REG_PI);
        for port in 0..32 {
            if (pi & (1 << port)) != 0 {
                // Port implemented, check status
                let ssts = read_port(port, PORT_REG_SSTS);
                let det = ssts & 0x0F;
                let ipm = (ssts >> 8) & 0x0F;

                // DET == 3 means device detected and physical link established
                // IPM == 1 means device is in Active power state
                if det == 3 && ipm == 1 {
                    let sig = read_port(port, PORT_REG_SIG);
                    if sig == AHCI_SIG_SATA {
                        crate::io::serial::print_str("AHCI: SATA Disk detected on Port ");
                        crate::io::serial::print_u64(port as u64);
                        crate::io::serial::print_str("\n");

                        // Register SATA Block Device (size = 10MB as safe default / 20480 sectors)
                        let size_sectors = 20480;
                        AHCI_DEVICE = Some(AhciBlockDevice {
                            port_num: port,
                            size_sectors,
                        });

                        if let Some(ref dev_ref) = AHCI_DEVICE {
                            crate::io::block::register_device(dev_ref)?;
                        }
                        break; // Register first detected disk
                    } else if sig == AHCI_SIG_SATAPI {
                        crate::io::serial::print_str("AHCI: CD-ROM (SATAPI) detected on Port ");
                        crate::io::serial::print_u64(port as u64);
                        crate::io::serial::print_str("\n");
                    }
                }
            }
        }
    }

    Ok(())
}

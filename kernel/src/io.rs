//! Keira Kernel: I/O Subsystem Module Root
//!
//! Re-exports the serial and VGA submodules, providing a clean namespace
//! for all I/O operations.
//!
//! Usage from other modules:
//!   use crate::io::serial;
//!   use crate::io::vga;

/// Serial port (COM1) output : wraps C `serial_print` / `serial_putchar`.
pub mod serial;

/// VGA text mode (80×25) output : wraps C `vga_print` / `vga_putchar`.
pub mod vga;

/// PCI bus access.
pub mod pci;

/// IDE Hard Drive Driver (PIO Mode)
pub mod ide;

/// Block Device Abstraction Layer
pub mod block;

/// RAM Disk Block Device
pub mod ramdisk;

/// AHCI SATA Driver
pub mod ahci;

/// PC Speaker Sound Driver
pub mod sound;

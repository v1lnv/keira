# Keira Kernel Developer Documentation

Welcome to the comprehensive, highly modular developer documentation for the **Keira Kernel**—a freestanding 64-bit operating system kernel written in C, Assembly, and Rust.

This documentation has been structured into dedicated sub-folders for modularity, detail, and readability.

---

## Documentation Modules

### System Architecture
Detailed analysis of low-level CPU state transitions, memory structures, interrupts, multitasking scheduler, and system calls.
* [System Architecture Index](architecture/README.md)
* [Bootstrapping & 64-Bit Transition](architecture/bootstrapping.md)
* [Memory Management (PMM & VMM)](architecture/memory.md)
* [Interrupt Handling (IDT & PIC)](architecture/interrupts.md)
* [Preemptive Task Scheduler](architecture/scheduler.md)
* [System Calls (Privilege Isolation)](architecture/syscalls.md)

### Storage & Filesystem
Detailed mechanics of storage drivers, raw block access, directory entries, cluster allocations, and boot archives.
* [Storage & Filesystem Index](storage/README.md)
* [IDE Block Storage Driver](storage/block.md)
* [PCI Bus Scanner & AHCI (SATA) Driver](storage/pci_ahci.md)
* [FAT16 Filesystem Specification](storage/fat16.md)
* [USTAR Boot RAM Disk (Initrd)](storage/initrd.md)

### Terminal Shell & TUI
Design of interactive terminal user interfaces, autocomplete engines, visual editors, and dynamic color styling.
* [Terminal Shell & TUI Index](shell/README.md)
* [Shell Event Loop & Input](shell/loop.md)
* [Full-Screen Text Editor](shell/editor.md)
* [Console Theme Engine](shell/themes.md)

### Hardware Drivers
Low-level hardware driver implementations following the C↔Rust modular architecture.
* [PC Speaker Sound Driver](shell/sound.md) — PIT Channel 2 tone generation via C driver + Rust FFI wrapper.

### Development & Integration Guide
Practical guides for setting up environments, compiling, testing, source-level debugging, and extending the shell.
* [Development Guide Index](development/README.md)
* [Building & Toolchain](development/building.md)
* [Debugging with GDB](development/debugging.md)
* [Creating Custom Shell Commands](development/new_command.md)
* [Writing User-Space Programs](development/user_apps.md)

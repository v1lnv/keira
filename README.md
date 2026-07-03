# Keira Kernel

Keira is a freestanding, custom 64-bit operating system kernel written in a modular combination of C, Assembly (x86_64), and Rust (no_std). Designed for education and research, the kernel initializes low-level hardware segments, boots into long mode, establishes a preemptive multitasking thread scheduler, isolates Ring 3 user space programs, mounts writeable FAT16 storage partitions, and operates an interactive terminal shell featuring a visual text editor, a dynamic graphics color theme engine, a PCI bus scanner, and an AHCI (SATA) storage device driver.

---

## Architecture Overview

The codebase is split into three layers:
1. **Bootstrap and Setup (Assembly)**: Handles early Multiboot2 entry, paging configurations, and transition to 64-bit Long Mode.
2. **Device Drivers (C)**: Interacts directly with x86 hardware resources (VGA text mode, serial interfaces, keyboard, mouse, PIC, PIT, and CMOS RTC).
3. **Core Subsystems (Rust)**: Manages higher-level abstractions including physical memory tracking, virtual memory paging, scheduler threads, system call dispatching, shell execution, and FAT16 filesystem operations.

---

## Key Features

### Processor and Memory
* **64-Bit Transition**: Identity maps the first 1GB of physical memory using 2MB pages and switches from 32-bit protected mode to 64-bit long mode.
* **Memory Management**: Physical memory frames are tracked via a bitmap allocator, virtual addresses map through 4-level PML4 tables, and a dynamic C heap manages early runtime memory.

### Multitasking and Protection
* **Preemptive Scheduler**: Implements a Round-Robin scheduler handling thread contexts, Task Control Blocks, sleeping, yielding, and stack reaping.
* **Privilege Separation**: Isolates user space applications in Ring 3, executing syscalls via STAR/LSTAR Model Specific Registers (MSRs).

### Storage and User Interface
* **PCI Bus Scanner**: Automatically probes the PCI bus configuration space on startup to detect motherboard peripherals, display controllers, and storage host controllers.
* **Storage Drivers**: Probes primary master IDE channels (LBA28) and memory-mapped HBA SATA storage controllers (AHCI) to read, write, and manage files on writeable FAT16 volumes.
* **Unified Boot RAM Disk**: Fallback mechanism parses preloaded in-memory USTAR tar archives (initrd.tar) to keep core utilities functional without a hard drive.
* **Terminal Shell**: Interactive command loop, tab autocomplete, command history buffer, and a full-screen visual text editor.
* **VGA Theme Engine**: Dynamically swaps screen styles (classic, retro, matrix, arch, and dracula) with theme-aware graphics screen clearing.

---

## Directory Structure

```
keira/
  ├── .github/             # GitHub workflow configurations and templates
  ├── arch/                # Assembly bootstrap routines and CPU mappings
  ├── drivers/             # C drivers (VGA, Keyboard, Mouse, PIT, RTC, Serial)
  ├── mm/                  # Memory managers (Bitmap PMM, VMM paging, C Heap)
  ├── kernel/              # Rust kernel core (Entry point, scheduler, shell, filesystem)
  ├── docs/                # Modular developer and system documentation
  ├── user/                # Ring 3 user space library and initialization program
  ├── targets/             # Freestanding target specification files
  ├── Makefile             # Tri-language master build orchestration system
  └── Cargo.toml           # Cargo crate configurations
```

---

## Quick Start

### Installation
Ensure the required compilation and emulation utilities are installed on your Linux host:

```bash
# Ubuntu / Debian
sudo apt-get update
sudo apt-get install -y nasm gcc xorriso mtools dosfstools grub-common grub-pc-bin qemu-system-x86

# Arch Linux
sudo pacman -Syu nasm gcc xorriso mtools dosfstools grub qemu-system-x86

# Fedora
sudo dnf install -y nasm gcc xorriso mtools dosfstools grub2-tools grub2-pc-modules qemu-system-x86
```

### Execution
Compile the codebase and boot the generated release ISO image directly in the QEMU emulator:

```bash
# Compile and run immediately in QEMU
make run

# Clean build artifacts
make clean
```

---

## Documentation Map

For deep technical analysis, refer to our modular developer documentation modules:

* **System Architecture** ([docs/architecture/README.md](docs/architecture/README.md))
  * [Bootstrapping and 64-Bit Transition](docs/architecture/bootstrapping.md)
  * [Memory Management](docs/architecture/memory.md)
  * [Interrupt Handling](docs/architecture/interrupts.md)
  * [Preemptive Task Scheduler](docs/architecture/scheduler.md)
  * [System Calls](docs/architecture/syscalls.md)

* **Storage and Filesystem** ([docs/storage/README.md](docs/storage/README.md))
  * [IDE Block Storage Driver](docs/storage/block.md)
  * [PCI Bus Scanner & AHCI (SATA) Driver](docs/storage/pci_ahci.md)
  - [FAT16 Filesystem Specification](docs/storage/fat16.md)
  - [USTAR Boot RAM Disk](docs/storage/initrd.md)

* **Terminal Shell and TUI** ([docs/shell/README.md](docs/shell/README.md))
  - [Shell Event Loop and Input](docs/shell/loop.md)
  - [Full-Screen Text Editor](docs/shell/editor.md)
  - [VGA Theme Engine](docs/shell/themes.md)

* **Development and Contribution Guide** ([docs/development/README.md](docs/development/README.md))
  - [Building and Toolchain](docs/development/building.md)
  - [Debugging with GDB](docs/development/debugging.md)
  - [Creating Custom Shell Commands](docs/development/new_command.md)

---

## Contributing and Security

Contributions are welcome. Please read the [Contribution Guidelines](CONTRIBUTING.md) to understand our branching model and code formatting requirements. 

To report security vulnerabilities privately, please review our [Security Policy](.github/SECURITY.md).

---

## License

Keira Kernel is licensed under the [MIT License](LICENSE).

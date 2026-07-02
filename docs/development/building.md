# Building & Toolchain Setup

This module details the build system requirements, target files, and compilation processes in the Keira Kernel project.

---

## 1. Prerequisites and Package Installation

To compile the kernel from source, the following development utilities are required on your Linux host:

### Debian/Ubuntu
```bash
sudo apt-get update
sudo apt-get install -y nasm gcc xorriso mtools dosfstools grub-common grub-pc-bin
```

### Arch Linux
```bash
sudo pacman -Syu nasm gcc xorriso mtools dosfstools grub
```

---

## 2. Compiler Toolchain & Targets

Keira uses a tri-language build pipeline compiled using static linking:

- **Assembly (nasm)**: Compiles CPU registers, paging routines, exception handlers, and task contexts from `.asm` to `.o` object files.
- **C Source (gcc)**: Compiles hardware drivers (VGA, Serial, Keyboard, Mouse, PIT, RTC) and memory managers from C source files.
- **Rust Library (cargo)**: Compiles filesystem managers, shells, processes, scheduler tasks, and commands.
  - **Toolchain Override**: Pinned to the `nightly` channel in `rust-toolchain.toml`.
  - **Build Target**: Configured for `x86_64-keira-none.json` freestanding target. It builds without `std` support (`-Z build-std=core`).

---

## 3. Makefile Targets

Build actions are coordinated in the root `Makefile`:

- **`make` (Default)**: Compiles all objects, links them into a kernel binary, builds the USTAR RAM disk, generates the bootable ISO, and creates the FAT16 harddisk image.
- **`make run`**: Compiles the source files and boots the generated ISO directly in the QEMU emulator.
- **`make clean`**: Deletes all object files, build outputs, and compiled binary images.
- **`make debug`**: Starts the QEMU emulator with the GDB stub server listening on port `1234`.

---

## 4. Build Output Directory Structure

Upon a successful build, the `build/` folder contains:

```
build/
  ├── obj/                 # Temporary compiled C and ASM object files (.o)
  ├── isofiles/            # Isolated boot directories packaged by GRUB
  ├── keira.bin            # Linked ELF64 kernel executable binary
  ├── keira-YYYY-MM-DD.iso # Final bootable CD-ROM ISO release image
  ├── disk.img             # 32MB FAT16 harddisk block storage image
  └── initrd.tar           # USTAR Tar archive preloaded into RAM
```

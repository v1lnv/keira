# Changelog

All notable changes to the Keira Kernel project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.7.2] - 2026-07-15

### Changed
- Reverted default QEMU audio backend in `Makefile` to `none` (after testing/verifying sound generation via temporary `wav` backend) to prevent creating local files by default.

## [0.7.1] - 2026-07-15

### Changed
- Replaced hardcoded version strings in Rust source files (`kernel/src/entry.rs`, `kernel/src/shell.rs`, `kernel/src/shell/editor.rs`, and `kernel/src/shell/cmds/system.rs`) with dynamic compile-time package version fetching (`env!("CARGO_PKG_VERSION")`).
- Modified `Makefile` to dynamically grep and parse the active version from `Cargo.toml`.

## [0.7.0] - 2026-07-15

### Added
- Intel High Definition Audio (HDA) driver implemented in C (`drivers/sound/hda.c`) and integrated via Rust FFI.
- Exposed write configurations for PCI registers to enable device memory space and bus mastering.
- Allocated physical memory pages for stream DMA BDL (Buffer Descriptor List) and audio double buffers.
- Interactive shell command `hda` to play tone frequencies (`hda play <freq>`), stop playback (`hda stop`), and view device initialization status (`hda status`).
- Enabled HDA device support in QEMU flags in the Makefile.

## [0.6.2] - 2026-07-15

### Added
- Standard safety documentation (`# Safety`) for all unsafe functions.
- Bounded `strncpy` implementation to user-space string library.

### Changed
- Replaced insecure `strcpy` in user init process with bounded `strncpy` to address clang-tidy warning.
- Marked `exception_dispatcher` as `unsafe` to resolve Rust raw pointer dereference warning.
- Refactored `kernel/src/shell/executor.rs` and `kernel/src/shell.rs` to use `unwrap_or`, `unwrap_or_default`, and `strip_prefix` per Clippy suggestions.
- Refactored array indexing loops in scheduler to iterator-based queries.
- Fixed Makefile logging macros and command script generation loops to use `%s` instead of `%%s` so that actual filenames and command paths are correctly formatted and printed.

## [0.6.1] - 2026-07-08

### Changed
- Refactored PC Speaker driver into modular C implementation (`drivers/sound/`).
- Rust `sound.rs` now calls into C via FFI instead of using inline assembly.

### Added
- C driver: `sound.c`, `include/sound.h`, and `regs.h` with hardware register definitions.
- Registered `ahci.sys` and `sound.sys` driver placeholders in disk and initrd targets.
- Registered `pci`, `grep`, and `play` command binaries in disk and initrd targets.

## [0.6.0] - 2026-07-06

### Added
- PC Speaker sound driver supporting frequency output and note duration.
- Shell `play` command with built-in retro melodies (mario, nokia, starwars).

## [0.5.0] - 2026-07-05

### Added
- AHCI (SATA) DMA read/write driver supporting bare-metal transfers.
- Shell pipelines (`|`) and input redirection (`<`) for data streaming.
- `grep` command to search strings from files or stdin/pipe buffer.
- `sys_exec` system call (ID 5) in kernel and user library.

## [0.4.1] - 2026-07-04

### Changed
- Default startup shell theme set to `classic` (standard retro light red, light grey, and cyan accents).
- Implemented dynamic theme-aware color redirection in console driver so all shell commands automatically follow the active theme colors.

## [0.4.0] - 2026-07-03

### Added
- PCI Bus Scanner module detecting connected hardware devices, vendor/device IDs, and class codes.
- `pci` interactive shell command displaying a formatted table of all detected motherboard PCI devices.
- AHCI (SATA) Storage Controller Driver mapping MMIO registers (BAR5) and probing SATA disk ports.
- Native SATA Disk (`ahci0`) registration in the kernel storage block device manager.

## [0.3.1] - 2026-07-03

### Fixed
- F3 and F10 function key mapping in keyboard interrupt handler to enable save and exit hotkeys in visual editor.
- Match overflows in tab autocompletion by enforcing a maximum display limit of 10 options.
- Out-of-bounds array access guard inside fallback text-mode mouse cursor hiding routine.

## [0.3.0] - 2026-07-02

### Added
- Support for Multiboot2 linear framebuffer (LFB) widescreen graphics mode with auto-detected native screen resolutions.
- Memory-mapped identity page table allocations for high-address LFB video memory blocks.
- Graphics console driver in Rust featuring an embedded IBM VGA 8x16 bitmap font renderer.
- Horizontal memory shifting console scrolling and visual text cursor underline.
- Premium White Arrow mouse cursor with a black outline using background pixel save/restore to eliminate trails.
- Fullscreen mouse movement boundaries dynamically mapped to native widescreen resolutions in the PS/2 driver.
- System timer (PIT/IRQ0) driven blinking text cursor with interrupt-safe busy guards (`VGA_BUSY`).
- Premium clean-look shell prompt utilizing a single angle bracket (`>`) and subtle host styling.
- Admin username visual color highlighting (LightRed) to indicate root privilege level.

## [0.2.0] - 2026-07-02

### Added
- Visual text editor improvements including a 5-column line numbering gutter.
- Fast interactive text search (Ctrl+F) with dynamic high-contrast match highlighting in the editor.
- Visual crash dump handling (Blue Screen of Death) rendering diagnostics on a White on Blue VGA screen.
- Enhanced batch script runner command (`script`) with robust 64KB static buffer allocations.

## [0.1.0] - 2026-07-02

### Added
- Initial release of the Keira Kernel.
- Freestanding x86_64 CPU bootstrap from 32-bit Protected Mode to 64-bit Long Mode.
- Physical Memory Manager (bitmap frames) and Virtual Memory Manager (4-level PML4 tables).
- Preemptive Round-Robin multitasking thread scheduler and TSS stack isolation.
- CPU privilege separation supporting Ring 3 user mode applications via STAR/LSTAR MSR syscalls.
- FAT16 storage block device driver supporting file and directory creation, reading, and writing.
- Unified boot archive (initrd.tar) memory USTAR parser with safe filesystem lookup fallbacks.
- Interactive keyboard and mouse input drivers, supporting text-rendering and mouse pointer tracking.
- Terminal shell loop featuring tab autocomplete, scrollback command history, and guidance guides.
- Full-screen text-mode visual editor (edit command) supporting 2D cursor navigation and F3/F10 controls.
- Dynamic terminal theme engine (theme command) supporting five color palettes with theme-aware VGA screen clearing.
- GitHub Actions CI kernel compiler workflows, weekly Dependabot updates, and community templates (bug reports, feature requests, PR templates, and security policies).

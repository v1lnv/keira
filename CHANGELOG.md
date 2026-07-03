# Changelog

All notable changes to the Keira Kernel project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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

# Keira Kernel

The Keira kernel is a freestanding, custom 64-bit operating system kernel written in a modular combination of C, Assembly (x86_64), and Rust (no_std). Hardware drivers are implemented in C and exposed to Rust via FFI for a clean separation between low-level I/O and high-level logic. **Keira v0.11.1 features the Keira C Compiler (kcc) in user space, supporting freestanding dynamic C compilation directly on the OS.**

## Quick Start

* Report a bug: See .github/SECURITY.md
* Get the latest kernel: https://github.com/v1lnv/keira
* Build the kernel: See docs/development/building.md

## Essential Documentation

All users should be familiar with:

* Building requirements: docs/development/building.md
* Code of Conduct / Contribution: CONTRIBUTING.md
* License: See LICENSE

Documentation can be read locally in the docs/ directory.


## Who Are You?

Find your role below:

* New Kernel Developer - Getting started with kernel development
* Academic Researcher - Studying kernel internals and multitasking
* Security Expert - Ring isolation and system call security
* System Administrator - Configuring and troubleshooting storage
* Maintainer - Leading subsystems and code review


## For Specific Users

### New Kernel Developer

Welcome! Start your kernel development journey here:

* Getting Started: docs/development/building.md
* Creating Custom Commands: docs/development/new_command.md
* Writing User-Space Programs: docs/development/user_apps.md
* Coding Style: CONTRIBUTING.md

### Academic Researcher

Explore the kernel's architecture and internals:

* Bootstrapping & 64-bit Long Mode: docs/architecture/bootstrapping.md
* Memory Management (PMM & VMM): docs/architecture/memory.md
* Thread Scheduler: docs/architecture/scheduler.md
* Interrupt Handling & IDT: docs/architecture/interrupts.md
* System Call Dispatcher: docs/architecture/syscalls.md

### Security Expert

Security documentation and hardening guides:

* System Calls & Ring Isolation: docs/architecture/syscalls.md
* Reporting Vulnerabilities: .github/SECURITY.md

### System Administrator

Configure, tune, and troubleshoot Keira systems:

* IDE Block Storage: docs/storage/block.md
* PCI Bus Scanner & AHCI (SATA) Driver: docs/storage/pci_ahci.md
* FAT16 Filesystem: docs/storage/fat16.md
* USTAR Boot RAM Disk (Initrd): docs/storage/initrd.md
* Shell Event Loop and Input: docs/shell/loop.md
* Full-Screen Text Editor: docs/shell/editor.md
* VGA Theme Engine: docs/shell/themes.md
* PC Speaker Sound Driver (C + Rust FFI): docs/shell/sound.md

### Maintainer

Lead kernel subsystems and manage contributions:

* Development & Contribution: CONTRIBUTING.md
* Target Platform Configs: targets/


## Communication and Support

* GitHub Repository: https://github.com/v1lnv/keira
* Issues & Bug Tracker: https://github.com/v1lnv/keira/issues
* Developer Email: v1lnv@proton.me
* License: Copyright (c) 2026 V1lnv. Licensed under the MIT License.

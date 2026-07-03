# Keira Kernel

Keira is a freestanding, custom 64-bit operating system kernel written in a modular combination of C, Assembly (x86_64), and Rust (no_std).

## Documentation

The primary developer and technical documentation for Keira is located in the [docs/](docs/) directory. Please refer to the corresponding subdirectories for detailed design specifications:

* **[docs/architecture/](docs/architecture/README.md)**: Bootstrapping, long mode initialization, PML4 paging tables, interrupt handling, preemptive scheduler, and system calls.
* **[docs/storage/](docs/storage/README.md)**: IDE block storage driver, PCI bus scanner, AHCI (SATA) disk driver, FAT16 filesystem specification, and USTAR boot RAM disk.
* **[docs/shell/](docs/shell/README.md)**: Shell event loop, visual text editor, and theme-aware graphics console engine.
* **[docs/development/](docs/development/README.md)**: Host toolchain setups, kernel debugging with GDB, and guide to creating custom shell commands.

## Build and Run

To compile the kernel and run it under the QEMU emulator, make sure you have the required build tools (`nasm`, `gcc`, `make`, `xorriso`, `mtools`, `qemu-system-x86`) installed on your host system, then execute:

```bash
# Clean, compile, and run immediately in QEMU
make run

# Clean build artifacts
make clean
```

## Contributing and Security

Please review [CONTRIBUTING.md](CONTRIBUTING.md) for contribution guidelines, coding style rules, and PR workflows.

To report security vulnerabilities privately, please read our [Security Policy](.github/SECURITY.md).

## License

Copyright (c) 2026 V1lnv. Licensed under the MIT License.

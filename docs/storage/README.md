# Storage & Filesystem Documentation Index

This directory documents the disk access layers, cluster maps, directory metadata structures, and boot archives of the Keira Kernel.

## Modules

- **[IDE Block Storage Driver](block.md)**
  Probing primary master IDE channels and reading/writing disk sectors via LBA28.
- **[FAT16 Filesystem Specification](fat16.md)**
  FAT tables, BIOS Parameter Block, directory entries, cluster allocations, and read/write implementation.
- **[USTAR Boot RAM Disk (Initrd)](initrd.md)**
  In-memory boot Tar archive reading and unified filesystem fallbacks.

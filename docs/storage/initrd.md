# USTAR Boot RAM Disk (Initrd)

This module describes the USTAR Tar RAM Disk parser and the unified fallback filesystem mechanism in the Keira Kernel.

---

## 1. Initrd RAM Disk Structure (USTAR)

The boot RAM Disk is packed as a standard USTAR Tar archive (`initrd.tar`) and loaded into memory by the GRUB bootloader as a multiboot module.

### USTAR Header Format
Every file in the tar archive starts with a 512-byte header containing:
- **Filename**: Offset `0`, length 100 bytes (null-terminated).
- **File Size**: Offset `124`, length 12 bytes (octal ASCII format).
- **Magic String**: Offset `257`, length 6 bytes (must contain `"ustar"`).
- **File Type**: Offset `156`, length 1 byte (`'0'` or `'\0'` for normal file, `'5'` for directory).

The actual file content starts immediately in the subsequent 512-byte sector following the header, padded to a multiple of 512 bytes.

---

## 2. In-Memory TAR Parser

The kernel implements a lightweight in-memory tar scanner in `kernel/src/fs/tar/reader.rs`:
- Receives the starting physical memory address and total length of the loaded `initrd.tar`.
- **Search Logic**: To locate a file, the parser iterates through headers. It reads the filename, parses the octal file size, and moves the pointer forward by `512 + aligned_file_size` bytes to inspect the next header.
- If a path matches, it returns a pointer slice directly referencing the file content inside the memory-mapped RAM disk.

---

## 3. Unified Filesystem Fallback

To provide a robust, self-contained shell environment, file reading commands (like `view` and `run`) use a unified fallback lookup:
1. **FAT16 Check**: The filesystem driver first checks if a block storage drive is mounted and searches for the requested file path on the FAT16 partition.
2. **Initrd Fallback**: If no block device is mounted, or if the file is not found on the FAT16 drive, the driver searches for the path inside the read-only memory-mapped `initrd.tar` archive.
3. **Execution**: This enables booting the kernel and launching system commands instantly from the RAM disk, while allowing users to mount and modify their writeable FAT16 partitions dynamically.

# FAT16 Filesystem Specification

This module describes the FAT16 filesystem implementation, covering BPB parsing, cluster tracking, directory entries, and file creation, deletion, reading, and writing.

---

## 1. BIOS Parameter Block (BPB) Parsing

When mounting a FAT16 partition (using the `use` command), the filesystem driver reads Sector 0 (the Boot Sector) to initialize the volume metadata:

| BPB Offset | Size | Description |
| ---------- | ---- | ----------- |
| `0x0B` | 2 Bytes | Bytes Per Sector (must be 512). |
| `0x0D` | 1 Byte | Sectors Per Cluster (e.g., 4, 8, 16). |
| `0x0E` | 2 Bytes | Reserved Sector Count (sectors before FAT starts). |
| `0x10` | 1 Byte | Number of FAT tables (usually 2). |
| `0x11` | 2 Bytes | Maximum Root Directory Entries (usually 512). |
| `0x16` | 2 Bytes | Sectors Per FAT (size of one FAT copy). |

From these values, the driver calculates:
- The starting sector of FAT1.
- The starting sector of the Root Directory.
- The starting sector of the Data Area (Cluster 2).

---

## 2. Directory Entry Layout

Directories contain an array of 32-byte directory entry records:

| Offset | Size | Description |
| ------ | ---- | ----------- |
| `0x00` | 8 Bytes | Filename (padded with spaces). |
| `0x08` | 3 Bytes | File Extension (padded with spaces). |
| `0x0B` | 1 Byte | File Attributes (0x10 = Directory, 0x08 = Volume ID, 0x02 = Hidden). |
| `0x1A` | 2 Bytes | First Cluster Low (starting cluster index). |
| `0x1C` | 4 Bytes | File Size in Bytes. |

If the first character of the filename is `0xE5`, the entry is marked as deleted. If it is `0x00`, the entry is free and terminates the directory list.

---

## 3. FAT Cluster Traversal

The File Allocation Table (FAT) is a linked list of cluster indices stored in sectors:
- Every 16-bit word in the FAT represents the status or next cluster offset of a data cluster.
- **Traversal Logic**: To read a file, the driver reads the cluster value from the directory entry. It fetches the next cluster index by reading word `index` from the active FAT sector.
- **Chain Termination**: A value between `0xFFF8` and `0xFFFF` indicates the End of File (EOF) cluster.

---

## 4. File I/O Operations

### Creating a File
1. Validates that the file does not already exist.
2. Finds a free directory entry block (a record starting with `0x00` or `0xE5`).
3. Writes the filename (converted to 8.3 format), attributes (`0x00` for file), starting cluster (`0`), and file size (`0`).

### Writing File Content
1. Allocates a free cluster chain in the FAT table.
2. Writes content sectors to the assigned data cluster areas.
3. Updates the file's directory entry record with the starting cluster index and final size.
4. Marks the end of the FAT chain with `0xFFFF`.

### Deleting a File
1. Resolves the file's directory entry.
2. Traverses the FAT chain starting from the file's first cluster, marking each cluster word in the FAT table as `0x0000` (free).
3. Overwrites the first byte of the file's directory entry filename with `0xE5` (deleted) and flushes the sector to disk.

---

## 5. Long File Names (LFN) Support

Keira v0.8.0 implements support for FAT Long File Names (LFN):
- **LFN Entry Format**: An LFN entry has attributes set to `0x0F` (a combination of Read Only, Hidden, System, and Volume ID attributes that prevents legacy systems from displaying them). It contains 13 Unicode (UTF-16) characters distributed across three name parts.
- **Sequence Mapping**: LFN entries occur sequentially *before* their associated 8.3 directory entry, in reverse order. The sequence byte indicates their index (1-based), with the final logical LFN entry having bit 6 (`0x40`) set.
- **LFN Accumulator**: As the driver traverses directory sectors, it parses sequential entries with attribute `0x0F` and accumulates their characters in a `LfnAccumulator` buffer at the appropriate sequence offsets. When a standard directory entry is reached, the accumulated characters are decoded into a UTF-8 string, resetting the accumulator for the next entry.

---

## 6. Virtual File System (VFS) & Mounts

To unify file interactions across multiple filesystems (such as FAT16 on hard disk and TAR on RAM disk/Initrd), Keira provides a **Virtual File System (VFS)** routing layer:
- **Unified Path Routing**: The VFS layer intercepts paths and identifies their destination based on path prefixes.
  - Paths prefixed with `/initrd/` are routed to the read-only `tar` filesystem.
  - All other absolute or relative paths default to the read-write `fat` filesystem.
- **Unified APIs**: System components and shell commands access data exclusively using `vfs::read_file`, `vfs::write_file`, `vfs::create_file`, and `vfs::remove_entry` rather than calling the FAT16 or Tar drivers directly.

# Memory Management (PMM & VMM)

This module describes the memory management architectures, detailing physical frame allocations and virtual memory paging.

---

## 1. Physical Memory Manager (PMM)

The Physical Memory Manager tracks available physical memory in 4KB chunks (frames) using a bitmap.

### Frame Allocation Bitmap
- Every bit in the bitmap represents a 4KB physical frame.
- A value of `0` indicates the frame is free; a value of `1` indicates it is allocated.
- The bitmap is sized statically to support up to the maximum capacity of physical RAM reported.

### Memory Map Detection
During kernel initialization, the PMM parses the Multiboot2 bootloader tags:
- Detects the locations of reserved kernel segments, the multiboot structure itself, and unusable memory blocks.
- Free memory frames are marked as `0` (available) in the PMM bitmap.

---

## 2. Virtual Memory Manager (VMM)

The x86_64 architecture uses 4-level paging to map 64-bit virtual memory addresses to physical frames.

### Page Directory Map Structure
Virtual address translation resolves through 4 tables:
```
Virtual Address
  |
  +---> PML4 Offset (9 bits)  ---> Index into PML4 Table
  |
  +---> PDPT Offset (9 bits)  ---> Index into PDPT Table
  |
  +---> PD Offset (9 bits)    ---> Index into Page Directory
  |
  +---> PT Offset (9 bits)    ---> Index into Page Table
  |
  +---> Frame Offset (12 bits) ---> Offset inside 4KB Physical Frame
```

### Identity and Custom Mapping
- **Identity Mapping**: The first 1GB is identity-mapped during boot to allow the kernel and critical device drivers (VGA text mode, COM1) to execute transparently.
- **Linear Framebuffer (LFB) Mapping**: When a graphics mode linear framebuffer is provided by GRUB, the VMM maps the framebuffer's physical memory pages (which can reside above the 1GB boundary) as writable identity-mapped virtual addresses to allow safe widescreen pixel drawing.
- **Dynamic Paging**: VMM page allocation dynamically maps virtual address pages to physical frames on demand when allocating process space.

---

## 3. Kernel C Heap Allocator

A dedicated heap memory manager is implemented in `mm/heap/heap.c` for low-level memory allocations inside the C segments:
- **Heap Range**: Allocates a static 1MB virtual memory block.
- **Metadata Blocks**: Implements a linked-list allocator tracking headers for free and allocated chunks.
- **Allocation Rules**:
  - `malloc(size)`: Performs a first-fit search in the free block list, splits the block if it is larger than requested, and returns the pointer.
  - `free(ptr)`: Merges adjacent free blocks immediately (coalescing) to prevent heap fragmentation.

---

## 4. User-Space Heap & Program Break

Keira v0.9.0 introduces dynamic user-space heap allocation:
- **Program Break tracking**: Each process tracks its active heap boundaries using `program_break` and `program_break_start` within its `Task` structure.
- **Virtual Memory Region**: The user heap is configured to start at address `0x600000000000` (isolated from the stack at `0x7FFFFFFF0000` and binary segments).
- **System Call `sys_sbrk`**: Expands or shrinks the program break pointer:
  - Aligns boundary changes to 4KB page frames.
  - Dynamically maps newly allocated physical pages with User + Writable permissions into the page tables.
  - Automatically unmaps and reclaims physical page frames on shrinking.
- **User-Space Allocator (`malloc` & `free`)**: Built as an implicit free-list memory manager inside `libkeira`, providing aligned, split-block, and coalesced block mappings on top of `sys_sbrk`.
- **Automatic Heap Cleanup**: When a user process exits, the loader iterates from `0x600000000000` up to `program_break`, calling `vmm::free_and_unmap_page` to release all allocated physical frames and clear the page table mappings to prevent memory leaks.

---

## 5. ELF Loader Memory Management

Keira v0.10.0 introduces full memory lifecycle management for loaded ELF binaries:
- **Expanded File Buffer**: The ELF file read buffer has been increased from 32KB to **64KB** to accommodate larger user-space binaries.
- **Address Space Isolation**: In Keira v0.13.0, the manual save and restore of parent mappings during loading has been removed. The ELF loader instead maps program segments and user stack directly into a new cloned address space.

---

## 6. Per-Process Address Spaces (v0.13.0)

Keira v0.13.0 implements true memory isolation between processes using per-process Page Map Level 4 (PML4) tables:
- **PML4 Cloning**: Each user task gets its own PML4 table via `clone_kernel_pml4`. The new PML4 shares only `PDPT[0]` (the kernel's 1GB identity map) from the boot PML4, while all user-space entries remain completely isolated.
- **CR3 Switching**: The scheduler updates the active page directory by writing the physical address of the task's PML4 to the `CR3` register during context switch.
- **Automatic Cleanup**: When a user process terminates, `vmm::free_user_pages` recursively traverses its PML4 table to free all mapped physical frames (code, data, heap, and stack) and intermediate page table structures (PDPT, PD, PT), eliminating memory leaks.

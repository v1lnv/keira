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
- **Identity Mapping**: The first 1GB is identity-mapped during boot to allow the kernel and critical device drivers (VGA, COM1) to execute transparently.
- **Dynamic Paging**: VMM page allocation dynamically maps virtual address pages to physical frames on demand when allocating process space.

---

## 3. Kernel C Heap Allocator

A dedicated heap memory manager is implemented in `mm/heap/heap.c` for low-level memory allocations inside the C segments:
- **Heap Range**: Allocates a static 1MB virtual memory block.
- **Metadata Blocks**: Implements a linked-list allocator tracking headers for free and allocated chunks.
- **Allocation Rules**:
  - `malloc(size)`: Performs a first-fit search in the free block list, splits the block if it is larger than requested, and returns the pointer.
  - `free(ptr)`: Merges adjacent free blocks immediately (coalescing) to prevent heap fragmentation.

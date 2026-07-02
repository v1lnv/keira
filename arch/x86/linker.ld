/*
 * Keira Kernel: Linker Script
 *
 * Controls the memory layout of the final kernel ELF binary.
 *
 * Key design decisions:
 *   - The kernel is loaded at 1 MiB (0x100000), which is above the legacy BIOS/VGA region.
 *   - The .multiboot_header section is placed first to ensure GRUB can locate it
 *     within the first 32 KiB of the file.
 *   - All sections are aligned on 4096-byte boundaries for paging compatibility.
 *   - BSS boundary symbols are exported so that the bootstrap code can zero-initialize the region.
 *
 * Section ordering:
 *   .multiboot_header -> .text -> .rodata -> .data -> .bss
 *
 * Reference: https://wiki.osdev.org/Linker_Scripts
 */

/* Entry point: the 32-bit assembly bootstrap function */
ENTRY(_start)

SECTIONS
{
    /* Start the kernel at the 1 MiB physical address mark.
     * Everything below 1 MiB is reserved by the BIOS, VGA, and legacy devices. */
    . = 1M;

    /* 
     * .multiboot_header: Must be first in the binary
     *
     * GRUB scans the first 32 KiB of the kernel file to locate the Multiboot2
     * magic number. Placing this section first guarantees that GRUB finds it
     * immediately.
     */
    .multiboot_header : ALIGN(8)
    {
        KEEP(*(.multiboot_header))
     }

    /* 
     * .text: Executable code from all languages
     *
     * Combines code from assembly (.o), C (.o), and Rust (.a) objects into
     * a single contiguous region. This section is page-aligned for memory
     * execution mapping.
     */
    .text : ALIGN(4096)
    {
        *(.text)
        *(.text.*)
    }

    /* 
     * .rodata: Read-only data (string literals, GDT, constants)
     *
     * Separated from the .text section so it can be mapped without execute
     * permissions in future memory protection enhancements.
     */
    .rodata : ALIGN(4096)
    {
        *(.rodata)
        *(.rodata.*)
    }

    /* 
     * .data: Initialized writable data (global variables with initial values)
     */
    .data : ALIGN(4096)
    {
        *(.data)
        *(.data.*)
    }

    /* 
     * .bss: Zero-initialized writable data
     *
     * The __bss_start and __bss_end symbols are used by arch/x86/boot/entry64.asm
     * to zero-initialize this region before calling C or Rust code.
     *
     * Note: The BSS section does not occupy physical space in the binary file.
     * It is allocated in memory at load time.
     */
    .bss : ALIGN(4096)
    {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        __bss_end = .;
    }

    /* 
     * .page_tables: CPU page table structures
     *
     * Placed after the .bss section so that BSS zero-initialization (in arch/x86/boot/entry64.asm)
     * does not destroy the page table entries written by setup_page_tables() in
     * arch/x86/kernel/paging.asm during the 32-bit bootstrap phase.
     */
    .page_tables : ALIGN(4096)
    {
        *(.page_tables)
    }

    /*
     * Kernel Heap Region
     *
     * A 1 MiB region reserved for dynamic memory allocation (kmalloc/kfree).
     * Placed after all code and data sections.
     */
    . = ALIGN(4096);
    __heap_start = .;
    . += 1M;
    __heap_end = .;

    /* 
     * Discard sections that are not needed
     */
    /DISCARD/ :
    {
        *(.comment)
        *(.note)
        *(.note.*)
        *(.eh_frame)
        *(.eh_frame_hdr)
    }
}

; Keira Kernel: Shared Assembly Constants
;
; Central definition of all magic numbers, physical memory addresses, and
; configuration values used across the assembly bootstrap modules.

; Multiboot2 Protocol Constants
MULTIBOOT2_MAGIC        equ 0xE85250D6  ; Magic number identifying Multiboot2
MULTIBOOT2_ARCH_I386    equ 0           ; Architecture: i386 (32-bit protected mode)
MULTIBOOT2_BOOTLOADER   equ 0x36D76289  ; Expected EAX value after Multiboot2 boot

; Memory Layout
KERNEL_PHYS_BASE        equ 0x00100000  ; Kernel load address (1 MiB physical)
KERNEL_STACK_SIZE       equ 0x4000      ; 16 KiB bootstrap stack size (4 pages)

; VGA Text Mode Constants
VGA_BUFFER_ADDR         equ 0xB8000     ; Physical address of the VGA text buffer
VGA_COLUMNS             equ 80          ; Number of columns in VGA text mode
VGA_ROWS                equ 25          ; Number of rows in VGA text mode
VGA_ATTR_WHITE_ON_BLACK equ 0x0F        ; Color attribute: white text on black background
VGA_ATTR_RED_ON_BLACK   equ 0x0C        ; Color attribute: light red text on black background

; Serial Port UART 16550A
COM1_PORT               equ 0x3F8       ; Base I/O port for COM1 serial interface

; CPU Control Registers and Model-Specific Registers (MSR)
CR0_PAGING_BIT          equ (1 << 31)   ; CR0.PG: Enable paging
CR0_PROTECTED_BIT       equ (1 << 0)    ; CR0.PE: Protected mode enable
CR4_PAE_BIT             equ (1 << 5)    ; CR4.PAE: Physical Address Extension

EFER_MSR                equ 0xC0000080  ; Extended Feature Enable Register MSR address
EFER_LONG_MODE_BIT      equ (1 << 8)    ; EFER.LME: Long Mode Enable bit

; Page Table Flags
PAGE_PRESENT            equ (1 << 0)    ; Page is present in physical memory
PAGE_WRITABLE           equ (1 << 1)    ; Page is writable
PAGE_HUGE               equ (1 << 7)    ; 2 MiB huge page identifier (used in PD entries)
PAGE_RW_PRESENT         equ (PAGE_PRESENT | PAGE_WRITABLE)

; GDT Segment Selectors
GDT_NULL_SEL            equ 0x00        ; Null descriptor selector
GDT_CODE64_SEL          equ 0x08        ; 64-bit code segment selector
GDT_DATA64_SEL          equ 0x10        ; 64-bit data segment selector

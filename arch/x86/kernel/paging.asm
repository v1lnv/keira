; Keira Kernel: Page Table Setup (Identity Mapping)
;
; Configures 4-level paging (PML4 -> PDPT -> PD) to identity-map the first 2 MiB
; of physical memory using a single 2 MiB huge page.
;
; This is the minimum paging structure required to enter long mode:
;   PML4[0] points to PDPT
;   PDPT[0] points to PD
;   PD[0]   points to a 2 MiB huge page at physical address 0x00000000
;
; Why identity mapping is required:
;   After paging is enabled and before jumping to 64-bit code, the CPU translates
;   instructions through the page tables. If the virtual address of the executing
;   code does not match its physical address, the CPU will triple-fault.
;   Identity mapping ensures that virtual addresses equal physical addresses
;   for the bootstrap code region.
;
; Reference: Intel SDM Vol. 3A, Section 4.5 (4-Level Paging)

%include "constants.inc"

; Page Table Entries (allocated in a dedicated section, not in .bss)
; Each table is 4096 bytes (512 entries x 8 bytes per entry).
; These tables must be 4096-byte aligned. They are placed in the .page_tables
; section so that the BSS zeroing step does not overwrite them.
section .page_tables write nobits
align 4096

global pml4_table
pml4_table:
    resb 4096                           ; Page Map Level 4 (512 entries)

global pdpt_table
pdpt_table:
    resb 4096                           ; Page Directory Pointer Table

global pd_table
pd_table:
    resb 4096                           ; Page Directory (holds 2 MiB huge pages)

; setup_page_tables: Initialize identity mapping for the first 2 MiB
; Called from arch/x86/boot/entry32.asm in 32-bit protected mode (before long mode).
; Modifies: EAX
; Preserves: EBX (Multiboot2 information pointer)
section .text
bits 32

global setup_page_tables
setup_page_tables:
    ; Step 1: PML4[0] = address of PDPT | PRESENT | WRITABLE
    mov eax, pdpt_table
    or  eax, PAGE_RW_PRESENT
    mov [pml4_table], eax

    ; Step 2: PDPT[0] = address of PD | PRESENT | WRITABLE
    mov eax, pd_table
    or  eax, PAGE_RW_PRESENT
    mov [pdpt_table], eax

    ; Step 3: Map all 512 entries in pd_table as 2 MiB huge pages (total 1 GiB identity mapped)
    mov ecx, 0
.map_loop:
    mov eax, ecx
    shl eax, 21                         ; Page physical address = ecx * 2 MiB
    or  eax, PAGE_PRESENT | PAGE_WRITABLE | PAGE_HUGE
    mov [pd_table + ecx * 8], eax
    mov dword [pd_table + ecx * 8 + 4], 0
    inc ecx
    cmp ecx, 512
    jne .map_loop

    ret
    
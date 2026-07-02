; Keira Kernel: 32-bit Entry Point
;
; This is the initial entry point executed after the GRUB bootloader loads the
; kernel. GRUB boots the CPU into 32-bit protected mode with the following state:
;   - EAX: Multiboot2 magic number (0x36D76289)
;   - EBX: Physical address of the Multiboot2 information structure
;   - Paging: Disabled
;   - Interrupts: Disabled
;   - A20 line: Enabled
;
; This code validates the bootloader, builds page tables, enables long mode,
; loads the 64-bit Global Descriptor Table (GDT), and performs a far jump
; to transition into 64-bit mode.
;
; Reference: Multiboot2 Specification, Section 3.3 (Machine State)

%include "constants.inc"

; External symbols defined in other modules
extern setup_page_tables               ; arch/x86/kernel/paging.asm: builds identity-mapped page tables
extern gdt_descriptor                  ; arch/x86/kernel/gdt.asm: descriptor pointing to the GDT
extern _start64                        ; arch/x86/boot/entry64.asm: the 64-bit entry point

; 32-bit Bootstrap Stack
; This stack is temporarily used during the transition to 64-bit mode.
section .bss
align 16

stack_bottom:
    resb KERNEL_STACK_SIZE              ; Allocate 16 KiB of stack space
global stack_top
stack_top:

; _start: Kernel entry point called by the bootloader
section .text
bits 32
global _start

_start:
    ; Step 1: Set up the temporary stack pointer
    mov esp, stack_top

    ; Step 2: Save the Multiboot2 information structure address (EBX)
    ; The pointer is saved on the stack so that arch/x86/boot/entry64.asm can retrieve it.
    push ebx

    ; Step 3: Verify that the kernel was loaded by a Multiboot2-compliant bootloader
    cmp eax, MULTIBOOT2_BOOTLOADER
    jne .halt_no_multiboot

    ; Step 4: Construct the page tables to identity-map the first 2 MiB
    call setup_page_tables

    ; Step 5: Load the address of the PML4 page table into the CR3 register
    extern pml4_table
    mov eax, pml4_table
    mov cr3, eax

    ; Step 6: Enable Physical Address Extension (PAE) in the CR4 register
    ; PAE is required to enable long mode.
    mov eax, cr4
    or  eax, CR4_PAE_BIT
    mov cr4, eax

    ; Step 7: Enable long mode in the Extended Feature Enable Register (IA32_EFER MSR)
    mov ecx, EFER_MSR
    rdmsr                               ; Read the EFER MSR into EDX:EAX
    or  eax, EFER_LONG_MODE_BIT         ; Set the Long Mode Enable (LME) bit
    wrmsr                               ; Write the updated value back to the MSR

    ; Step 8: Enable paging in the CR0 register
    ; This activates long mode because the LME bit is already set in the EFER MSR.
    ; The CPU enters compatibility mode, which executes 32-bit code in a long mode context
    ; until a far jump is performed.
    mov eax, cr0
    or  eax, CR0_PAGING_BIT
    mov cr0, eax

    ; Step 9: Load the 64-bit Global Descriptor Table
    lgdt [gdt_descriptor]

    ; Step 10: Perform a far jump to the 64-bit code segment
    ; This transition switches the CPU from 32-bit compatibility mode to full 64-bit long mode
    ; by loading CS with the 64-bit code selector.
    jmp GDT_CODE64_SEL:_start64

; Error Handlers
; These routines output an error character to the VGA buffer and halt the execution.

.halt_no_multiboot:
    ; Print 'M' (Multiboot error) at the top-left corner of the VGA screen
    mov dword [VGA_BUFFER_ADDR], 0x4F4D ; Character 'M' with white text on a red background
    jmp .halt

.halt:
    cli                                 ; Disable interrupts
    hlt                                 ; Halt CPU execution
    jmp .halt                           ; Prevent execution from falling through if a spurious wakeup occurs
    
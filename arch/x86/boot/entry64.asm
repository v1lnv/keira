; Keira Kernel: 64-bit Entry Trampoline
;
; This entry point is reached via a far jump from arch/x86/boot/entry32.asm after long
; mode has been enabled. The CPU is now operating in full 64-bit mode.
;
; Responsibilities:
;   1. Initialize segment registers to clear outdated 32-bit state
;   2. Establish the 64-bit stack pointer (RSP)
;   3. Zero-initialize the BSS section
;   4. Initialize hardware by calling hw_init()
;   5. Hand over execution to the Rust kernel entry point: kernel_main()
;   6. Halt the CPU as a fallback if kernel_main ever returns

%include "constants.inc"

; External entry points for hardware initialization and Rust kernel main
extern hw_init                          ; drivers/c/hw_init.c
extern kernel_main                      ; kernel/rust/src/entry.rs

; BSS boundaries defined by the linker script
extern __bss_start
extern __bss_end

; Stack defined in arch/x86/boot/entry32.asm
extern stack_top

; _start64: 64-bit entry point
section .text
bits 64
global _start64

_start64:
    ; Save the Multiboot2 information pointer before the BSS section is cleared.
    ; arch/x86/boot/entry32.asm pushed this pointer to the stack (at stack_top - 4).
    ; We store it in R12 (a callee-saved register) to prevent hw_init from overwriting it.
    mov r12d, dword [rel stack_top - 4]

    ; Step 1: Initialize segment registers
    ; In 64-bit mode, segment selectors like DS, ES, and SS are largely unused,
    ; but they should be set to a valid data segment selector for consistency
    ; and to prevent undefined CPU behavior.
    mov ax, GDT_DATA64_SEL
    mov ds, ax
    mov es, ax
    mov ss, ax
    xor ax, ax                          ; FS and GS are typically zeroed
    mov fs, ax
    mov gs, ax

    ; Step 2: Set up the 64-bit stack pointer
    ; Use the same stack space allocated in arch/x86/boot/entry32.asm
    mov rsp, stack_top

    ; Step 3: Zero-initialize the BSS section
    ; Standard C and Rust ABIs require the BSS section to be zero-initialized.
    ; The boundary symbols are defined in the linker script.
    mov rdi, __bss_start                ; Destination address: start of BSS
    mov rcx, __bss_end                  ; End address of BSS
    sub rcx, rdi                        ; Calculate BSS size in bytes
    xor al, al                          ; Set fill value to zero
    rep stosb                           ; Fill the region with zeros

    ; Step 4: Perform early hardware initialization
    ; hw_init configures the serial interface, VGA output, and displays the boot banner.
    call hw_init

    ; Step 5: Transfer execution to the Rust kernel entry point
    ; Pass the saved Multiboot2 information pointer in the first argument register (RDI).
    mov rdi, r12
    call kernel_main

    ; Step 6: Halt the CPU (fallback in case kernel_main returns)
.hang:
    cli                                 ; Disable interrupts
    hlt                                 ; Halt CPU execution
    jmp .hang                           ; Loop to catch spurious wakeups
    
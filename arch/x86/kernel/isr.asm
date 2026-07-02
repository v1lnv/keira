; Keira Kernel: Interrupt Service Routines (ISRs)
;
; This file contains the assembly stubs for handling hardware interrupts.
; The stubs save the CPU state, call the appropriate C/Rust handler,
; restore the CPU state, and return using the `iretq` instruction.

section .text
bits 64

; Macro to save all general-purpose registers (15 registers, 120 bytes)
%macro pushaq 0
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

; Macro to restore all general-purpose registers
%macro popaq 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax
%endmacro

global isr32
global isr33
global isr44

extern isr_handler
extern keyboard_handler
extern mouse_handler
extern pit_handler
extern schedule_tick
extern exception_dispatcher

%macro exception_no_err 1
global exception%1
exception%1:
    push 0      ; Dummy error code
    push %1     ; Vector number
    jmp exception_common
%endmacro

%macro exception_err 1
global exception%1
exception%1:
    push %1     ; Vector number
    jmp exception_common
%endmacro

exception_no_err 0
exception_no_err 1
exception_no_err 2
exception_no_err 3
exception_no_err 4
exception_no_err 5
exception_no_err 6
exception_no_err 7
exception_err    8
exception_no_err 9
exception_err    10
exception_err    11
exception_err    12
exception_err    13
exception_err    14
exception_no_err 15
exception_no_err 16
exception_err    17
exception_no_err 18
exception_no_err 19
exception_no_err 20
exception_err    21
exception_no_err 22
exception_no_err 23
exception_no_err 24
exception_no_err 25
exception_no_err 26
exception_no_err 27
exception_no_err 28
exception_no_err 29
exception_no_err 30
exception_no_err 31

global exception_common
exception_common:
    pushaq
    mov rdi, rsp
    call exception_dispatcher
    popaq
    add rsp, 16
    iretq

; ISR 32: PIT Timer (IRQ 0). Performs preemptive context switching between tasks.
isr32:
    ; 1. Save general-purpose registers
    pushaq

    ; 2. Call C pit_handler to update ticks and EOI
    call pit_handler

    ; 3. Perform scheduler context switch
    mov rdi, rsp                        ; Pass the pointer to the current stack frame as the 1st argument
    call schedule_tick                  ; Returns the next task's RSP in RAX
    mov rsp, rax                        ; Switch stack pointer to the next task's RSP

    ; 4. Restore the new task's general-purpose registers and return
    popaq
    iretq

; ISR 33: Keyboard (IRQ 1)
isr33:
    pushaq
    call keyboard_handler
    popaq
    iretq

; ISR 44: PS/2 Mouse (IRQ 12)
isr44:
    pushaq
    call mouse_handler
    popaq
    iretq
    
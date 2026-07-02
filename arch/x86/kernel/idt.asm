; Keira Kernel: IDT Load Routine
;
; Loads the Interrupt Descriptor Table (IDT) into the CPU register.

global idt_load

section .text
bits 64

; Function: void idt_load(uint64_t idt_ptr)
; Under the SysV AMD64 ABI, the first argument is passed in the RDI register.
idt_load:
    lidt [rdi]
    ret
    
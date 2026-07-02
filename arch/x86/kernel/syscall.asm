; Keira Kernel: Assembly System Call & User Mode Handlers
;
; Provides setup routines for the syscall/sysret MSRs, the central
; syscall assembly entry point, and the user mode transition handler.

global init_syscall_msrs
global syscall_handler_asm
global jump_to_user

global user_rsp_temp
global kernel_stack_temp

extern syscall_dispatcher

section .data
align 8
user_rsp_temp:     dq 0
kernel_stack_temp:   dq 0

section .text
bits 64

; init_syscall_msrs: Set up IA32_STAR, IA32_LSTAR, IA32_SFMASK, and EFER registers
init_syscall_msrs:
    ; 1. Enable System Call Extensions (SCE) in EFER (MSR 0xC0000080)
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1                           ; Set SCE bit (bit 0)
    wrmsr

    ; 2. Configure STAR MSR (0xC0000081)
    ;    Bits 31-47: Kernel Segment Base (0x08 for Kernel CS, 0x10 for Kernel SS)
    ;    Bits 48-63: User Segment Base (0x18 for User CS/SS base selector for sysret)
    mov ecx, 0xC0000081
    rdmsr
    mov edx, 0x001B0008                 ; High dword: User Base = 0x18 | 3 = 0x1B, Kernel Base = 0x08
    wrmsr

    ; 3. Configure LSTAR MSR (0xC0000082): target RIP for 64-bit system calls
    mov ecx, 0xC0000082
    mov rax, syscall_handler_asm
    mov rdx, rax
    shr rdx, 32                         ; High dword in EDX, Low dword in EAX
    wrmsr

    ; 4. Configure SFMASK MSR (0xC0000084): clear flags on system call execution
    ;    Masks out the Interrupt Flag (IF = bit 9) and Trap Flag (TF = bit 8)
    mov ecx, 0xC0000084
    rdmsr
    mov eax, 0x00000300                 ; Mask out IF (0x200) and TF (0x100)
    wrmsr
    ret

; syscall_handler_asm: Fast System Call Entry Point
syscall_handler_asm:
    ; 1. Save the user stack pointer
    mov [rel user_rsp_temp], rsp
    
    ; 2. Load the kernel stack pointer
    mov rsp, [rel kernel_stack_temp]
    
    ; 3. Push context registers onto the kernel stack
    push r11                            ; Save user RFLAGS
    push rcx                            ; Save user RIP
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15
    
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    
    ; 4. Set up arguments and call the Rust dispatcher
    mov rcx, rdx                        ; arg3: RDX
    mov rdx, rsi                        ; arg2: RSI
    mov rsi, rdi                        ; arg1: RDI
    mov rdi, rax                        ; num:  RAX
    
    call syscall_dispatcher             ; Returns result in RAX
    
    ; Check if EAX matches the exit code 0xDEADBEEF (uses EAX to prevent sign issues in imm32)
    cmp eax, 0xDEADBEEF
    je .exit_user_mode
    
    ; 5. Restore the saved registers
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    pop rcx                             ; Restore user RIP
    pop r11                             ; Restore user RFLAGS
    
    ; 6. Restore the user stack pointer
    mov rsp, [rel user_rsp_temp]
    
    ; 7. Return to user mode (using standard o64 sysret for the 64-bit transition)
    o64 sysret

.exit_user_mode:
    ; Discard the pushed context and restore the saved kernel stack pointer
    mov rsp, [rel kernel_stack_temp]
    
    ; Restore kernel's callee-saved registers (pushed in jump_to_user)
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret                                 ; Returns directly to the caller of jump_to_user

; jump_to_user: Lower the privilege level and execute a user mode function
; Registers:
;   RDI: entry point
;   RSI: user stack
jump_to_user:
    ; Save kernel's callee-saved registers before entering User Mode
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15
    
    ; Save the current RSP to kernel_stack_temp for recovery during system calls
    mov [rel kernel_stack_temp], rsp
    
    cli
    
    ; Push stack frame for iretq
    push 0x23                           ; SS (User Data Segment 0x20 | 3 RPL)
    push rsi                            ; RSP (User Stack)
    push 0x202                          ; RFLAGS (Interrupts enabled)
    push 0x2B                           ; CS (User Code Segment 0x28 | 3 RPL)
    push rdi                            ; RIP (Entry Point)
    
    ; Clear general-purpose registers to prevent leaking kernel information
    xor rax, rax
    xor rbx, rbx
    xor rcx, rcx
    xor rdx, rdx
    xor rsi, rsi
    xor rdi, rdi
    xor rbp, rbp
    xor r8, r8
    xor r9, r9
    xor r10, r10
    xor r11, r11
    xor r12, r12
    xor r13, r13
    xor r14, r14
    xor r15, r15
    
    iretq                               ; Pop registers and transition to Ring 3
    
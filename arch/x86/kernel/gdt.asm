; Keira Kernel: GDT (Global Descriptor Table)
;
; Extended 64-bit GDT configured to support User Mode (Ring 3) and TSS:
;   0x00: Null descriptor
;   0x08: 64-bit Kernel Code (DPL 0)
;   0x10: 64-bit Kernel Data (DPL 0)
;   0x18: 64-bit User Data (DPL 3)
;   0x20: 64-bit User Data (DPL 3) (Required by sysret: STAR[63:48]+8)
;   0x28: 64-bit User Code (DPL 3) (Required by sysret: STAR[63:48]+16)
;   0x30: Task State Segment (TSS) Descriptor (16 bytes)

section .data
align 16

global gdt_start
gdt_start:

; Entry 0: Null Descriptor (0x00)
    dq 0x0000000000000000

; Entry 1: 64-bit Kernel Code Segment (0x08)
; Present=1, DPL=0, Code=1, Readable=1, Long=1, Granularity=1
    dw 0xFFFF                           ; Limit [15:0]
    dw 0x0000                           ; Base [15:0]
    db 0x00                             ; Base [23:16]
    db 10011010b                        ; Access: P=1, DPL=00, S=1, E=1, DC=0, RW=1, A=0
    db 10101111b                        ; Flags: G=1, D=0, L=1, AVL=0 | Limit[19:16]=0xF
    db 0x00                             ; Base [31:24]

; Entry 2: 64-bit Kernel Data Segment (0x10)
; Present=1, DPL=0, Data=1, Writable=1, Granularity=1
    dw 0xFFFF                           ; Limit [15:0]
    dw 0x0000                           ; Base [15:0]
    db 0x00                             ; Base [23:16]
    db 10010010b                        ; Access: P=1, DPL=00, S=1, E=0, DC=0, RW=1, A=0
    db 11001111b                        ; Flags: G=1, D=1, L=0, AVL=0 | Limit[19:16]=0xF
    db 0x00                             ; Base [31:24]

; Entry 3: 64-bit User Data Segment (0x18)
; Present=1, DPL=3, Data=1, Writable=1, Granularity=1
    dw 0xFFFF                           ; Limit [15:0]
    dw 0x0000                           ; Base [15:0]
    db 0x00                             ; Base [23:16]
    db 11110010b                        ; Access: P=1, DPL=11, S=1, E=0, DC=0, RW=1, A=0
    db 11001111b                        ; Flags: G=1, D=1, L=0, AVL=0 | Limit[19:16]=0xF
    db 0x00                             ; Base [31:24]

; Entry 4: 64-bit User Data Segment (0x20) (Loaded by sysret as SS)
; Present=1, DPL=3, Data=1, Writable=1, Granularity=1
    dw 0xFFFF                           ; Limit [15:0]
    dw 0x0000                           ; Base [15:0]
    db 0x00                             ; Base [23:16]
    db 11110010b                        ; Access: P=1, DPL=11, S=1, E=0, DC=0, RW=1, A=0
    db 11001111b                        ; Flags: G=1, D=1, L=0, AVL=0 | Limit[19:16]=0xF
    db 0x00                             ; Base [31:24]

; Entry 5: 64-bit User Code Segment (0x28) (Loaded by sysret as CS)
; Present=1, DPL=3, Code=1, Readable=1, Long=1, Granularity=1
    dw 0xFFFF                           ; Limit [15:0]
    dw 0x0000                           ; Base [15:0]
    db 0x00                             ; Base [23:16]
    db 11111010b                        ; Access: P=1, DPL=11, S=1, E=1, DC=0, RW=1, A=0
    db 10101111b                        ; Flags: G=1, D=0, L=1, AVL=0 | Limit[19:16]=0xF
    db 0x00                             ; Base [31:24]

; Entries 6 and 7: 64-bit TSS Descriptor (0x30, 16 bytes)
; This descriptor is populated dynamically by the kernel at runtime.
global tss_descriptor
tss_descriptor:
    dw 0x0000                           ; Limit [15:0]
    dw 0x0000                           ; Base [15:0]
    db 0x00                             ; Base [23:16]
    db 10001001b                        ; Access: P=1, DPL=00, Type=0x9 (64-bit TSS Available)
    db 0x00                             ; Flags and Limit [19:16]
    db 0x00                             ; Base [31:24]
    dd 0x00000000                       ; Base [63:32]
    dd 0x00000000                       ; Reserved

gdt_end:

; GDT Descriptor pointing to the table
global gdt_descriptor
gdt_descriptor:
    dw gdt_end - gdt_start - 1          ; Table size
    dq gdt_start                        ; Table offset

; Function to reload the Global Descriptor Table (GDT)
global reload_gdt
reload_gdt:
    lgdt [rel gdt_descriptor]
    ret

; Function to load the Task State Segment (TSS) selector
global load_tss
load_tss:
    ; TSS selector offset is 0x30
    mov ax, 0x30
    ltr ax
    ret
    
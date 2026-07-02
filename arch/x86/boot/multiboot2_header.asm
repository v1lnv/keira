; Keira Kernel: Multiboot2 Header
;
; This section must appear within the first 32,768 bytes of the kernel binary.
; The linker script places the `.multiboot_header` section before all other
; sections to ensure that GRUB2 can locate and validate it during boot.
;
; Reference: Multiboot2 Specification, Section 3.1
;   https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html

%include "constants.inc"

section .multiboot_header
align 8                                 ; Multiboot2 requires 8-byte alignment

header_start:
    dd MULTIBOOT2_MAGIC                 ; Magic number identifying Multiboot2
    dd MULTIBOOT2_ARCH_I386             ; Architecture selector for i386 protected mode
    dd header_end - header_start        ; Total length of the Multiboot2 header in bytes
    ; Checksum formula: (magic + arch + length + checksum) must equal 0 modulo 2^32
    dd -(MULTIBOOT2_MAGIC + MULTIBOOT2_ARCH_I386 + (header_end - header_start))

    ; End Tag (required): signals the end of the Multiboot2 header structures
    align 8                             ; Every tag structure must be 8-byte aligned
    dw 0                                ; Tag type: 0 (signals the terminator tag)
    dw 0                                ; Tag flags: 0
    dd 8                                ; Tag size: 8 bytes (minimum size for this tag)
header_end:

# Bootstrapping & 64-Bit Transition

This module covers the early phase of the Keira Kernel startup, detailing the transition from a 32-bit Protected Mode bootloader environment to 64-bit Long Mode.

---

## 1. Multiboot2 Header Configuration

The booting process is defined in `arch/x86/boot/multiboot2_header.asm`.
- A magic number (`0xE85250D6`) is declared at the beginning of the header.
- Tag fields specify the architecture target (32-bit i386 Protected Mode, type 0) and the length of the header.
- GRUB uses this magic signature to locate and validate the kernel binary before transferring control.

---

## 2. Early i386 Protected Mode Initialization

Upon bootloader handoff, execution begins in `arch/x86/boot/entry32.asm`:
1. **Interrupt Disable**: Executing `cli` disables hardware interrupts.
2. **Boot Stack Allocation**: ESP is loaded with a statically allocated stack pointer (`boot_stack_top`).
3. **CPU Capability Checks**:
   - **CPUID Check**: Verifies that the CPU supports the `cpuid` command by toggling the ID bit (bit 21) in the `EFLAGS` register.
   - **Long Mode Check**: Queries `cpuid` extended function `0x80000001` to confirm that the processor supports 64-bit Long Mode (checking bit 29 of `EDX`).

---

## 3. Paging Initialization (Page Directories)

Before switching to Long Mode, a basic 4-level page table must be configured in `arch/x86/boot/paging.asm`:
- **Identity Mapping**: PML4 (Page Map Level 4), PDPT (Page Directory Pointer Table), and PD (Page Directory) tables are statically set up in memory.
- The first 1GB of physical memory is identity-mapped (virtual address equals physical address) using large 2MB pages (checking the Page Size bit 7 in the PD entry).
- **CR3 Register**: The address of the PML4 table is written to the `CR3` register.

---

## 4. Activating Long Mode

To transition from 32-bit protected mode to 64-bit long mode, the following steps are performed:
1. **Enable PAE**: Sets the Physical Address Extension bit (bit 5) in the `CR4` register.
2. **Enable Long Mode**: Sets the LME bit (bit 8) in the `IA32_EFER` MSR (Model Specific Register `0xC0000080`) using `rdmsr`/`wrmsr` instructions.
3. **Activate Paging**: Sets the PG bit (bit 31) in the `CR0` register.
4. **Segment Reload**: Performs a 64-bit far jump using the new Global Descriptor Table (GDT) segments.

---

## 5. 64-Bit Entry (`entry64.asm`)

Execution moves into `arch/x86/boot/entry64.asm`:
1. **Segment Registers**: Segment registers (DS, SS, ES, FS, GS) are reloaded with null segment selector offsets (`0x00`).
2. **Kernel Stack Setup**: RSP is loaded with a 64-bit stack pointer.
3. **Main Invocation**: Performs a call jump to the Rust entry function `kernel_main`.

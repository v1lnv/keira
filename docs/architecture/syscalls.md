# System Calls (Privilege Isolation)

This module describes how Keira Kernel implements system call routing and switches CPU execution contexts between kernel space (Ring 0) and user space (Ring 3).

---

## 1. Privilege Rings and Stack Isolation

To protect critical kernel resources, applications are executed in Ring 3 (User Space):
- **Stack Separation**: Each user thread has two stacks:
  - **Ring 3 Stack**: Used for local variables and functions inside the application.
  - **Ring 0 Stack**: Stored in the TSS (Task State Segment) structure. The CPU switches to this stack when entering an interrupt or executing a system call.
- **Segment Selectors**: User space code segment (`0x1B` or `0x23` depending on GDT layout) and data segment (`0x23`) are reloaded when returning to Ring 3.

---

## 2. Enabling the x86_64 Syscall Interface

Keira configures the hardware-level `syscall`/`sysret` mechanism during boot:
1. **Enable SCE (System Call Extension)**: Sets the SCE bit (bit 0) in the `IA32_EFER` MSR register.
2. **STAR MSR (`0xC0000081`)**: Specifies the segments used for transitions:
   - Bits [47:32] set the kernel code/data segment base selector (`0x08`).
   - Bits [63:48] set the user code/data segment base selector (`0x18`).
3. **LSTAR MSR (`0xC0000082`)**: Programmed with the 64-bit memory address of the assembly syscall entry handler (`syscall_entry`).
4. **SFMASK MSR (`0xC0000084`)**: Configured with a bitmask of flags to clear when entering kernel mode (usually disabling interrupts by masking `IF`).

---

## 3. Syscall Handler Execution Flow

When a Ring 3 application executes the `syscall` instruction:
1. The CPU saves the return address in `RCX` and the RFLAGS register in `R11`.
2. The CPU switches to Ring 0 code segment, clears flags defined in SFMASK, and jumps to `syscall_entry`.
3. `syscall_entry` saves the user RSP and switches to the thread's Ring 0 stack.
4. Preserves caller registers by pushing them onto the stack.
5. Invokes the central Rust router `syscall_handler` with the syscall number in `RAX` and arguments in `RDI`, `RSI`, `RDX`, `R10`, `R8`, `R9`.
6. Upon completion, the return value is loaded into `RAX`.
7. User registers are restored, the stack pointer is swapped back, and the `sysretq` instruction is called to return to Ring 3.

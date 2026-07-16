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
5. Invokes the central Rust router `syscall_dispatcher` with the syscall number in `RAX` and arguments in `RDI`, `RSI`, `RDX`.
6. Upon completion, the return value is loaded into `RAX`.
7. User registers are restored, the stack pointer is swapped back, and the `sysretq` instruction is called to return to Ring 3.

---

## 4. Supported System Calls List

The kernel exposes the following system calls to Ring 3 applications:

| ID | Name | Signature | Description |
|---|---|---|---|
| **1** | `sys_print_char` | `void sys_print_char(char c)` | Prints a single character to the screen buffer. |
| **2** | `sys_exit` | `void sys_exit(void)` | Exits the user program context and jumps back to Ring 0 kernel shell. |
| **3** | `sys_sleep` | `void sys_sleep(unsigned long ms)` | Pauses user thread execution for a duration in milliseconds. |
| **4** | `sys_uptime` | `unsigned long sys_uptime(void)` | Returns system uptime since boot in milliseconds. |
| **5** | `sys_exec` | `int sys_exec(const char *filename)` | Loads a dynamic ELF executable from disk and executes it. |
| **6** | `sys_open` | `int sys_open(const char *path, int write_mode)` | Opens a file at the given path. Returns fd index or -1 on error. |
| **7** | `sys_read` | `int sys_read(int fd, void *buf, int len)` | Reads up to `len` bytes from the open file descriptor into `buf`. Returns bytes read. |
| **8** | `sys_write` | `int sys_write(int fd, const void *buf, int len)` | Writes up to `len` bytes from `buf` into the open file descriptor. Returns bytes written. |
| **9** | `sys_close` | `int sys_close(int fd)` | Closes the file descriptor. Returns 0 on success, or -1 on error. |
| **10** | `sys_seek` | `int sys_seek(int fd, unsigned long offset)` | Seeks to a specific offset pointer in the open file descriptor. Returns 0. |

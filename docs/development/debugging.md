# Debugging with GDB

This module provides instructions on setting up remote debug sessions to trace Keira Kernel execution line-by-line using QEMU and GDB.

---

## 1. QEMU GDB Server Launch

To debug the kernel, launch QEMU with a GDB socket enabled:
```bash
make debug
```
This commands flags QEMU to:
- Suspend the virtual CPU immediately on the first boot instruction (`-S`).
- Start a GDB stub server listening on TCP port `1234` (`-gdb tcp::1234` or `-s`).

---

## 2. Connecting the GDB Client

In a separate terminal window, launch GDB and connect to the running QEMU emulator instance:

```bash
gdb build/keira.bin
```

Within the GDB prompt, execute the connection command:
```gdb
(gdb) target remote :1234
```

This registers the symbol table loaded from `keira.bin` and pauses execution at the early BIOS/multiboot entry instructions.

---

## 3. Essential Debugging Commands

Here are the most useful commands to debug the kernel:

- **Setting breakpoints**:
  ```gdb
  (gdb) break kernel_main
  ```
  This halts execution exactly when entering the Rust `kernel_main` entry point.
- **Continuing execution**:
  ```gdb
  (gdb) continue
  ```
  Resumes execution until the next breakpoint or exception.
- **Step-by-step trace**:
  - `step` (or `s`): Steps into functions at the source code line level.
  - `next` (or `n`): Steps over function calls.
  - `stepi` (or `si`): Steps by a single assembly instruction.
- **Inspecting memory and registers**:
  - `info registers`: Prints values of all CPU registers (RAX, RBX, RIP, RSP, etc.).
  - `print <variable>`: Prints the active value of a variable.
  - `x/<count><format><size> <address>`: Examines raw memory. For example, `x/16xb 0xB8000` prints the first 16 bytes of the VGA screen memory buffer.

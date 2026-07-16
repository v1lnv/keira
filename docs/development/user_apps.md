# Writing User-Space Programs (Ring 3)

This guide walks you through the process of writing, compiling, and executing standalone **Ring 3 User-Space Applications** on the Keira Operating System.

---

## 1. Overview of the User-Space Environment

User-space applications in Keira are compiled as statically linked 64-bit ELF executables. They execute at Privilege Level 3 (Ring 3), isolated from Ring 0 kernel space.
- **Entry Point**: The standard entry point is `void _start(void)`.
- **System Library (`libkeira`)**: Applications link against the static library in `user/lib/` which provides:
  - System call wrappers (`sys_open`, `sys_write`, `sys_sbrk`, `sys_spawn`, etc.)
  - Standard output (`printf` wrapping `sys_print_char`)
  - String utilities (`strlen`, `strcmp`, `memcpy`, etc.)
  - Memory allocator (`malloc` and `free` wrapping `sys_sbrk`)

---

## 2. Step 1: Create the Application Source

Create a new directory for your app under `user/apps/` (e.g., `user/apps/uptime/`):
Create `user/apps/uptime/main.c`:

```c
#include "../../lib/include/stdio.h"
#include "../../lib/include/syscall.h"

void _start(void) {
    unsigned long ms = sys_uptime();
    printf("Keira System Uptime: %d ms\n", (int)ms);
    
    // Ring 3 executables must terminate using sys_exit()
    sys_exit();
}
```

> [!IMPORTANT]
> **No implicit return**: Because user-space programs are loaded directly without a C runtime wrapper (`crt0`), returning from `_start` using `ret` or standard function exit will pop a null value from the unmapped stack top and crash. You **MUST** terminate the execution using `sys_exit()`.

---

## 3. Step 2: Register the Application in the Makefile

Open the root [Makefile](file:///home/v1lnv/Documents/Projects/keira/Makefile).

### A. Add the Build Target
Under the `user:` target, add your new binary. Declare the target rules using `USER_CC_FLAGS` and `USER_LIB_SRCS`:

```makefile
user: build/user_test.elf build/hello.elf build/kcc.elf build/uptime.elf

build/uptime.elf: user/apps/uptime/main.c $(USER_LIB_SRCS) user/linker.ld | dirs
	@$(LOG_INFO) "Building user space program: uptime (uptime.elf)..."
	@$(CC) $(USER_CC_FLAGS) user/apps/uptime/main.c $(USER_LIB_SRCS) -o build/uptime.elf
```

### B. Copy to FAT16 Disk Image
Append your binary to the copy instructions in the `$(DISK_IMG)` target so it gets written onto the filesystem:

```makefile
	@mcopy -o -i $(DISK_IMG) build/user_test.elf ::/apps/bin/user_test.elf
	@mcopy -o -i $(DISK_IMG) build/hello.elf ::/apps/bin/hello.elf
	@mcopy -o -i $(DISK_IMG) build/kcc.elf ::/apps/bin/kcc.elf
	@mcopy -o -i $(DISK_IMG) build/uptime.elf ::/apps/bin/uptime.elf
```

### C. Copy to RAM Disk (Initrd)
Append your binary to the `initrd` build target:

```makefile
	@cp build/user_test.elf $(BUILD_DIR)/initrd_root/apps/bin/user_test.elf
	@cp build/hello.elf $(BUILD_DIR)/initrd_root/apps/bin/hello.elf
	@cp build/kcc.elf $(BUILD_DIR)/initrd_root/apps/bin/kcc.elf
	@cp build/uptime.elf $(BUILD_DIR)/initrd_root/apps/bin/uptime.elf
```

---

## 4. Compile and Run

Rebuild the system to compile your new binary and repackage the ISO/disk:
```bash
make clean && make run
```

Once the Keira Shell is ready, run your application:
```bash
run uptime
```
This will load and execute `/apps/bin/uptime.elf` in Ring 3 and display the system uptime.

---

## 5. Architectural Considerations

### Disabling Vectorized Instructions (SSE/AVX/MMX)
Because the Keira Kernel is a minimalist freestanding kernel, it does not enable SSE/AVX CPU extensions in `CR4` nor save/restore the large SSE registers (`XMM`/`YMM`) on thread context switches. 
Therefore, GCC optimization flags (`-O2`) must be constrained to prevent the compiler from generating vectorized loops (e.g. inside `memset`/`memcpy`). 

Ensure that `USER_CC_FLAGS` always includes:
`-mno-sse -mno-sse2 -mno-mmx -mno-sse3 -mno-ssse3 -mno-sse4.1 -mno-sse4.2 -mno-avx -mno-avx2`
to guarantee that all compiled user space binaries rely purely on standard x86_64 integer registers.

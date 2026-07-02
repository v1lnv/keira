# Keira Kernel: Master Build System
#
# Orchestrates the tri-language build pipeline:
#   1. NASM:  Assembly (.asm)   -> Object files (.o)
#   2. GCC:   C source (.c)      -> Object files (.o)
#   3. Cargo: Rust source (.rs)  -> Static library (.a)
#   4. LD:    Link all above     -> kernel.bin (ELF64)
#   5. GRUB:  Package            -> keira.iso (bootable)
#
# Usage:
#   make          : Build everything (kernel.bin + keira.iso)
#   make run      : Build and launch in QEMU with serial output
#   make clean    : Remove all build artifacts
#   make debug    : Build and launch in QEMU with GDB server
#
# Prerequisites:
#   nasm, gcc, rustup (nightly), ld, grub-mkrescue, xorriso, qemu-system-x86_64

# Configuration

# Build directories
BUILD_DIR     := build
OBJ_DIR       := $(BUILD_DIR)/obj
ISO_DIR       := $(BUILD_DIR)/isofiles
DISK_IMG      := $(BUILD_DIR)/disk.img

# Project metadata
KERNEL_NAME   := keira
KERNEL_BIN    := $(BUILD_DIR)/$(KERNEL_NAME).bin
# Professional dated ISO naming
DATE_SUFFIX   := $(shell date +%Y-%m-%d)
KERNEL_ISO    := $(BUILD_DIR)/$(KERNEL_NAME)-$(DATE_SUFFIX).iso


# Toolchain
ASM           := nasm
CC            := gcc
LD            := ld
# Cargo automatically uses the nightly toolchain defined in rust-toolchain.toml
CARGO         := cargo

# NASM flags: ELF64 output, include path for constants.inc
ASM_FLAGS     := -f elf64 -I arch/x86/include/asm/

# GCC flags for freestanding kernel C code:
#   -ffreestanding        No standard library, no startup files
#   -mno-red-zone         Disable red zone (unsafe with interrupts)
#   -mno-mmx/sse/...      Disable SIMD/AVX (matches features in targets/x86_64-keira-none.json)
#   -msoft-float          Use software floating point (matches targets/x86_64-keira-none.json)
#   -mcmodel=large        Support addresses above 2GB
#   -fno-stack-protector  No stack canaries (no __stack_chk_fail)
#   -nostdlib             Don't link standard libraries
#   -Wall -Wextra         Enable comprehensive warnings
CC_FLAGS      := -ffreestanding \
	         -mno-red-zone \
	         -mno-mmx \
	         -mno-sse \
	         -mno-sse2 \
	         -mno-sse3 \
	         -mno-ssse3 \
	         -mno-sse4.1 \
	         -mno-sse4.2 \
	         -mno-avx \
	         -mno-avx2 \
	         -msoft-float \
	         -mcmodel=large \
	         -fno-stack-protector \
	         -fno-pic \
	         -nostdlib \
	         -m64 \
	         -I arch/x86/include \
	         -Wall -Wextra \
	         -O2

# Linker flags
LD_FLAGS      := -n \
	         -T arch/x86/linker.ld \
	         --gc-sections \
	         --no-warn-rwx-segments

# Rust target and build mode
RUST_TARGET   := targets/x86_64-keira-none.json
RUST_MODE     := release
RUST_LIB      := target/x86_64-keira-none/$(RUST_MODE)/libkeira_kernel.a

# QEMU flags
QEMU          := qemu-system-x86_64
QEMU_FLAGS    := -cdrom $(KERNEL_ISO) \
	         -drive file=$(DISK_IMG),format=raw,index=0,media=disk \
	         -boot d \
	         -serial stdio \
	         -no-shutdown \
	         -m 128M

# Source Files

# Assembly sources (order matters: multiboot header must link first)
ASM_SRCS      := arch/x86/boot/multiboot2_header.asm \
	         arch/x86/boot/entry32.asm \
	         arch/x86/boot/entry64.asm \
	         arch/x86/kernel/gdt.asm \
	         arch/x86/kernel/paging.asm \
	         arch/x86/kernel/idt.asm \
	         arch/x86/kernel/isr.asm \
	         arch/x86/kernel/syscall.asm

# C sources
C_SRCS        := drivers/serial/serial.c \
	         drivers/vga/vga.c \
	         arch/x86/kernel/idt.c \
	         arch/x86/kernel/pic.c \
	         arch/x86/kernel/pit.c \
	         drivers/keyboard/keyboard.c \
	         drivers/mouse/mouse.c \
	         drivers/rtc/rtc.c \
	         mm/heap/heap.c \
	         arch/x86/kernel/hw_init.c

# Generate object file paths from sources
ASM_OBJS      := $(patsubst %.asm,$(OBJ_DIR)/%.asm.o,$(ASM_SRCS))
C_OBJS        := $(patsubst %.c,$(OBJ_DIR)/%.c.o,$(C_SRCS))

# All object files for linking
ALL_OBJS      := $(ASM_OBJS) $(C_OBJS)

# Targets

.PHONY: all run debug clean rust iso dirs format lint

# Default target: build the bootable ISO
all: $(KERNEL_ISO) $(DISK_IMG)

build/user_test.elf: user/apps/init/main.c user/lib/syscall.c user/lib/string.c user/lib/stdio.c user/linker.ld | dirs
	@echo "[USER]  Compiling user space program and modular library..."
	@$(CC) -ffreestanding -nostdlib -fno-stack-protector -m64 -O2 -Iuser/lib -T user/linker.ld \
	        user/apps/init/main.c user/lib/syscall.c user/lib/string.c user/lib/stdio.c \
	        -o build/user_test.elf -Wl,--no-warn-rwx-segments -static -no-pie

$(DISK_IMG): build/user_test.elf
	@rm -f $(DISK_IMG)
	@echo "[DISK]  Creating 32MB FAT16 disk image..."
	@dd if=/dev/zero of=$(DISK_IMG) bs=1M count=32 2>/dev/null
	@mkfs.fat -F 16 $(DISK_IMG) >/dev/null
	@echo "[DISK]  Creating nested Keira directory structure..."
	@mmd -i $(DISK_IMG) ::/system ::/system/bin ::/system/drivers ::/apps ::/apps/bin ::/apps/games ::/config ::/config/boot ::/config/theme ::/users ::/users/admin ::/users/default ::/users/guest ::/temp ::/data ::/data/log ::/data/save 2>/dev/null || true
	@echo "[DISK]  Populating directories with command binaries..."
	@mkdir -p $(BUILD_DIR)/system_bin
	@for cmd in guide login drives use ramdisk system cpu runtime time memory \
	            devices wait initrd wipe reset run write tasks demo disk list \
	            go script view create folder delete edit say copy help history \
	            move theme please; do \
	    printf '#!/system/bin\n# Keira built-in command: %s\n# Type: kernel-mode binary\n# Path: /system/bin/%s\n' "$$cmd" "$$cmd" > $(BUILD_DIR)/system_bin/$$cmd; \
	    mcopy -o -i $(DISK_IMG) $(BUILD_DIR)/system_bin/$$cmd ::/system/bin/$$cmd; \
	done
	@echo "[DISK]  Copying driver files and system config..."
	@mkdir -p $(BUILD_DIR)/drivers
	@echo "Keira Serial Port Driver (COM1, 115200bps, 8N1)" > $(BUILD_DIR)/drivers/serial.sys
	@echo "Keira VGA Text Console Driver (80x25 characters, color support)" > $(BUILD_DIR)/drivers/vga.sys
	@echo "Keira PS/2 Keyboard Driver (US QWERTY layout)" > $(BUILD_DIR)/drivers/keyboard.sys
	@echo "Keira PS/2 Mouse Driver (basic coordinate tracking)" > $(BUILD_DIR)/drivers/mouse.sys
	@echo "Keira Real-Time Clock Driver (CMOS direct port communication)" > $(BUILD_DIR)/drivers/rtc.sys
	@echo "Keira IDE Storage Controller Driver (LBA28 read/write)" > $(BUILD_DIR)/drivers/ide.sys
	@for driver in serial.sys vga.sys keyboard.sys mouse.sys rtc.sys ide.sys; do \
	    mcopy -o -i $(DISK_IMG) $(BUILD_DIR)/drivers/$$driver ::/system/drivers/$$driver; \
	done
	@echo "color_scheme=arch_retro\nprompt_symbol=»\ncursor=block" > $(BUILD_DIR)/default.cfg
	@mcopy -o -i $(DISK_IMG) $(BUILD_DIR)/default.cfg ::/config/theme/default.cfg
	@echo "[DISK]  Copying binaries and configuration files..."
	@mcopy -o -i $(DISK_IMG) build/user_test.elf ::/apps/bin/user_test.elf

$(BUILD_DIR)/initrd.tar: build/user_test.elf
	@echo "[INITRD] Building RAM Disk (Initrd)..."
	@mkdir -p $(BUILD_DIR)/initrd_root/system/bin
	@mkdir -p $(BUILD_DIR)/initrd_root/system/drivers
	@mkdir -p $(BUILD_DIR)/initrd_root/apps/bin
	@mkdir -p $(BUILD_DIR)/initrd_root/config/boot
	@mkdir -p $(BUILD_DIR)/initrd_root/config/theme
	@mkdir -p $(BUILD_DIR)/initrd_root/users/admin
	@mkdir -p $(BUILD_DIR)/initrd_root/users/default
	@mkdir -p $(BUILD_DIR)/initrd_root/users/guest
	@mkdir -p $(BUILD_DIR)/initrd_root/temp
	@mkdir -p $(BUILD_DIR)/initrd_root/data
	@for cmd in guide login drives use ramdisk system cpu runtime time memory \
	            devices wait initrd wipe reset run write tasks demo disk list \
	            go script view create folder delete edit say copy help history \
	            move theme please; do \
	    printf '#!/system/bin\n# Keira built-in command: %s\n# Type: kernel-mode binary\n# Path: /system/bin/%s\n' "$$cmd" "$$cmd" > $(BUILD_DIR)/initrd_root/system/bin/$$cmd; \
	done
	@echo "Keira Serial Port Driver (COM1, 115200bps, 8N1)" > $(BUILD_DIR)/initrd_root/system/drivers/serial.sys
	@echo "Keira VGA Text Console Driver (80x25 characters, color support)" > $(BUILD_DIR)/initrd_root/system/drivers/vga.sys
	@echo "Keira PS/2 Keyboard Driver (US QWERTY layout)" > $(BUILD_DIR)/initrd_root/system/drivers/keyboard.sys
	@echo "Keira PS/2 Mouse Driver (basic coordinate tracking)" > $(BUILD_DIR)/initrd_root/system/drivers/mouse.sys
	@echo "Keira Real-Time Clock Driver (CMOS direct port communication)" > $(BUILD_DIR)/initrd_root/system/drivers/rtc.sys
	@echo "Keira IDE Storage Controller Driver (LBA28 read/write)" > $(BUILD_DIR)/initrd_root/system/drivers/ide.sys
	@echo "color_scheme=arch_retro\nprompt_symbol=»\ncursor=block" > $(BUILD_DIR)/initrd_root/config/theme/default.cfg
	@cp build/user_test.elf $(BUILD_DIR)/initrd_root/apps/bin/user_test.elf
	@cd $(BUILD_DIR)/initrd_root && tar -cf ../initrd.tar *

# Build the bootable ISO
$(KERNEL_ISO): $(KERNEL_BIN) $(BUILD_DIR)/initrd.tar | dirs
	@echo "[ISO]   Creating bootable ISO..."
	@mkdir -p $(ISO_DIR)/boot/grub
	@cp $(KERNEL_BIN) $(ISO_DIR)/boot/$(KERNEL_NAME).bin
	@cp $(BUILD_DIR)/initrd.tar $(ISO_DIR)/boot/initrd.tar
	@echo 'set timeout=0' > $(ISO_DIR)/boot/grub/grub.cfg
	@echo 'set default=0' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo '' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo 'menuentry "Keira" {' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo '	multiboot2 /boot/keira.bin' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo '	module2 /boot/initrd.tar initrd' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo '	boot' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo '}' >> $(ISO_DIR)/boot/grub/grub.cfg
	@grub-mkrescue -o $(KERNEL_ISO) $(ISO_DIR) 2>/dev/null
	@echo "[DONE]  $(KERNEL_ISO) ready"

# Link all objects into final kernel ELF
$(KERNEL_BIN): $(ALL_OBJS) $(RUST_LIB) arch/x86/linker.ld | dirs
	@echo "[LD]    Linking kernel..."
	@$(LD) $(LD_FLAGS) -o $(KERNEL_BIN) $(ALL_OBJS) $(RUST_LIB)
	@echo "[DONE]  $(KERNEL_BIN) ready"

# Build Rust static library
$(RUST_LIB): rust
rust:
	@echo "[CARGO] Building Rust kernel ($(RUST_MODE))..."
	@$(CARGO) -Zjson-target-spec -Zbuild-std=core build --target $(RUST_TARGET) --$(RUST_MODE) 2>&1 | sed 's/^/        /'

# Assemble NASM sources
$(OBJ_DIR)/%.asm.o: %.asm | dirs
	@echo "[ASM]   $<"
	@mkdir -p $(dir $@)
	@$(ASM) $(ASM_FLAGS) -o $@ $<

# Compile C sources
$(OBJ_DIR)/%.c.o: %.c | dirs
	@echo "[CC]    $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CC_FLAGS) -I drivers -I arch/x86/kernel -c -o $@ $<

# Create build directories
dirs:
	@mkdir -p $(BUILD_DIR) $(OBJ_DIR)

# Run Targets

# Run in QEMU with serial output to terminal
run: all
	@echo "[QEMU]  Launching Keira..."
	@$(QEMU) $(QEMU_FLAGS)

# Run in QEMU with GDB server for debugging
debug: all
	@echo "[QEMU]  Launching Keira (debug mode, waiting for GDB on :1234)..."
	@$(QEMU) $(QEMU_FLAGS) -S -s

# Clean

clean:
	@echo "[CLEAN] Removing build artifacts..."
	@rm -rf $(BUILD_DIR)
	@$(CARGO) clean 2>/dev/null || true
	@echo "[DONE]  Clean complete"

# Formatting and Linting

format:
	@echo "[FMT]   Formatting Rust code..."
	@$(CARGO) fmt --all
	@echo "[FMT]   Formatting C code..."
	@find . -type f \( -name "*.c" -o -name "*.h" \) -exec clang-format -i {} +
	@echo "[DONE]  Formatting complete"

lint:
	@echo "[LINT]  Linting C code..."
	@find . -type f \( -name "*.c" -o -name "*.h" \) -exec clang-tidy --quiet {} -- -I drivers -I arch/x86/include -I arch/x86/kernel -ffreestanding -m64 \;
	@echo "[DONE]  Linting complete"
	
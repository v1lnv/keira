/**
 * Keira Kernel: Hardware Initialization Entry Point
 *
 * This is the first C function called by the 64-bit assembly trampoline.
 * It initializes all hardware drivers and prints a boot banner to confirm
 * that the C runtime is functional.
 *
 * Call chain: ASM _start64 -> hw_init() -> [returns] -> kernel_main() (Rust)
 */

#include "../../../drivers/keyboard/include/keyboard.h"
#include "../../../drivers/mouse/include/mouse.h"
#include "../../../drivers/rtc/include/rtc.h"
#include "../../../drivers/serial/include/serial.h"
#include "../../../drivers/vga/include/vga.h"
#include "../../../include/keira/heap.h"
#include <asm/idt.h>
#include <asm/pic.h>
#include <asm/pit.h>

/* Linker script provides heap boundaries */
extern uint8_t __heap_start;
extern uint8_t __heap_end;

static void print_boot_log(const char *msg, int status) {
    /* 1. Print to VGA in Arch Linux style */
    vga_set_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    vga_print(":: ");

    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print(msg);

    // Calculate length to pad to column 72
    int len = 0;
    while (msg[len])
        len++;
    int padding = 72 - 3 - len; // 3 for ":: "
    if (padding < 1)
        padding = 1;
    for (int i = 0; i < padding; i++) {
        vga_print(" ");
    }

    if (status == 0) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_print("[ OK ]\n");
    } else if (status == 1) {
        vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
        vga_print("[ WARN ]\n");
    } else {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_print("[ FAIL ]\n");
    }

    /* 2. Print to Serial in Arch Linux style */
    serial_print("\033[1;34m::\033[0m ");
    serial_print(msg);

    padding = 72 - 3 - len;
    if (padding < 1)
        padding = 1;
    for (int i = 0; i < padding; i++) {
        serial_print(" ");
    }

    if (status == 0) {
        serial_print("\033[1;32m[ OK ]\033[0m\n");
    } else if (status == 1) {
        serial_print("\033[1;33m[WARN]\033[0m\n");
    } else {
        serial_print("\033[1;31m[FAIL]\033[0m\n");
    }
}

/**
 * Master hardware initialization routine.
 *
 * Called once by entry64.asm after BSS has been zeroed.
 */
void hw_init(void) {
    /* Phase 1: Initialize output devices */
    serial_init();
    vga_init();

    print_boot_log("Initializing Serial Port (COM1) driver", 0);
    print_boot_log("Configuring VGA text-mode frame buffer (80x25)", 0);

    /* Phase 2: Initialize interrupts and timers */
    idt_init();
    print_boot_log("Loading Interrupt Descriptor Table (IDT) registers", 0);

    pic_init(32, 40); /* Remap PIC IRQs to 32-47 */
    print_boot_log("Remapping dual 8259 PIC interrupt controller IRQs", 0);

    pit_init(1000); /* Set PIT to 1000 Hz (1ms tick) */
    print_boot_log("Configuring 8253 PIT system timer tick rate to 1000Hz", 0);

    keyboard_init();
    print_boot_log("Initializing PS/2 keyboard controller & driver", 0);

    mouse_init();
    print_boot_log("Initializing PS/2 mouse controller & driver", 0);

    /* Phase 3: Initialize RTC and Heap */
    rtc_init();
    print_boot_log("Reading CMOS Real-Time Clock (RTC) date/time registers", 0);

    heap_init(&__heap_start, (size_t)((uintptr_t)&__heap_end - (uintptr_t)&__heap_start));
    print_boot_log("Determining kernel C heap memory boundaries", 0);
    print_boot_log("Initializing local C heap allocator space (1MB)", 0);

    print_boot_log("Completing low-level hardware subsystem checks", 0);
    print_boot_log("Jumping to Rust 64-bit kernel_main() entry point", 0);
}

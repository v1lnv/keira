#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

/**
 * Keira Kernel: PS/2 Keyboard Driver
 *
 * Handles IRQ1 (Keyboard) interrupts, reads scan codes from port 0x60,
 * translates them to ASCII, and forwards them to the Rust shell.
 */

/**
 * Initialize the keyboard driver.
 */
void keyboard_init(void);

/**
 * The C interrupt handler for IRQ1 (Keyboard).
 * Called by the assembly stub `isr33` in isr.asm.
 */
void keyboard_handler(void);

#endif /* KEYBOARD_H */

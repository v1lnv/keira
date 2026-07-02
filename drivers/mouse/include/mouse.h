#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>

/**
 * Keira Kernel: PS/2 Mouse Driver
 *
 * Initializes the 8042 PS/2 controller auxiliary device and handles IRQ12.
 * Processes 3-byte packets to track mouse movement.
 */

/**
 * Initialize the PS/2 mouse.
 */
void mouse_init(void);

/**
 * The C interrupt handler for IRQ12 (Mouse).
 * Called by the assembly stub `isr44` in isr.asm.
 */
void mouse_handler(void);

#endif /* MOUSE_H */

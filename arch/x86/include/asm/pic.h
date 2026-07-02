#ifndef PIC_H
#define PIC_H

#include <stdint.h>

/**
 * Keira Kernel: Programmable Interrupt Controller (8259 PIC) Driver
 *
 * The PIC maps hardware interrupts (IRQs) to CPU interrupt vectors.
 * By default, they are mapped to 0-15, which conflicts with CPU exceptions.
 * We must remap them to 32-47.
 */

/**
 * Initialize the PIC and remap IRQs.
 *
 * @param offset1 Vector offset for master PIC (usually 32)
 * @param offset2 Vector offset for slave PIC (usually 40)
 */
void pic_init(int offset1, int offset2);

/**
 * Send End of Interrupt (EOI) to the PIC.
 *
 * @param irq The IRQ number (0-15) that was handled.
 */
void pic_eoi(unsigned char irq);

/**
 * Mask an IRQ (disable it).
 */
void pic_set_mask(unsigned char irqline);

/**
 * Unmask an IRQ (enable it).
 */
void pic_clear_mask(unsigned char irqline);

#endif /* PIC_H */

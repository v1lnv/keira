#ifndef PIT_H
#define PIT_H

#include <stdint.h>

/**
 * Keira Kernel: Programmable Interval Timer (PIT) Driver
 */

/**
 * Initialize the PIT to fire an interrupt at the specified frequency (Hz).
 * A frequency of 1000 Hz means 1 interrupt per millisecond.
 */
void pit_init(uint32_t frequency);

/**
 * Handle the PIT interrupt (IRQ0).
 * This is called by the ISR stub.
 */
void pit_handler(void);

/**
 * Get the total number of milliseconds since boot.
 *
 * @return Uptime in milliseconds
 */
uint64_t get_uptime_ms(void);

#endif /* PIT_H */

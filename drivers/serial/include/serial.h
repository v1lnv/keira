/**
 * Keira Kernel: Serial Port Driver (Header)
 *
 * Public interface for the COM1 UART 16550A serial port driver.
 * Provides character and string output for boot diagnostics and debugging.
 *
 * Usage:
 *   serial_init();                    // Call once during hw_init()
 *   serial_print("Hello, Keira!\n");  // Output to COM1
 */

#ifndef KEIRA_SERIAL_H
#define KEIRA_SERIAL_H

#include <stdint.h>

/**
 * Initialize COM1 UART to 38400 baud, 8N1.
 *
 * Must be called before any serial output. Configures:
 *   - Baud rate: 38400 (divisor = 3)
 *   - Data bits: 8
 *   - Stop bits: 1
 *   - Parity:    None
 *   - FIFO:      Enabled, 14-byte threshold
 */
void serial_init(void);

/**
 * Write a single byte to COM1.
 *
 * Blocks until the UART transmit buffer is ready, then sends the byte.
 * For newlines ('\n'), automatically prepends carriage return ('\r')
 * to ensure correct line breaks on serial terminals.
 *
 * @param c The character to transmit.
 */
void serial_putchar(char c);

/**
 * Write a null-terminated string to COM1.
 *
 * Iterates through each character and calls serial_putchar().
 *
 * @param str Pointer to null-terminated string.
 */
void serial_print(const char *str);

#endif /* KEIRA_SERIAL_H */

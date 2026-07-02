/**
 * Keira Kernel: Serial Port Driver (Implementation)
 *
 * COM1 UART 16550A driver for kernel debug output.
 *
 * The 16550A UART has 8 registers accessed via I/O ports at offsets from
 * the base port (COM1 = 0x3F8):
 *
 *   Offset | DLAB=0 Read     | DLAB=0 Write       | DLAB=1
 *   -------|-----------------|--------------------|-----------------
 *   +0     | Receive Buffer  | Transmit Holding   | Divisor LSB
 *   +1     | Interrupt Enable| Interrupt Enable   | Divisor MSB
 *   +2     | Interrupt ID    | FIFO Control       | :
 *   +3     | Line Control    | Line Control       | :
 *   +4     | Modem Control   | Modem Control      | :
 *   +5     | Line Status     | :                  | :
 *   +6     | Modem Status    | :                  | :
 *   +7     | Scratch         | Scratch            | :
 *
 * Reference: https://wiki.osdev.org/Serial_Ports
 */

#include "include/serial.h"
#include "../../arch/x86/include/asm/io.h"
#include "regs.h"

/* Register offsets and bitmasks are included from regs.h */

/**
 * Check if the transmit buffer is empty.
 *
 * Reads the Line Status Register (LSR) and checks the TX Empty bit.
 * Returns non-zero if the UART is ready to accept a new byte.
 */
static int serial_is_tx_ready(void) {
    return inb(COM1_LINE_STATUS) & LSR_TX_EMPTY;
}

/* Public API Implementation */

void serial_init(void) {
    outb(COM1_INT_ENABLE, 0x00);  /* Disable all UART interrupts            */
    outb(COM1_LINE_CTRL, 0x80);   /* Enable DLAB to set baud rate divisor   */
    outb(COM1_DIVISOR_LSB, 0x03); /* Divisor = 3 → 38400 baud (lo byte)    */
    outb(COM1_DIVISOR_MSB, 0x00); /*                           (hi byte)   */
    outb(COM1_LINE_CTRL, 0x03);   /* 8 data bits, no parity, 1 stop bit    */
    outb(COM1_FIFO_CTRL, 0xC7);   /* Enable FIFO, clear buffers, 14-byte   */
    outb(COM1_MODEM_CTRL, 0x0B);  /* Enable IRQs, set RTS and DTR          */
}

void serial_putchar(char c) {
    /* Prepend carriage return before newline for serial terminal compat */
    if (c == '\n') {
        while (!serial_is_tx_ready()) { /* spin */
        }
        outb(COM1_DATA, '\r');
    }

    while (!serial_is_tx_ready()) { /* spin */
    }
    outb(COM1_DATA, (uint8_t)c);
}

void serial_print(const char *str) {
    while (*str) {
        serial_putchar(*str);
        str++;
    }
}

#ifndef SERIAL_REGS_H
#define SERIAL_REGS_H

/* Serial Port COM1 Base Address */
#define COM1_BASE 0x3F8

/* Serial Port Register Offsets */
#define COM1_DATA (COM1_BASE + 0)        /* TX/RX data register (DLAB=0) */
#define COM1_INT_ENABLE (COM1_BASE + 1)  /* Interrupt enable register (DLAB=0) */
#define COM1_FIFO_CTRL (COM1_BASE + 2)   /* FIFO control register */
#define COM1_LINE_CTRL (COM1_BASE + 3)   /* Line control register */
#define COM1_MODEM_CTRL (COM1_BASE + 4)  /* Modem control register */
#define COM1_LINE_STATUS (COM1_BASE + 5) /* Line status register */

#define COM1_DIVISOR_LSB (COM1_BASE + 0) /* Divisor latch LSB (DLAB=1) */
#define COM1_DIVISOR_MSB (COM1_BASE + 1) /* Divisor latch MSB (DLAB=1) */

/* Line Status Register bits */
#define LSR_TX_EMPTY 0x20 /* Transmit holding reg empty */

#endif /* SERIAL_REGS_H */

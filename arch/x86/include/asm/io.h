#ifndef IO_H
#define IO_H

#include <stdint.h>

/**
 * Keira Kernel: Hardware I/O Port Abstraction
 *
 * Provides inline assembly wrappers for x86 port I/O instructions.
 */

/**
 * Write a byte to the specified I/O port.
 *
 * @param port  The 16-bit I/O port address
 * @param value The 8-bit value to write
 */
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * Read a byte from the specified I/O port.
 *
 * @param port The 16-bit I/O port address
 * @return     The 8-bit value read from the port
 */
static inline uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

/**
 * Write a 32-bit dword to the specified I/O port.
 */
static inline void outl(uint16_t port, uint32_t value) {
    __asm__ volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * Read a 32-bit dword from the specified I/O port.
 */
static inline uint32_t inl(uint16_t port) {
    uint32_t result;
    __asm__ volatile("inl %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

/**
 * Wait for a very short I/O cycle (used for PIC).
 *
 * Writes to an unused port (0x80) to force the CPU to wait for the bus.
 */
static inline void io_wait(void) {
    outb(0x80, 0);
}

#endif /* IO_H */

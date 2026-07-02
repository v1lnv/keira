#ifndef IDT_H
#define IDT_H

#include <stdint.h>

/**
 * Keira Kernel: Interrupt Descriptor Table (IDT)
 *
 * The IDT tells the CPU where to find the Interrupt Service Routines (ISRs)
 * for exceptions (0-31) and hardware interrupts (32-255).
 */

/* 64-bit IDT Entry (Gate Descriptor) */
typedef struct {
    uint16_t offset_low;  /* Offset bits 0..15 */
    uint16_t selector;    /* Code segment selector in GDT */
    uint8_t ist;          /* Interrupt Stack Table offset (0 = don't use) */
    uint8_t type_attr;    /* Type and attributes */
    uint16_t offset_mid;  /* Offset bits 16..31 */
    uint32_t offset_high; /* Offset bits 32..63 */
    uint32_t zero;        /* Reserved, must be 0 */
} __attribute__((packed)) idt_entry_t;

/* IDT Pointer (loaded with `lidt`) */
typedef struct {
    uint16_t limit; /* Size of IDT - 1 */
    uint64_t base;  /* Address of IDT */
} __attribute__((packed)) idt_ptr_t;

/**
 * Set an entry in the IDT.
 *
 * @param num   Interrupt vector number (0-255)
 * @param base  Address of the ISR
 * @param sel   Code segment selector (typically 0x08 for 64-bit kernel)
 * @param flags Type and attributes (typically 0x8E for 64-bit interrupt gate)
 * @param ist   Interrupt Stack Table index (0 for none)
 */
void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags, uint8_t ist);

/**
 * Initialize the IDT and load it into the CPU.
 */
void idt_init(void);

#endif /* IDT_H */

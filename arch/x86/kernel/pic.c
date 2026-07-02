#include <asm/io.h>
#include <asm/pic.h>

/* PIC I/O Ports */
#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1

/* PIC Initialization Commands */
#define ICW1_ICW4 0x01 /* Indicates that ICW4 will be present */
#define ICW1_INIT 0x10 /* Initialization - required! */
#define ICW4_8086 0x01 /* 8086/88 (x86) mode */

/* PIC End of Interrupt */
#define PIC_EOI 0x20 /* End of Interrupt command code */

void pic_init(int offset1, int offset2) {
    /* Start the initialization sequence (in cascade mode) */
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    /* ICW2: Master PIC vector offset */
    outb(PIC1_DATA, offset1);
    io_wait();
    /* ICW2: Slave PIC vector offset */
    outb(PIC2_DATA, offset2);
    io_wait();

    /* ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100) */
    outb(PIC1_DATA, 4);
    io_wait();
    /* ICW3: tell Slave PIC its cascade identity (0000 0010) */
    outb(PIC2_DATA, 2);
    io_wait();

    /* ICW4: have the PICs use 8086 mode (and not 8080 mode) */
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    /* Mask all interrupts by default (0xFF) */
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);

    /* Explicitly unmask IRQ2 (Cascade) so the Slave PIC can talk to the Master
     * PIC */
    pic_clear_mask(2);
}

void pic_eoi(unsigned char irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

void pic_set_mask(unsigned char irqline) {
    uint16_t port;
    uint8_t value;

    if (irqline < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irqline -= 8;
    }
    value = inb(port) | (1 << irqline);
    outb(port, value);
}

void pic_clear_mask(unsigned char irqline) {
    uint16_t port;
    uint8_t value;

    if (irqline < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irqline -= 8;
    }
    value = inb(port) & ~(1 << irqline);
    outb(port, value);
}

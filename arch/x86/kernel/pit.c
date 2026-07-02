#include <asm/io.h>
#include <asm/pic.h>
#include <asm/pit.h>

#define PIT_CMD_PORT 0x43
#define PIT_CH0_PORT 0x40

/* The oscillator frequency of the PIT chip is ~1.193182 MHz */
#define PIT_BASE_FREQ 1193180

static volatile uint64_t timer_ticks_ms = 0;

void pit_init(uint32_t frequency) {
    if (frequency == 0)
        return;

    /* Calculate the divisor */
    uint32_t divisor = PIT_BASE_FREQ / frequency;
    if (divisor > 65535)
        divisor = 65535;
    if (divisor == 0)
        divisor = 1;

    /*
     * Send the command byte.
     * 0x36 = 00110110 in binary:
     * Bits 6-7: 00 (Channel 0)
     * Bits 4-5: 11 (Access mode: lobyte/hibyte)
     * Bits 1-3: 011 (Operating mode 3: Square Wave Mode)
     * Bit 0:    0 (16-bit binary)
     */
    outb(PIT_CMD_PORT, 0x36);

    /* Divisor has to be sent byte-wise, so split into upper/lower bytes. */
    uint8_t l = (uint8_t)(divisor & 0xFF);
    uint8_t h = (uint8_t)((divisor >> 8) & 0xFF);

    /* Send the frequency divisor */
    outb(PIT_CH0_PORT, l);
    outb(PIT_CH0_PORT, h);

    /* Unmask IRQ0 (PIT Timer) so the PIC passes the interrupts to the CPU */
    pic_clear_mask(0);
}

void pit_handler(void) {
    timer_ticks_ms++;

    /*
     * Send EOI (End of Interrupt) to master PIC (IRQ0 is on master).
     */
    pic_eoi(0);
}

uint64_t get_uptime_ms(void) {
    return timer_ticks_ms;
}

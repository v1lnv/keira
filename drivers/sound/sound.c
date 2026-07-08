/**
 * Keira Kernel: PC Speaker Sound Driver (Implementation)
 *
 * Controls audio output through the PC Speaker by programming PIT Channel 2
 * as a square wave generator and toggling the speaker gate on System Control
 * Port B (0x61).
 *
 * The PC Speaker hardware chain:
 *
 *   PIT Channel 2  -->  AND gate  -->  Speaker Cone
 *                        ^
 *                        |
 *                   Port 0x61 bit 0 (gate)
 *                   Port 0x61 bit 1 (speaker data)
 *
 * To produce sound, both the PIT gate and speaker data bits must be enabled.
 * The PIT oscillator runs at a base frequency of 1,193,182 Hz. A 16-bit
 * divisor is loaded to set the output frequency: freq = 1193182 / divisor.
 *
 * Reference: https://wiki.osdev.org/PC_Speaker
 */

#include "include/sound.h"
#include "../../arch/x86/include/asm/io.h"
#include "regs.h"

/* Register offsets and bitmasks are included from regs.h */

void sound_play(uint32_t freq) {
    if (freq == 0) {
        return;
    }

    /* Calculate the PIT frequency divisor */
    uint32_t div = PIT_BASE_FREQ / freq;

    /* Configure PIT Channel 2 for square wave generation (Mode 3) */
    outb(PIT_CMD, PIT_CH2_MODE3);

    /* Load the 16-bit divisor (low byte first, then high byte) */
    outb(PIT_CH2_DATA, (uint8_t)(div & 0xFF));
    outb(PIT_CH2_DATA, (uint8_t)((div >> 8) & 0xFF));

    /* Enable the speaker gate: set bits 0 and 1 of Port B */
    uint8_t ctrl = inb(SYS_CTRL_B);
    if ((ctrl & SPKR_ENABLE) != SPKR_ENABLE) {
        outb(SYS_CTRL_B, ctrl | SPKR_ENABLE);
    }
}

void sound_stop(void) {
    /* Disable the speaker gate: clear bits 0 and 1 of Port B */
    uint8_t ctrl = inb(SYS_CTRL_B);
    outb(SYS_CTRL_B, ctrl & SPKR_DISABLE);
}

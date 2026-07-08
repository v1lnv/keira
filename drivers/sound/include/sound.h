/**
 * Keira Kernel: PC Speaker Sound Driver (Header)
 *
 * Public interface for the PC Speaker driver using PIT Channel 2.
 * Provides frequency output and speaker gate control for audio playback.
 *
 * Usage:
 *   sound_play(1000);   // Play a 1000 Hz tone
 *   sound_stop();       // Silence the speaker
 */

#ifndef KEIRA_SOUND_H
#define KEIRA_SOUND_H

#include <stdint.h>

/**
 * Play a tone at the specified frequency on the PC Speaker.
 *
 * Configures PIT Channel 2 as a square wave generator and enables
 * the speaker gate via System Control Port B (0x61).
 *
 * @param freq The target frequency in Hz (must be > 0).
 */
void sound_play(uint32_t freq);

/**
 * Stop all sound output on the PC Speaker.
 *
 * Clears the speaker gate and PIT Channel 2 gate bits in
 * System Control Port B (0x61).
 */
void sound_stop(void);

#endif /* KEIRA_SOUND_H */

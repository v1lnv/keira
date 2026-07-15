#ifndef HDA_H
#define HDA_H

#include <stdint.h>

/**
 * Initialize Intel High Definition Audio controller
 * Maps registers and resets controller
 */
void hda_init(uint64_t bar_phys);

/**
 * Starts audio DMA playback with a square wave of the specified frequency
 * Uses double buffering with allocated page buffers
 */
void hda_start_tone(uint64_t bdl_phys, uint64_t buf1_phys, uint64_t buf2_phys, uint32_t freq);

/**
 * Stops the audio DMA output stream
 */
void hda_stop(void);

#endif // HDA_H

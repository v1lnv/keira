/**
 * Keira Kernel: Intel High Definition Audio (HDA) Driver
 *
 * Implements register-level initialization, Immediate Command verb routing,
 * and output stream DMA configuration for Intel HD Audio.
 */

#include "include/hda.h"

// HDA Register Offsets
#define HDA_REG_GCTL       0x08
#define HDA_REG_STATESTS   0x0E
#define HDA_REG_IC         0x60
#define HDA_REG_IR         0x64
#define HDA_REG_ICS        0x68

// Stream 4 (Output 0) Descriptor Offsets (Base 0x80 + 4 * 0x20 = 0x100)
#define SD_BASE            0x100
#define SD_REG_CTL         (SD_BASE + 0x00)
#define SD_REG_STS         (SD_BASE + 0x03)
#define SD_REG_CBL         (SD_BASE + 0x08)
#define SD_REG_LVI         (SD_BASE + 0x0C)
#define SD_REG_FMTS        (SD_BASE + 0x12)
#define SD_REG_BDPL        (SD_BASE + 0x18)
#define SD_REG_BDPU        (SD_BASE + 0x1C)

struct hda_bdl_entry {
    uint32_t phys_low;
    uint32_t phys_high;
    uint32_t length;
    uint32_t flags; // Bit 0: IOC (Interrupt on Completion)
} __attribute__((packed));

static uint64_t hda_base_virt = 0;

static void delay(int count) {
    volatile int i = count;
    while (i > 0) {
        i--;
    }
}

static uint32_t hda_send_verb(uint8_t codec, uint8_t node, uint32_t verb_param) {
    if (!hda_base_virt) return 0;

    volatile uint32_t *ic = (volatile uint32_t *)(hda_base_virt + HDA_REG_IC);
    volatile uint32_t *ir = (volatile uint32_t *)(hda_base_virt + HDA_REG_IR);
    volatile uint16_t *ics = (volatile uint16_t *)(hda_base_virt + HDA_REG_ICS);

    // Wait until Immediate Command Status is not busy (bit 0 == 0)
    int timeout = 100000;
    while ((*ics & 1) && timeout > 0) {
        timeout--;
    }
    if (timeout == 0) return 0;

    // Clear Immediate Response Valid (bit 1) by writing 1 to it
    *ics = *ics | 2;

    // Form 32-bit command: [codec:4][node:8][verb_param:20]
    uint32_t command = ((uint32_t)codec << 28) | ((uint32_t)node << 20) | (verb_param & 0xFFFFF);
    *ic = command;

    // Trigger execution: set ICB (bit 0) to 1, clear IRV (bit 1) write
    *ics = (*ics & ~2) | 1;

    // Wait until done
    timeout = 100000;
    while ((*ics & 1) && timeout > 0) {
        timeout--;
    }
    if (timeout == 0) return 0;

    return *ir;
}

void hda_init(uint64_t bar_phys) {
    hda_base_virt = bar_phys;

    volatile uint32_t *gctl = (volatile uint32_t *)(hda_base_virt + HDA_REG_GCTL);

    // 1. Reset controller (clear CRST bit 0 to 0)
    *gctl &= ~1;
    int timeout = 100000;
    while ((*gctl & 1) && timeout > 0) {
        timeout--;
    }
    delay(10000);

    // 2. Bring out of reset (set CRST bit 0 to 1)
    *gctl |= 1;
    timeout = 100000;
    while (!(*gctl & 1) && timeout > 0) {
        timeout--;
    }
    delay(10000);

    // 3. Configure primary codec widgets (Headphone Node 0x03, DAC Node 0x02)
    // Pin Widget Control: Enable Output
    hda_send_verb(0, 0x03, (0x707 << 8) | 0x40);
    // Connection Select: Pin Widget Node 0x03 selects DAC Node 0x02
    hda_send_verb(0, 0x03, (0x701 << 8) | 0x00);
    // Output Amp Gain/Mute on DAC Node 0x02: Unmute, maximum volume (0x7F)
    hda_send_verb(0, 0x02, (0x3 << 16) | 0xB07F);
    // Output Amp Gain/Mute on Pin Node 0x03: Unmute, maximum volume (0x7F)
    hda_send_verb(0, 0x03, (0x3 << 16) | 0xB07F);
}

static void hda_fill_square(int16_t *buf, int count, int freq) {
    // 48000 Hz, stereo (2 channels), 16-bit
    int sample_rate = 48000;
    int period = sample_rate / freq;
    int half_period = period / 2;

    for (int i = 0; i < count; i++) {
        int phase = i % period;
        int16_t val = (phase < half_period) ? 8000 : -8000;
        buf[2 * i] = val;     // Left
        buf[2 * i + 1] = val; // Right
    }
}

void hda_start_tone(uint64_t bdl_phys, uint64_t buf1_phys, uint64_t buf2_phys, uint32_t freq) {
    if (!hda_base_virt) return;

    // Fill double buffers with square wave
    // 4096 bytes per page / 4 bytes per stereo sample = 1024 samples
    hda_fill_square((int16_t *)buf1_phys, 1024, freq);
    hda_fill_square((int16_t *)buf2_phys, 1024, freq);

    // Set up BDL entries (2 buffers * 4096 bytes)
    struct hda_bdl_entry *bdl = (struct hda_bdl_entry *)bdl_phys;
    
    bdl[0].phys_low = (uint32_t)buf1_phys;
    bdl[0].phys_high = (uint32_t)(buf1_phys >> 32);
    bdl[0].length = 4096;
    bdl[0].flags = 1; // IOC (Interrupt on Completion)

    bdl[1].phys_low = (uint32_t)buf2_phys;
    bdl[1].phys_high = (uint32_t)(buf2_phys >> 32);
    bdl[1].length = 4096;
    bdl[1].flags = 1;

    // Configure Stream ID and Format on DAC Node 0x02
    // Format: 48kHz, 16-bit, 2 channels -> 0x0011
    hda_send_verb(0, 0x02, (0x2 << 16) | 0x0011);
    // Stream 1, Channel 0 -> 0x10
    hda_send_verb(0, 0x02, (0x706 << 8) | 0x10);

    // Configure Output Stream 0 (SD4) registers
    volatile uint32_t *sd_ctl = (volatile uint32_t *)(hda_base_virt + SD_REG_CTL);
    volatile uint8_t *sd_sts = (volatile uint8_t *)(hda_base_virt + SD_REG_STS);
    volatile uint32_t *sd_cbl = (volatile uint32_t *)(hda_base_virt + SD_REG_CBL);
    volatile uint16_t *sd_lvi = (volatile uint16_t *)(hda_base_virt + SD_REG_LVI);
    volatile uint16_t *sd_fmts = (volatile uint16_t *)(hda_base_virt + SD_REG_FMTS);
    volatile uint32_t *sd_bdpl = (volatile uint32_t *)(hda_base_virt + SD_REG_BDPL);
    volatile uint32_t *sd_bdpu = (volatile uint32_t *)(hda_base_virt + SD_REG_BDPU);

    // 1. Disable stream first
    *sd_ctl &= ~2; // Clear RUN bit (bit 1)
    int timeout = 100000;
    while ((*sd_ctl & 2) && timeout > 0) {
        timeout--;
    }

    // 2. Set physical address of BDL
    *sd_bdpl = (uint32_t)bdl_phys;
    *sd_bdpu = (uint32_t)(bdl_phys >> 32);

    // 3. Set buffer length (8192 bytes total)
    *sd_cbl = 8192;

    // 4. Set last valid index to 1 (0-indexed, so 2 entries total)
    *sd_lvi = 1;

    // 5. Set Stream Format (0x0011) and Stream ID (1)
    *sd_fmts = 0x0011;
    
    // Clear status register
    *sd_sts = 0xFF;

    // Set Stream ID to 1 in Control register (bits 20-23)
    uint32_t ctrl = *sd_ctl;
    ctrl &= ~(0xF << 20);
    ctrl |= (1 << 20);
    *sd_ctl = ctrl;

    // 6. Enable stream (set RUN bit 1 to 1)
    *sd_ctl |= 2;
}

void hda_stop(void) {
    if (!hda_base_virt) return;

    volatile uint32_t *sd_ctl = (volatile uint32_t *)(hda_base_virt + SD_REG_CTL);
    *sd_ctl &= ~2; // Clear RUN bit
}

#ifndef SOUND_REGS_H
#define SOUND_REGS_H

/* Programmable Interval Timer (PIT) Registers */
#define PIT_CMD        0x43   /* PIT Mode/Command register                   */
#define PIT_CH2_DATA   0x42   /* PIT Channel 2 data port (frequency divisor) */

/* System Control Port B */
#define SYS_CTRL_B     0x61   /* Speaker gate and PIT Channel 2 gate control */

/* PIT Command Byte for Channel 2, Mode 3 (Square Wave Generator) */
#define PIT_CH2_MODE3  0xB6   /* Channel 2 | Access lo/hi | Mode 3 | Binary */

/* PIT Base Oscillator Frequency (Hz) */
#define PIT_BASE_FREQ  1193182

/* System Control Port B Bitmasks */
#define SPKR_GATE_EN   0x01   /* Bit 0: PIT Channel 2 gate enable           */
#define SPKR_DATA_EN   0x02   /* Bit 1: Speaker data enable                 */
#define SPKR_ENABLE    (SPKR_GATE_EN | SPKR_DATA_EN) /* Both bits combined  */
#define SPKR_DISABLE   0xFC   /* Mask to clear both speaker bits             */

#endif /* SOUND_REGS_H */

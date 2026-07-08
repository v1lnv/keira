# PC Speaker Sound Driver & Melodies

This module documents the low-level hardware control of the PC Speaker (via PIT Channel 2) and the implementation of the `play` command to output audio frequencies and melodies.

---

## 1. Architecture Overview

The sound subsystem follows Keira's C↔Rust modular driver model:

```
┌──────────────────────────────────────────────────────────────────┐
│  Shell `play` command (Rust)                                     │
│  kernel/src/shell/cmds/play.rs                                   │
├──────────────────────────────────────────────────────────────────┤
│  Safe Rust FFI Wrapper                                           │
│  kernel/src/io/sound.rs                                          │
│  play_sound() → sound_play()    stop_sound() → sound_stop()     │
├──────────────────────────────────────────────────────────────────┤
│  C Hardware Driver                                               │
│  drivers/sound/sound.c                                           │
│  Handles PIT programming and speaker gate control via outb/inb  │
├──────────────────────────────────────────────────────────────────┤
│  Register Definitions        │  Shared I/O Abstraction          │
│  drivers/sound/regs.h        │  arch/x86/include/asm/io.h      │
└──────────────────────────────┴──────────────────────────────────┘
```

### File Layout

| File | Purpose |
|------|---------|
| `drivers/sound/sound.c` | C implementation: PIT Channel 2 and speaker gate I/O |
| `drivers/sound/include/sound.h` | Public C header (API for FFI) |
| `drivers/sound/regs.h` | Hardware register addresses and bitmasks |
| `kernel/src/io/sound.rs` | Safe Rust wrapper calling C functions via `extern "C"` |

---

## 2. Programmable Interval Timer (PIT) Channel 2

The PC Speaker in standard IBM PC compatible hardware is connected to Channel 2 of the PIT:
- **PIT Frequency**: The hardware oscillator runs at a base frequency of **1,193,182 Hz**.
- **Channel 2 Ports**:
  - **`0x43` (PIT Mode/Command Register)**: Written to configure Channel 2 mode.
  - **`0x42` (PIT Channel 2 Data Register)**: Read or written to set the 16-bit frequency divisor.
- **System Control Port B (`0x61`)**:
  - Controls the gate of PIT Channel 2 and the speaker connection.
  - **Bit 0**: Gate of PIT Channel 2 (0 = disable, 1 = enable).
  - **Bit 1**: Speaker connection (0 = disconnect, 1 = connect).

---

## 3. Audio Control Implementation

### Starting Sound (`sound_play` in C / `play_sound` in Rust)
To output a frequency on the speaker, the driver:
1. Calculates the divisor: `div = PIT_BASE_FREQ / frequency`.
2. Writes `PIT_CH2_MODE3` (`0xB6`) to the Mode/Command register to set PIT Channel 2 to mode 3 (square wave generator).
3. Writes the low byte of the divisor followed by the high byte to data register `PIT_CH2_DATA`.
4. Reads the System Control Port B (`SYS_CTRL_B`), sets `SPKR_ENABLE` bits, and writes it back.

### Stopping Sound (`sound_stop` in C / `stop_sound` in Rust)
To stop sound output, the driver reads Port `SYS_CTRL_B`, masks with `SPKR_DISABLE` to clear the lower 2 bits, and writes it back:
```c
uint8_t ctrl = inb(SYS_CTRL_B);
outb(SYS_CTRL_B, ctrl & SPKR_DISABLE);
```

### Delay & Notes (`play_note` in Rust)
To play distinct notes:
1. Trigger sound at the note's target frequency.
2. Sleep for the duration of the note (in milliseconds) using CPU `hlt` instructions.
3. Turn off the sound.
4. Sleep for a short gap (e.g. 10ms) to distinguish consecutive identical notes.

---

## 4. Shell `play` Command

The `play` command processes arguments to select one of the built-in retro melodies:

- **`mario`**: Plays the famous introductory melody of the Super Mario Bros Theme.
- **`nokia`**: Plays the standard retro Nokia Tune.
- **`starwars`**: Plays the majestic main theme of Star Wars.
- **`beep`**: Plays a single 1000Hz beep for 200 milliseconds.

Usage:
```bash
play mario
play starwars
```

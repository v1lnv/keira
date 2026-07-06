# PC Speaker Sound Driver & Melodies

This module documents the low-level hardware control of the PC Speaker (via PIT Channel 2) and the implementation of the `play` command to output audio frequencies and melodies.

---

## 1. Programmable Interval Timer (PIT) Channel 2

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

## 2. Audio Control Implementation

### Starting Sound (`play_sound`)
To output a frequency on the speaker, the driver:
1. Calculates the divisor: `div = 1193182 / frequency`.
2. Writes `0xB6` to the Mode/Command register (`0x43`) to set PIT Channel 2 to mode 3 (square wave generator).
3. Writes the low byte of the divisor followed by the high byte to data register `0x42`.
4. Reads the System Control Port B (`0x61`), sets bits `0` and `1` to high, and writes it back to enable the speaker gate and connect it.

### Stopping Sound (`stop_sound`)
To stop sound output, the driver reads Port `0x61`, clears the lower 2 bits (bits `0` and `1`), and writes it back:
```rust
let tmp = inb(0x61) & 0xFC;
outb(0x61, tmp);
```

### Delay & Notes (`play_note`)
To play distinct notes:
1. Trigger sound at the note's target frequency.
2. Sleep for the duration of the note (in milliseconds) using CPU `hlt` instructions.
3. Turn off the sound.
4. Sleep for a short gap (e.g. 10ms) to distinguish consecutive identical notes.

---

## 3. Shell `play` Command

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

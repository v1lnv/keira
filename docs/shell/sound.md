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

---

## 5. Intel High Definition Audio (HDA) Subsystem

Keira v0.7.0 adds support for the Intel High Definition Audio (HDA) standard. HDA allows playing actual digital audio (PCM samples) rather than simple frequency beeps.

### Architecture Overview

```
┌──────────────────────────────────────────────────────────────────┐
│  Shell `hda` command (Rust)                                      │
│  kernel/src/shell/cmds/hda.rs                                    │
├──────────────────────────────────────────────────────────────────┤
│  Safe Rust FFI Wrapper & DMA Manager                             │
│  kernel/src/io/hda.rs                                            │
│  Allocates BDL page and DMA buffers, maps MMIO BAR0              │
├──────────────────────────────────────────────────────────────────┤
│  C Hardware Driver                                               │
│  drivers/sound/hda.c                                             │
│  Resets HDA, routes codec widgets, and runs the stream DMA engine│
└──────────────────────────────────────────────────────────────────┘
```

### File Layout

| File | Purpose |
|------|---------|
| `drivers/sound/hda.c` | C implementation: HDA stream DMA and immediate command codec configuration |
| `drivers/sound/include/hda.h` | Public C header (API for FFI) |
| `kernel/src/io/hda.rs` | Rust PCI device scanner, memory mapper, and DMA allocator |
| `kernel/src/shell/cmds/hda.rs` | Shell command implementation for `hda` |

### Initialization Sequence

1. **PCI Discovery**: The Rust scanner matches the HDA device (Class `0x04`, Subclass `0x03`).
2. **PCI Configuration**: Enables PCI Bus Mastering and Memory Space mapping by writing to the PCI Command Register.
3. **Register Mapping**: Maps the HDA controller MMIO BAR0 (up to 16KB physical space) to virtual pages.
4. **DMA Allocation**: Allocates three physical page frames:
   - One frame for the Buffer Descriptor List (BDL).
   - Two frames for double-buffered stereo samples (Buffer 1 & Buffer 2).
5. **Controller Reset**: Resets the HDA controller via the Global Control Register (GCTL, offset `0x08`).
6. **Codec Configuration**: Uses the Immediate Command interface (offset `0x60`-`0x68`) to:
   - Enable output pin widget (Node `0x03`).
   - Route Audio Output DAC (Node `0x02`) to Node `0x03`.
   - Unmute and set maximum volume on both DAC and Pin widgets.

### DMA Stream Playback

When starting playback (e.g., `hda play 440`):
1. **Sample Generation**: The driver fills the two 4KB page buffers with a 16-bit stereo square-wave signal corresponding to the target frequency (using a 48,000 Hz sample rate).
2. **BDL Setup**: Populates two BDL entries, one for each page buffer, setting the `IOC` (Interrupt on Completion) flag.
3. **Stream Configuration**: Configures Output Stream 0 (stream 4, offset `0x100`):
   - Sets the stream BDL physical address.
   - Sets total length to 8192 bytes.
   - Sets the last valid descriptor index (LVI) to 1.
   - Sets the format to 48kHz, 16-bit, stereo.
   - Assigns Stream ID 1.
4. **Trigger Playback**: Activates the stream by setting the RUN bit in the stream control register. The HDA controller automatically cycles through the buffers in a hardware-controlled loop.

### Shell `hda` Command

Usage:
- **`hda status`**: Check if HDA was found and initialized.
- **`hda play <freq>`**: Play a continuous wave tone at `<freq>` Hz (default 440 Hz).
- **`hda stop`**: Stop the active audio stream.

```bash
hda status
hda play 880
hda stop
```

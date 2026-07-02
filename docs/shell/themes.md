# VGA Theme Engine

This module documents the dynamic color styling engine, color registers, and custom screen clearing implementations in the Keira Kernel.

---

## 1. VGA Color Palette and Attributes

The VGA display controller in text mode maps character cells to 16-bit values containing 8-bit character indices and 8-bit color attributes:
- **Foreground Color (Bits 0-3)**: Maps to 16 standard CGA colors.
- **Background Color (Bits 4-7)**: Maps to 16 colors (high bit toggles blinking if enabled).

```
16-Bit VGA Cell:
 [ 15 | 14 | 13 | 12 | 11 | 10 |  9 |  8 ] [  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 ]
 [    Background     |     Foreground     ] [            ASCII Character          ]
```

---

## 2. Shell Theme Structure (`ShellTheme`)

The shell coordinates colors dynamically using the global `CURRENT_THEME` structure defined in `kernel/src/shell/state.rs`:
```rust
pub struct ShellTheme {
    pub user: vga::Color,     // Prompt username color
    pub host: vga::Color,     // Prompt hostname color
    pub path: vga::Color,     // Prompt working directory path color
    pub symbol: vga::Color,   // Prompt separator symbol (») color
    pub text_fg: vga::Color,  // Standard command input and output text color
    pub text_bg: vga::Color,  // Terminal background color
}
```

---

## 3. Dynamic Theme Configurations

Selecting a theme via `theme <name>` updates the `CURRENT_THEME` structure:

| Theme | User/Host | Path | Symbol | Text Fg | Text Bg | Style |
| ----- | --------- | ---- | ------ | ------- | ------- | ----- |
| `classic` | `LightRed` | `LightBlue` | `LightGreen` | `LightGrey` | `Black` | Standard Console |
| `retro` | `Green` | `LightGreen` | `LightGreen` | `LightGreen` | `Black` | Green Phosphor CRT |
| `matrix` | `LightGreen` | `Green` | `Green` | `Green` | `Black` | Hacker Lime |
| `arch` | `LightBlue` | `LightBlue` | `Cyan` | `LightCyan` | `Black` | Arch Blue Console |
| `dracula` | `LightMagenta`| `LightMagenta`| `LightGreen` | `White` | `Black` | Gothic Dark Theme |

---

## 4. Theme-Aware Screen Clearing

To achieve a full-screen theme transition, the terminal screen-clearing driver is integrated with the active theme background color:
1. **Attribute Update**: The `theme` command calls `vga::set_color(text_fg, text_bg)` to update the active write attribute in the VGA driver.
2. **Re-initialization**: Invokes the C function `vga_init()` (the handler behind the `wipe` command).
3. **Screen Paint**: `vga_init()` reads the modified color attribute. It loops through all 2000 character cells (80 × 25) and overwrites them with a space character `' '` and the new color attribute.
4. **Result**: The entire screen immediately updates to the selected theme's background color, providing a smooth and professional terminal styling experience.

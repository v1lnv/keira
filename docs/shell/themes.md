# Console Theme Engine

This module documents the dynamic color styling engine, color registers, and custom screen clearing implementations in both VGA text mode and Multiboot2 Linear Framebuffer (LFB) graphics mode.

---

## 1. Framebuffer Color Palette and Attributes

The console driver supports two display interfaces:
- **VGA Text Mode (Fallback)**: Character cells are mapped to 16-bit values containing 8-bit character indices and 8-bit color attributes (foreground and background).
- **Linear Framebuffer (Primary)**: Standard CGA 16-color palette colors are mapped to 32-bit RGB hex colors during pixel drawing:

| CGA Color | RGB Hex Value | Description |
| --------- | ------------- | ----------- |
| `Black` | `0x000000` | Pure Black |
| `Blue` | `0x0000AA` | Standard Blue |
| `Green` | `0x00AA00` | Standard Green |
| `Cyan` | `0x00AAAA` | Standard Cyan |
| `Red` | `0xAA0000` | Standard Red |
| `Magenta` | `0xAA00AA` | Standard Magenta |
| `Brown` | `0xAA5500` | Standard Brown/Orange |
| `LightGrey` | `0xAAAAAA` | Light Grey (default text) |
| `DarkGrey` | `0x555555` | Dark Grey |
| `LightBlue` | `0x5555FF` | High-intensity Blue |
| `LightGreen` | `0x55FF55` | High-intensity Green |
| `LightCyan` | `0x55FFFF` | High-intensity Cyan |
| `LightRed` | `0xFF5555` | High-intensity Red |
| `LightMagenta`| `0xFF55FF` | High-intensity Magenta |
| `Yellow` | `0xFFFF55` | Standard Yellow |
| `White` | `0xFFFFFF` | Pure White |

---

## 2. Shell Theme Structure (`ShellTheme`)

The shell coordinates colors dynamically using the global `CURRENT_THEME` structure defined in `kernel/src/shell/state.rs`:
```rust
pub struct ShellTheme {
    pub user: vga::Color,     // Prompt username color
    pub host: vga::Color,     // Prompt hostname color
    pub path: vga::Color,     // Prompt working directory path color
    pub symbol: vga::Color,   // Prompt separator symbol (>) color
    pub text_fg: vga::Color,  // Standard command input and output text color
    pub text_bg: vga::Color,  // Terminal background color
}
```

### Clean-Look Dynamic Styling
- **Subtle Hostname**: `@keira` is printed with low contrast using `Color::LightGrey` to keep the prompt clean.
- **Root/Admin Highlight**: If the active user has root privileges (`IS_ADMIN` is `true`), the username color overrides `ShellTheme.user` and displays in `Color::LightRed`.
- **Minimalist Separator**: The prompt uses a single right-angle bracket `>` (ASCII 62) as the input separator.

---

## 3. Dynamic Theme Configurations

Selecting a theme via the `theme <name>` command updates the active colors:

| Theme | User/Host | Path | Symbol | Text Fg | Text Bg | Style |
| ----- | --------- | ---- | ------ | ------- | ------- | ----- |
| `classic` | `LightGreen`| `LightCyan` | `LightGreen` | `White` | `Black` | Modern Minimalist |
| `retro` | `Green` | `LightGreen` | `LightGreen` | `LightGreen` | `Black` | Green Phosphor CRT |
| `matrix` | `LightGreen`| `Green` | `Green` | `Green` | `Black` | Hacker Lime |
| `arch` | `LightBlue` | `LightBlue` | `Cyan` | `LightCyan` | `Black` | Arch Blue Console |
| `dracula` | `LightMagenta`| `LightMagenta`| `LightGreen` | `White` | `Black` | Gothic Dark Theme |

---

## 4. Theme-Aware Screen Clearing

To achieve a full-screen theme transition, screen clearing is integrated with the active theme background color:
1. **Attribute Update**: The `theme` command updates the `ACTIVE_FG_COLOR` and `ACTIVE_BG_COLOR` values in the graphics console.
2. **Screen Paint**: The clearing driver (`wipe` command / `vga::init()`) detects the display mode:
   - **Text Mode**: Overwrites the 80x25 characters with a space `' '` and the new 8-bit attribute byte.
   - **Graphics Mode**: Fills the entire linear framebuffer pixel array (`width * height` pixels) with the 32-bit `ACTIVE_BG_COLOR` color, then redraws the blinking cursor and mouse graphics.
3. **Result**: The entire console immediately matches the selected theme's background color.

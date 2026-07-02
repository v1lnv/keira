# Full-Screen Text Editor

This module covers the design and controls of the full-screen visual text editor implemented in the Keira Kernel.

---

## 1. Grid and Coordinate Structures

When editing a file (`edit <filename>`), the shell enters full-screen editor mode:
- **Character Grid**: The text editor manages a 2D buffer `EDITOR_GRID` containing `80 columns × 23 rows` of text characters.
- **Line Length Tracking**: The length of each active line is tracked in the `LINE_LENS` array to prevent reading garbage spaces beyond active characters.
- **Coordinates**: The cursor coordinates (`EDIT_CUR_X`, `EDIT_CUR_Y`) track the character grid position.

---

## 2. Editor Layout and UI Components

The screen is divided into three distinct visual regions:
1. **Header Bar (Row 0)**: Displays the active filename, modified status, and help shortcuts in reverse video.
2. **Text Area (Row 1 to 23)**: Displays the text content. The hardware cursor tracks the editing coordinates within this area.
3. **Status Bar (Row 24)**: Displays warnings or shortcut commands (`F3` to save, `F10` to exit).

---

## 3. Interactive Keyboard Controls

The editor intercepts all keyboard inputs to provide full-screen controls:
- **Typing**: Inserts characters at `(X, Y)`, increments `X`, and updates line length.
- **Backspace**: If `X > 0`, it decrements `X`, shifts subsequent characters left, and decrements line length.
- **Arrow Keys**: Move the cursor within line and row boundaries.
- **Enter Key**: Moves the cursor to the start of the next row (`X = 0`, `Y++`).
- **F3 (Save)**: Flushes the 2D character buffer (excluding trailing spaces) to the active drive file.
- **F10 (Exit)**: Exits editor mode and restores the interactive terminal shell.

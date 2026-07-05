# Shell Event Loop & Input

This module explains the interactive shell loop, keyboard input handling, autocomplete logic, and command execution flow.

---

## 1. Shell Thread Event Loop

The shell runs as a dedicated system thread in the multitasking scheduler:
- It initializes the prompt, saves the cursor columns/rows for editing, and sits in a loop polling the `COMMAND_READY` state variable.
- When `COMMAND_READY` is toggled to `true` (triggered by pressing Enter), the shell copies the input buffer, sets it back to empty, and routes the command string to the command executor.
- Once execution finishes, it re-renders the prompt and waits for the next input.

---

## 2. Keyboard Input and Backspace Handling

Keyboard signals generate IRQ1 interrupts:
- **Scan Codes**: The keyboard driver reads the scan codes from port `0x60` and converts them to ASCII characters.
- **Backspace Deletion**: Pressing Backspace triggers the safe `vga::backspace()` helper:
  1. Decrements the cursor column position.
  2. Overwrites the character cell/pixel grid with a space `' '`.
  3. Updates the visual cursor position.
- **Special Keys**: Arrow keys trigger history navigation. The Tab key triggers autocomplete.

---

## 3. Tab Autocomplete Engine

The autocomplete engine is triggered when the Tab key is pressed:
1. The driver checks the current word prefix being typed in `INPUT_BUFFER`.
2. It queries `find_matches` in the active directory (FAT16 or Initrd) to find files or folders starting with the typed prefix.
3. If exactly one match is found:
   - It appends the missing suffix to the input buffer and prints it to the VGA screen, updating the cursor column instantly.
4. If multiple matches are found:
   - It lists the matching file options below the prompt for the user to select.

---

## 4. Shell Redirection and Command Pipelines

Keira shell supports standard I/O redirection and pipelines (`|`, `>`, `<`) parsed sequentially inside the executor:

- **Output Redirection (`>`)**: Intercepts console print strings by toggling `REDIRECT_TO_FILE` and copying characters to a `REDIRECT_BUFFER`. Once the command completes, it writes this buffer to the target file.
- **Input Redirection (`<`)**: Reads a file's content from storage into a global `PIPE_BUFFER` and activates `PIPE_ACTIVE`. Future shell/command read operations fetch characters directly from the buffer instead of blocking on keyboard interrupts.
- **Command Pipelines (`|`)**: Combines both redirection mechanisms. If `cmd1 | cmd2` is executed, the shell runs `cmd1` with output redirection active, copies `REDIRECT_BUFFER` directly to `PIPE_BUFFER`, toggles `PIPE_ACTIVE = true`, and executes `cmd2` utilizing the piped buffer as its standard input stream.
- **`grep` Command**: A text-filtering utility that searches for pattern matches per-line. It reads from a specified file or automatically reads from `PIPE_BUFFER` if pipeline input redirection is active.

//! Keira Kernel: VGA Code Editor

use super::state::*;
use crate::io::vga;

extern "C" {
    fn vga_init();
    fn vga_get_cursor_col() -> u16;
    fn vga_set_cursor_pos(row: u16, col: u16);
}

unsafe fn editor_save_file() -> Result<(), &'static str> {
    let mut flat_buf = [0u8; 2048];
    let mut flat_len = 0;

    let mut last_y = 0;
    for y in 0..23 {
        if LINE_LENS[y] > 0 {
            last_y = y;
        }
    }

    for y in 0..=last_y {
        let row_len = LINE_LENS[y] as usize;
        for x in 0..row_len {
            if flat_len < 2048 {
                flat_buf[flat_len] = EDITOR_GRID[y][x];
                flat_len += 1;
            }
        }

        if y < last_y {
            if flat_len < 2048 {
                flat_buf[flat_len] = b'\n';
                flat_len += 1;
            }
        }
    }

    while flat_len > 0 && flat_buf[flat_len - 1] == b'\n' {
        flat_len -= 1;
    }

    let filename_slice = &EDIT_FILENAME[..EDIT_FILENAME_LEN];
    let filename_str =
        core::str::from_utf8(filename_slice).map_err(|_| "Invalid filename encoding")?;

    crate::fs::fat::write_file_content(filename_str, &flat_buf[..flat_len])?;
    Ok(())
}

pub unsafe fn editor_start(filename: &str) -> Result<(), &'static str> {
    EDIT_FILENAME = [0; 12];
    EDIT_FILENAME_LEN = core::cmp::min(filename.len(), 12);
    EDIT_FILENAME[..EDIT_FILENAME_LEN].copy_from_slice(filename.as_bytes());

    EDITOR_GRID = [[b' '; 80]; 23];
    LINE_LENS = [0; 23];
    EDIT_CUR_X = 0;
    EDIT_CUR_Y = 0;
    EDITOR_CONFIRM_EXIT = false;
    EDITOR_CONFIRM_SAVE = false;

    let mut file_buf = [0u8; 2048];
    match crate::fs::fat::read_file_content(filename, &mut file_buf) {
        Ok(bytes_read) => {
            let mut x = 0;
            let mut y = 0;
            for &b in &file_buf[..bytes_read] {
                if b == b'\n' {
                    if y < 23 {
                        LINE_LENS[y] = x as u16;
                    }
                    x = 0;
                    y += 1;
                    if y >= 23 {
                        break;
                    }
                } else if b == b'\r' {
                    // skip CR
                } else {
                    if x < 80 {
                        EDITOR_GRID[y][x] = b;
                        x += 1;
                    }
                }
            }
            if y < 23 {
                LINE_LENS[y] = x as u16;
            }
        }
        Err(_) => {
            if let Err(e) = crate::fs::fat::create_file(filename) {
                if e != "File or directory already exists" {
                    return Err(e);
                }
            }
        }
    }

    IN_EDITOR_MODE = true;
    editor_redraw();
    Ok(())
}

pub unsafe fn editor_redraw() {
    vga_init();

    // 1. Draw top bar (Header)
    vga::set_color(vga::Color::White, vga::Color::DarkGrey);
    vga::print_str("  Keira Text Editor 0.1.0  |  File: ");
    let filename_slice = &EDIT_FILENAME[..EDIT_FILENAME_LEN];
    if let Ok(name_str) = core::str::from_utf8(filename_slice) {
        vga::print_str(name_str);
    }
    vga::print_str(" ");

    let mut current_col = vga_get_cursor_col();
    while current_col < 80 {
        vga::print_str(" ");
        current_col += 1;
    }

    // 2. Draw grid content with syntax highlighting
    for y in 0..23 {
        vga_set_cursor_pos((y + 1) as u16, 0);
        let len = LINE_LENS[y] as usize;
        let mut x = 0;

        while x < len {
            let c = EDITOR_GRID[y][x];

            // 1. Highlight numbers
            if c >= b'0' && c <= b'9' {
                vga::set_color(vga::Color::LightRed, vga::Color::Black);
                let s = [c];
                if let Ok(c_str) = core::str::from_utf8(&s) {
                    vga::print_str(c_str);
                }
                x += 1;
                continue;
            }

            // 2. Highlight strings
            if c == b'"' {
                vga::set_color(vga::Color::Yellow, vga::Color::Black);
                let s = [c];
                if let Ok(c_str) = core::str::from_utf8(&s) {
                    vga::print_str(c_str);
                }
                x += 1;
                while x < len {
                    let sc = EDITOR_GRID[y][x];
                    let s_sc = [sc];
                    if let Ok(c_str) = core::str::from_utf8(&s_sc) {
                        vga::print_str(c_str);
                    }
                    x += 1;
                    if sc == b'"' {
                        break;
                    }
                }
                continue;
            }
            if c == b'\'' {
                vga::set_color(vga::Color::Yellow, vga::Color::Black);
                let s = [c];
                if let Ok(c_str) = core::str::from_utf8(&s) {
                    vga::print_str(c_str);
                }
                x += 1;
                while x < len {
                    let sc = EDITOR_GRID[y][x];
                    let s_sc = [sc];
                    if let Ok(c_str) = core::str::from_utf8(&s_sc) {
                        vga::print_str(c_str);
                    }
                    x += 1;
                    if sc == b'\'' {
                        break;
                    }
                }
                continue;
            }

            // 3. Highlight comments
            if c == b'/' && x + 1 < len && EDITOR_GRID[y][x + 1] == b'/' {
                vga::set_color(vga::Color::LightGreen, vga::Color::Black);
                while x < len {
                    let sc = EDITOR_GRID[y][x];
                    let s_sc = [sc];
                    if let Ok(c_str) = core::str::from_utf8(&s_sc) {
                        vga::print_str(c_str);
                    }
                    x += 1;
                }
                continue;
            }

            // 4. Highlight symbols / operators
            if c == b'='
                || c == b'+'
                || c == b'-'
                || c == b'*'
                || c == b'/'
                || c == b'%'
                || c == b'&'
                || c == b'|'
                || c == b'^'
                || c == b'!'
                || c == b'<'
                || c == b'>'
            {
                vga::set_color(vga::Color::LightMagenta, vga::Color::Black);
                let s = [c];
                if let Ok(c_str) = core::str::from_utf8(&s) {
                    vga::print_str(c_str);
                }
                x += 1;
                continue;
            }

            // 5. Highlight words (keywords vs identifier)
            let is_alpha = |b: u8| -> bool { (b >= b'a' && b <= b'z') || (b >= b'A' && b <= b'Z') };
            let is_alnum = |b: u8| -> bool { is_alpha(b) || (b >= b'0' && b <= b'9') || b == b'_' };

            if is_alpha(c) || c == b'_' {
                let start = x;
                while x < len && is_alnum(EDITOR_GRID[y][x]) {
                    x += 1;
                }
                let word_slice = &EDITOR_GRID[y][start..x];
                let is_keyword = match word_slice {
                    b"fn" | b"let" | b"struct" | b"impl" | b"pub" | b"for" | b"if" | b"else"
                    | b"match" | b"return" | b"loop" | b"mut" | b"static" | b"const" | b"use"
                    | b"mod" | b"as" | b"enum" | b"type" | b"true" | b"false" => true,
                    _ => false,
                };

                if is_keyword {
                    vga::set_color(vga::Color::LightBlue, vga::Color::Black);
                } else {
                    vga::set_color(vga::Color::White, vga::Color::Black);
                }

                for &b in word_slice {
                    let s_b = [b];
                    if let Ok(c_str) = core::str::from_utf8(&s_b) {
                        vga::print_str(c_str);
                    }
                }
                continue;
            }

            // Default character
            vga::set_color(vga::Color::White, vga::Color::Black);
            let s = [c];
            if let Ok(c_str) = core::str::from_utf8(&s) {
                vga::print_str(c_str);
            }
            x += 1;
        }

        // Pad rest of line with space
        vga::set_color(vga::Color::White, vga::Color::Black);
        let mut pad = len;
        while pad < 80 {
            vga::print_str(" ");
            pad += 1;
        }
    }

    // 3. Draw bottom bar (Help/Status)
    vga_set_cursor_pos(24, 0);
    if EDITOR_CONFIRM_SAVE {
        vga::set_color(vga::Color::White, vga::Color::DarkGrey);
        vga::print_str("  Save changes? [Y] Yes  [N] No  [C] Cancel");
    } else if EDITOR_STATUS_LEN > 0 {
        vga::set_color(vga::Color::White, vga::Color::DarkGrey);
        vga::print_str("  ");
        let status_slice = &EDITOR_STATUS_MSG[..EDITOR_STATUS_LEN];
        if let Ok(s) = core::str::from_utf8(status_slice) {
            vga::print_str(s);
        }
    } else {
        vga::set_color(vga::Color::White, vga::Color::DarkGrey);
        vga::print_str("  ESC: Exit Prompt  |  Ctrl+S: Quick Save  |  Ctrl+Q: Save & Exit");
    }

    let mut current_col = vga_get_cursor_col();
    while current_col < 79 {
        vga::print_str(" ");
        current_col += 1;
    }

    vga::set_color(vga::Color::LightGrey, vga::Color::Black);

    if EDITOR_CONFIRM_SAVE {
        vga_set_cursor_pos(24, 45);
    } else {
        vga_set_cursor_pos(EDIT_CUR_Y + 1, EDIT_CUR_X);
    }
}

pub unsafe fn editor_handle_keypress(c: u8) {
    if EDITOR_CONFIRM_SAVE {
        match c {
            b'y' | b'Y' => {
                if let Err(e) = editor_save_file() {
                    vga_init();
                    vga::set_color(vga::Color::LightRed, vga::Color::Black);
                    vga::print_str("Error saving file: ");
                    vga::print_str(e);
                    vga::print_str("\nPress any key to return...\n");
                    vga::set_color(vga::Color::LightGrey, vga::Color::Black);
                    EDITOR_CONFIRM_SAVE = false;
                    EDITOR_CONFIRM_EXIT = true;
                    return;
                }
                IN_EDITOR_MODE = false;
                vga_init();
                super::print_prompt();
            }
            b'n' | b'N' => {
                IN_EDITOR_MODE = false;
                vga_init();
                super::print_prompt();
            }
            b'c' | b'C' | 27 => {
                EDITOR_CONFIRM_SAVE = false;
                editor_redraw();
            }
            _ => {}
        }
        return;
    }

    if EDITOR_CONFIRM_EXIT {
        IN_EDITOR_MODE = false;
        vga_init();
        super::print_prompt();
        return;
    }

    // Ctrl+S (19) Quick Save shortcut
    if c == 19 {
        match editor_save_file() {
            Ok(_) => {
                let msg = b"File saved successfully!";
                EDITOR_STATUS_LEN = msg.len();
                EDITOR_STATUS_MSG[..msg.len()].copy_from_slice(msg);
                EDITOR_STATUS_COLOR = vga::Color::LightGreen;
            }
            Err(e) => {
                let mut msg = [0u8; 40];
                let prefix = b"Error: ";
                let mut len = prefix.len();
                msg[..len].copy_from_slice(prefix);
                let e_bytes = e.as_bytes();
                let to_copy = core::cmp::min(e_bytes.len(), 40 - len);
                msg[len..len + to_copy].copy_from_slice(&e_bytes[..to_copy]);
                len += to_copy;
                EDITOR_STATUS_LEN = len;
                EDITOR_STATUS_MSG[..len].copy_from_slice(&msg[..len]);
                EDITOR_STATUS_COLOR = vga::Color::LightRed;
            }
        }
        editor_redraw();
        return;
    }

    // Ctrl+Q (17) Save & Exit shortcut
    if c == 17 {
        if let Err(e) = editor_save_file() {
            vga_init();
            vga::set_color(vga::Color::LightRed, vga::Color::Black);
            vga::print_str("Error saving file: ");
            vga::print_str(e);
            vga::print_str("\nPress any key to return...\n");
            vga::set_color(vga::Color::LightGrey, vga::Color::Black);
            EDITOR_CONFIRM_SAVE = false;
            EDITOR_CONFIRM_EXIT = true;
        } else {
            IN_EDITOR_MODE = false;
            vga_init();
            super::print_prompt();
        }
        return;
    }

    EDITOR_STATUS_LEN = 0;

    match c {
        27 => {
            EDITOR_CONFIRM_SAVE = true;
            editor_redraw();
        }
        9 => {
            // Tab key: Insert 4 spaces
            let y = EDIT_CUR_Y as usize;
            let mut x = EDIT_CUR_X as usize;
            let mut len = LINE_LENS[y] as usize;
            for _ in 0..4 {
                if len < 80 {
                    for i in (x..len).rev() {
                        EDITOR_GRID[y][i + 1] = EDITOR_GRID[y][i];
                    }
                    EDITOR_GRID[y][x] = b' ';
                    LINE_LENS[y] += 1;
                    len += 1;
                    if EDIT_CUR_X < 79 {
                        EDIT_CUR_X += 1;
                        x += 1;
                    }
                }
            }
            editor_redraw();
        }
        10 | 13 => {
            if EDIT_CUR_Y < 22 {
                let cur_y = EDIT_CUR_Y as usize;
                let cur_x = EDIT_CUR_X as usize;
                let cur_len = LINE_LENS[cur_y] as usize;

                for r in (cur_y + 1..22).rev() {
                    EDITOR_GRID[r + 1] = EDITOR_GRID[r];
                    LINE_LENS[r + 1] = LINE_LENS[r];
                }

                let next_y = cur_y + 1;
                EDITOR_GRID[next_y] = [b' '; 80];
                LINE_LENS[next_y] = 0;

                if cur_x < cur_len {
                    let to_move = cur_len - cur_x;
                    for i in 0..to_move {
                        EDITOR_GRID[next_y][i] = EDITOR_GRID[cur_y][cur_x + i];
                        EDITOR_GRID[cur_y][cur_x + i] = b' ';
                    }
                    LINE_LENS[next_y] = to_move as u16;
                }
                LINE_LENS[cur_y] = cur_x as u16;

                EDIT_CUR_Y += 1;
                EDIT_CUR_X = 0;
                editor_redraw();
            }
        }
        8 => {
            let y = EDIT_CUR_Y as usize;
            if EDIT_CUR_X > 0 {
                let x = EDIT_CUR_X as usize;
                let len = LINE_LENS[y] as usize;
                for i in x..len {
                    EDITOR_GRID[y][i - 1] = EDITOR_GRID[y][i];
                }
                if len > 0 {
                    EDITOR_GRID[y][len - 1] = b' ';
                    LINE_LENS[y] -= 1;
                }
                EDIT_CUR_X -= 1;
                editor_redraw();
            } else if EDIT_CUR_Y > 0 {
                let prev_y = (EDIT_CUR_Y - 1) as usize;
                let prev_len = LINE_LENS[prev_y] as usize;
                let cur_len = LINE_LENS[y] as usize;

                let space_left = 80 - prev_len;
                let to_copy = core::cmp::min(cur_len, space_left);

                for i in 0..to_copy {
                    EDITOR_GRID[prev_y][prev_len + i] = EDITOR_GRID[y][i];
                }
                LINE_LENS[prev_y] += to_copy as u16;

                if cur_len <= space_left {
                    for r in y..22 {
                        EDITOR_GRID[r] = EDITOR_GRID[r + 1];
                        LINE_LENS[r] = LINE_LENS[r + 1];
                    }
                    EDITOR_GRID[22] = [b' '; 80];
                    LINE_LENS[22] = 0;

                    EDIT_CUR_Y -= 1;
                    EDIT_CUR_X = prev_len as u16;
                } else {
                    let shifted = to_copy;
                    for i in 0..(cur_len - shifted) {
                        EDITOR_GRID[y][i] = EDITOR_GRID[y][i + shifted];
                    }
                    for i in (cur_len - shifted)..80 {
                        EDITOR_GRID[y][i] = b' ';
                    }
                    LINE_LENS[y] -= shifted as u16;

                    EDIT_CUR_Y -= 1;
                    EDIT_CUR_X = prev_len as u16;
                }
                editor_redraw();
            }
        }
        0x80 => {
            if EDIT_CUR_Y > 0 {
                EDIT_CUR_Y -= 1;
                let len = LINE_LENS[EDIT_CUR_Y as usize];
                if EDIT_CUR_X > len {
                    EDIT_CUR_X = len;
                }
                editor_redraw();
            }
        }
        0x81 => {
            if EDIT_CUR_Y < 22 {
                EDIT_CUR_Y += 1;
                let len = LINE_LENS[EDIT_CUR_Y as usize];
                if EDIT_CUR_X > len {
                    EDIT_CUR_X = len;
                }
                editor_redraw();
            }
        }
        0x82 => {
            // Arrow Left
            if EDIT_CUR_X > 0 {
                EDIT_CUR_X -= 1;
                editor_redraw();
            } else if EDIT_CUR_Y > 0 {
                EDIT_CUR_Y -= 1;
                EDIT_CUR_X = LINE_LENS[EDIT_CUR_Y as usize];
                editor_redraw();
            }
        }
        0x83 => {
            // Arrow Right
            let len = LINE_LENS[EDIT_CUR_Y as usize];
            if EDIT_CUR_X < len {
                EDIT_CUR_X += 1;
                editor_redraw();
            } else if EDIT_CUR_Y < 22 {
                EDIT_CUR_Y += 1;
                EDIT_CUR_X = 0;
                editor_redraw();
            }
        }
        32..=126 => {
            let y = EDIT_CUR_Y as usize;
            let x = EDIT_CUR_X as usize;
            let len = LINE_LENS[y] as usize;

            if len < 80 {
                for i in (x..len).rev() {
                    EDITOR_GRID[y][i + 1] = EDITOR_GRID[y][i];
                }
                EDITOR_GRID[y][x] = c;
                LINE_LENS[y] += 1;

                if EDIT_CUR_X < 79 {
                    EDIT_CUR_X += 1;
                } else if EDIT_CUR_Y < 22 {
                    EDIT_CUR_X = 0;
                    EDIT_CUR_Y += 1;
                }
                editor_redraw();
            }
        }
        _ => {}
    }
}

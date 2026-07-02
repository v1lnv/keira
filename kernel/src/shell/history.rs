//! Keira Kernel: Terminal Command History

use super::state::*;
use crate::io::vga;

extern "C" {
    fn vga_set_cursor_pos(row: u16, col: u16);
    fn vga_clear_line_from(col: u16);
}

pub unsafe fn history_push() {
    if BUFFER_LEN == 0 {
        return;
    }
    let idx = HISTORY_COUNT % HISTORY_SIZE;
    HISTORY[idx] = [0; BUFFER_SIZE];
    for i in 0..BUFFER_LEN {
        HISTORY[idx][i] = INPUT_BUFFER[i];
    }
    HISTORY_LENS[idx] = BUFFER_LEN;
    HISTORY_COUNT += 1;
}

/// Replace current input buffer with history entry and redraw
pub unsafe fn history_load(idx: usize) {
    vga_set_cursor_pos(PROMPT_ROW, PROMPT_COL);
    vga_clear_line_from(PROMPT_COL);

    BUFFER_LEN = HISTORY_LENS[idx];
    for i in 0..BUFFER_LEN {
        INPUT_BUFFER[i] = HISTORY[idx][i];
    }

    let buffer_slice = &INPUT_BUFFER[..BUFFER_LEN];
    if let Ok(s) = core::str::from_utf8(buffer_slice) {
        vga::print_str(s);
    }
}

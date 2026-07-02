//! Keira Kernel: Shell Command 'move'
//!
//! Implementation of the file move/rename command.

use crate::io::vga;
use crate::fs::fat::{read_file_content, create_file, write_file_content, remove_entry};

static mut MOVE_BUFFER: [u8; 65536] = [0; 65536];

pub fn run(parts: &mut core::str::SplitWhitespace) {
    let src = match parts.next() {
        Some(s) => s,
        None => {
            vga::print_str("Usage: move <src_file> <dest_file>\n");
            return;
        }
    };

    let dest = match parts.next() {
        Some(d) => d,
        None => {
            vga::print_str("Usage: move <src_file> <dest_file>\n");
            return;
        }
    };

    unsafe {
        let move_buf = &mut *core::ptr::addr_of_mut!(MOVE_BUFFER);
        // Read source file content into the static move buffer
        match read_file_content(src, move_buf) {
            Ok(size) => {
                // Try to create the destination file (if it already exists, we will overwrite it)
                let _ = create_file(dest);

                match write_file_content(dest, &move_buf[..size]) {
                    Ok(_) => {
                        // Delete the source file after a successful copy
                        match remove_entry(src) {
                            Ok(_) => {
                                vga::print_str("File moved successfully.\n");
                            }
                            Err(e) => {
                                vga::print_str("move warning: Copied successfully, but failed to remove source: ");
                                vga::print_str(e);
                                vga::print_str("\n");
                            }
                        }
                    }
                    Err(e) => {
                        vga::print_str("move error: Failed to write destination: ");
                        vga::print_str(e);
                        vga::print_str("\n");
                    }
                }
            }
            Err(e) => {
                vga::print_str("move error: Failed to read source: ");
                vga::print_str(e);
                vga::print_str("\n");
            }
        }
    }
}

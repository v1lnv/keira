//! Keira Kernel: Shell Command 'hda'
//!
//! Plays sound using the Intel High Definition Audio (HDA) controller.

use crate::io::vga;
use crate::io::hda;

pub fn run(parts: &mut core::str::SplitWhitespace) {
    let action = match parts.next() {
        Some(s) => s,
        None => {
            vga::print_str("Usage: hda <play [freq]|stop|status>\n");
            return;
        }
    };

    match action {
        "status" => {
            unsafe {
                if hda::HDA_INITIALIZED {
                    vga::print_str("Intel HD Audio Controller: Mapped & Initialized (Active)\n");
                } else if hda::HDA_PCI_FOUND {
                    vga::print_str("Intel HD Audio Controller: Found on PCI but failed to initialize\n");
                } else {
                    vga::print_str("Intel HD Audio Controller: Not detected on PCI bus\n");
                }
            }
        }
        "play" => {
            unsafe {
                if !hda::HDA_INITIALIZED {
                    vga::print_str("Error: HDA is not initialized.\n");
                    return;
                }
            }
            let freq = match parts.next() {
                Some(s) => match s.parse::<u32>() {
                    Ok(val) => val,
                    Err(_) => {
                        vga::print_str("Error: Invalid frequency value.\n");
                        return;
                    }
                },
                None => 440, // Default A4 note
            };
            if freq == 0 || freq > 20000 {
                vga::print_str("Error: Frequency must be between 1 and 20000 Hz.\n");
                return;
            }
            vga::print_str("Starting continuous HDA tone at ");
            vga::print_u64(freq as u64);
            vga::print_str(" Hz...\n");
            hda::play_tone(freq);
        }
        "stop" => {
            unsafe {
                if !hda::HDA_INITIALIZED {
                    vga::print_str("Error: HDA is not initialized.\n");
                    return;
                }
            }
            vga::print_str("Stopping HDA audio stream.\n");
            hda::stop();
        }
        _ => {
            vga::print_str("Unknown action. Try: hda play <freq>, hda stop, hda status\n");
        }
    }
}

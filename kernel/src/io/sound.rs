#![allow(dead_code)]
//! Keira Kernel: PC Speaker Sound Driver
//!
//! Provides a safe Rust interface to play frequencies on the PC Speaker (PIT Channel 2).

use core::arch::asm;

extern "C" {
    fn get_uptime_ms() -> u64;
}

unsafe fn outb(port: u16, val: u8) {
    asm!(
        "out dx, al",
        in("dx") port,
        in("al") val,
        options(nomem, nostack, preserves_flags)
    );
}

unsafe fn inb(port: u16) -> u8 {
    let val: u8;
    asm!(
        "in al, dx",
        out("al") val,
        in("dx") port,
        options(nomem, nostack, preserves_flags)
    );
    val
}

/// Plays a beep at the specified frequency (in Hz).
pub fn play_sound(freq: u32) {
    if freq == 0 {
        return;
    }
    
    // PIT frequency is 1.193182 MHz
    let div = 1193182 / freq;
    
    unsafe {
        // Set Channel 2 to mode 3 (square wave generator)
        outb(0x43, 0xB6);
        // Set divisor low byte
        outb(0x42, (div & 0xFF) as u8);
        // Set divisor high byte
        outb(0x42, ((div >> 8) & 0xFF) as u8);
        
        // Turn speaker on
        let tmp = inb(0x61);
        if (tmp & 3) != 3 {
            outb(0x61, tmp | 3);
        }
    }
}

/// Stops playing sound on the PC speaker.
pub fn stop_sound() {
    unsafe {
        let tmp = inb(0x61) & 0xFC;
        outb(0x61, tmp);
    }
}

/// Busy wait sleep helper using CPU hlt instructions.
pub fn sleep_ms(ms: u64) {
    let start = unsafe { get_uptime_ms() };
    while unsafe { get_uptime_ms() } < start + ms {
        unsafe {
            asm!("hlt");
        }
    }
}

/// Plays a note (freq) for duration (ms) followed by a short silence gap.
pub fn play_note(freq: u32, duration_ms: u64) {
    if freq == 0 {
        stop_sound();
        sleep_ms(duration_ms);
    } else {
        play_sound(freq);
        sleep_ms(duration_ms);
        stop_sound();
    }
    // 10ms gap between notes
    sleep_ms(10);
}

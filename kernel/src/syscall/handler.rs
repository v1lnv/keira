//! Keira Kernel: System Call Handler

use crate::io::vga;

extern "C" {
    fn get_uptime_ms() -> u64;
}

/// Central system call dispatcher
/// Maps standard user registers to operations.
#[no_mangle]
pub extern "C" fn syscall_dispatcher(num: u64, arg1: u64, _arg2: u64, _arg3: u64) -> u64 {
    match num {
        // Syscall 1: Print Character
        1 => {
            let c = arg1 as u8;
            let slice = [c];
            if let Ok(s) = core::str::from_utf8(&slice) {
                vga::print_str(s);
            }
            0
        }
        // Syscall 2: Exit User Mode (special return code to trigger ASM exit jump)
        2 => 0xDEADBEEF,
        // Syscall 3: Sleep (busy halt)
        3 => {
            let ms = arg1;
            let start = unsafe { get_uptime_ms() };
            while unsafe { get_uptime_ms() } < start + ms {
                unsafe {
                    core::arch::asm!("hlt");
                }
            }
            0
        }
        // Syscall 4: Get System Uptime in Milliseconds
        4 => {
            unsafe { get_uptime_ms() }
        }
        _ => {
            u64::MAX // Unknown syscall
        }
    }
}

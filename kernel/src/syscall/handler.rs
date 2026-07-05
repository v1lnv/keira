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
        // Syscall 5: Execute User Program (exec)
        5 => {
            let filename_ptr = arg1 as *const u8;
            if filename_ptr.is_null() {
                return u64::MAX;
            }
            
            let mut name_buf = [0u8; 64];
            let mut len = 0;
            unsafe {
                while len < 63 {
                    let c = *filename_ptr.add(len);
                    if c == 0 {
                        break;
                    }
                    name_buf[len] = c;
                    len += 1;
                }
            }
            
            if let Ok(filename_str) = core::str::from_utf8(&name_buf[..len]) {
                unsafe {
                    match crate::fs::elf::run_user_program(filename_str) {
                        Ok(_) => 0,
                        Err(_) => u64::MAX,
                    }
                }
            } else {
                u64::MAX
            }
        }
        _ => {
            u64::MAX // Unknown syscall
        }
    }
}

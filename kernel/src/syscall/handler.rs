//! Keira Kernel: System Call Handler

use crate::io::vga;

extern "C" {
    fn get_uptime_ms() -> u64;
}

/// Central system call dispatcher
/// Maps standard user registers to operations.
#[no_mangle]
pub extern "C" fn syscall_dispatcher(num: u64, arg1: u64, arg2: u64, arg3: u64) -> u64 {
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
        // Syscall 6: Open File
        // Signature: sys_open(path_ptr: *const u8, write_mode: u64) -> fd
        6 => {
            let path_ptr = arg1 as *const u8;
            let write_mode = arg2 != 0;
            if path_ptr.is_null() {
                return u64::MAX;
            }
            let mut path_buf = [0u8; 128];
            let mut len = 0;
            while len < 127 {
                let c = unsafe { *path_ptr.add(len) };
                if c == 0 {
                    break;
                }
                path_buf[len] = c;
                len += 1;
            }
            let path_str = match core::str::from_utf8(&path_buf[..len]) {
                Ok(s) => s,
                Err(_) => return u64::MAX,
            };

            // Check if file exists, if not write_mode and it doesn't exist, error out
            let exists = crate::fs::vfs::exists(path_str);
            if !exists {
                if !write_mode {
                    return u64::MAX;
                }
                // Try to create it
                if crate::fs::vfs::create_file(path_str).is_err() {
                    return u64::MAX;
                }
            }

            unsafe {
                let task = &mut crate::task::scheduler::TASKS[crate::task::scheduler::CURRENT_TASK_IDX];
                if let Some(t) = task {
                    let mut fd_slot = None;
                    for i in 0..8 {
                        if !t.fds[i].is_open {
                            fd_slot = Some(i);
                            break;
                        }
                    }
                    if let Some(fd) = fd_slot {
                        t.fds[fd].is_open = true;
                        t.fds[fd].offset = 0;
                        t.fds[fd].write_mode = write_mode;
                        t.fds[fd].path_len = len;
                        t.fds[fd].path[..len].copy_from_slice(&path_buf[..len]);
                        return fd as u64;
                    }
                }
            }
            u64::MAX
        }
        // Syscall 7: Read File
        // Signature: sys_read(fd: u64, buf_ptr: *mut u8, len: u64) -> bytes_read
        7 => {
            let fd = arg1 as usize;
            let buf_ptr = arg2 as *mut u8;
            let len = arg3;
            if fd >= 8 || buf_ptr.is_null() {
                return u64::MAX;
            }

            unsafe {
                let task = &mut crate::task::scheduler::TASKS[crate::task::scheduler::CURRENT_TASK_IDX];
                if let Some(t) = task {
                    if t.fds[fd].is_open {
                        let path_str = match core::str::from_utf8(&t.fds[fd].path[..t.fds[fd].path_len]) {
                            Ok(s) => s,
                            Err(_) => return u64::MAX,
                        };

                        let mut file_buf = [0u8; 4096];
                        let bytes_read = match crate::fs::vfs::read_file(path_str, &mut file_buf) {
                            Ok(b) => b,
                            Err(_) => return u64::MAX,
                        };

                        let offset = t.fds[fd].offset as usize;
                        if offset >= bytes_read {
                            return 0; // EOF
                        }

                        let to_copy = core::cmp::min(len as usize, bytes_read - offset);
                        for i in 0..to_copy {
                            *buf_ptr.add(i) = file_buf[offset + i];
                        }

                        t.fds[fd].offset += to_copy as u64;
                        return to_copy as u64;
                    }
                }
            }
            u64::MAX
        }
        // Syscall 8: Write File
        // Signature: sys_write(fd: u64, buf_ptr: *const u8, len: u64) -> bytes_written
        8 => {
            let fd = arg1 as usize;
            let buf_ptr = arg2 as *const u8;
            let len = arg3;
            if fd >= 8 || buf_ptr.is_null() {
                return u64::MAX;
            }

            unsafe {
                let task = &mut crate::task::scheduler::TASKS[crate::task::scheduler::CURRENT_TASK_IDX];
                if let Some(t) = task {
                    if t.fds[fd].is_open && t.fds[fd].write_mode {
                        let path_str = match core::str::from_utf8(&t.fds[fd].path[..t.fds[fd].path_len]) {
                            Ok(s) => s,
                            Err(_) => return u64::MAX,
                        };

                        let mut file_buf = [0u8; 4096];
                        let existing_size = crate::fs::vfs::read_file(path_str, &mut file_buf).unwrap_or(0);
                        let offset = t.fds[fd].offset as usize;
                        if offset + (len as usize) > 4096 {
                            return u64::MAX; // Limit to 4KB
                        }

                        for i in 0..(len as usize) {
                            file_buf[offset + i] = *buf_ptr.add(i);
                        }

                        let new_size = core::cmp::max(existing_size, offset + (len as usize));
                        match crate::fs::vfs::write_file(path_str, &file_buf[..new_size]) {
                            Ok(_) => {
                                t.fds[fd].offset += len;
                                return len;
                            }
                            Err(_) => return u64::MAX,
                        }
                    }
                }
            }
            u64::MAX
        }
        // Syscall 9: Close File
        // Signature: sys_close(fd: u64) -> status
        9 => {
            let fd = arg1 as usize;
            if fd >= 8 {
                return u64::MAX;
            }
            unsafe {
                let task = &mut crate::task::scheduler::TASKS[crate::task::scheduler::CURRENT_TASK_IDX];
                if let Some(t) = task {
                    if t.fds[fd].is_open {
                        t.fds[fd].is_open = false;
                        return 0;
                    }
                }
            }
            u64::MAX
        }
        // Syscall 10: Seek File
        // Signature: sys_seek(fd: u64, offset: u64) -> status
        10 => {
            let fd = arg1 as usize;
            let offset = arg2;
            if fd >= 8 {
                return u64::MAX;
            }
            unsafe {
                let task = &mut crate::task::scheduler::TASKS[crate::task::scheduler::CURRENT_TASK_IDX];
                if let Some(t) = task {
                    if t.fds[fd].is_open {
                        t.fds[fd].offset = offset;
                        return 0;
                    }
                }
            }
            u64::MAX
        }
        _ => {
            u64::MAX // Unknown syscall
        }
    }
}

//! Keira Kernel: ELF64 Binary Loader Logic

use super::types::{ElfHeader, ProgramHeader, PT_LOAD};
use crate::mem::{pmm, vmm};

static mut ELF_FILE_BUF: [u8; 65536] = [0u8; 65536];

#[derive(Copy, Clone)]
struct SavedMapping {
    vaddr: u64,
    phys: u64,
}

static mut LOADED_PAGES: [u64; 256] = [0u64; 256];
static mut LOADED_PAGES_COUNT: usize = 0;

/// Load an ELF binary from Routed VFS disk, map pages, and return virtual entry address
pub unsafe fn load_elf(filename: &str) -> Result<u64, &'static str> {
    let file_buf = unsafe { &mut *core::ptr::addr_of_mut!(ELF_FILE_BUF) };
    let file_len = crate::fs::vfs::read_file(filename, file_buf)?;

    if file_len < core::mem::size_of::<ElfHeader>() {
        return Err("File is too small to be a valid ELF");
    }

    let header = &*(file_buf.as_ptr() as *const ElfHeader);

    // Check ELF magic
    if header.ident[0..4] != [0x7F, b'E', b'L', b'F'] {
        return Err("Invalid ELF magic");
    }

    // Verify 64-bit class (ident[4] = 2) and Little Endian (ident[5] = 1)
    if header.ident[4] != 2 || header.ident[5] != 1 {
        return Err("Only 64-bit Little Endian ELF binaries are supported");
    }

    let ph_size = core::mem::size_of::<ProgramHeader>();

    // Iterate program headers
    for i in 0..header.phnum {
        let offset = header.phoff + (i as u64 * header.phentsize as u64);
        if offset + ph_size as u64 > file_len as u64 {
            return Err("Program header out of bounds");
        }

        let ph = &*(file_buf.as_ptr().add(offset as usize) as *const ProgramHeader);

        if ph.p_type == PT_LOAD {
            // Map the segment pages
            let start_vaddr = ph.p_vaddr;
            let mem_size = ph.p_memsz;

            // Align start_vaddr to page boundary (4KB)
            let page_offset = start_vaddr % pmm::PAGE_SIZE;
            let aligned_start = start_vaddr - page_offset;
            let total_size = mem_size + page_offset;

            let mut offset_in_segment = 0u64;

            while offset_in_segment < total_size {
                let vaddr = aligned_start + offset_in_segment;

                // Allocate a physical page frame
                let frame = pmm::alloc_frame().ok_or("Out of memory during ELF loading")?;

                // Map page to frame
                vmm::map_page(vaddr, frame, vmm::PAGE_USER | vmm::PAGE_WRITABLE)?;

                if LOADED_PAGES_COUNT < 256 {
                    LOADED_PAGES[LOADED_PAGES_COUNT] = vaddr;
                    LOADED_PAGES_COUNT += 1;
                }

                // Clear the frame contents to zero (BSS)
                let frame_ptr = vaddr as *mut u8;
                core::ptr::write_bytes(frame_ptr, 0, pmm::PAGE_SIZE as usize);

                // Copy filesz bytes from file buffer
                let mut page_offset_in_data = 0;
                let mut data_len_to_copy = pmm::PAGE_SIZE;

                if offset_in_segment == 0 {
                    page_offset_in_data = page_offset;
                    data_len_to_copy = pmm::PAGE_SIZE - page_offset;
                }

                let segment_data_offset = if offset_in_segment == 0 {
                    0
                } else {
                    offset_in_segment - page_offset
                };

                if segment_data_offset < ph.p_filesz {
                    let mut bytes_left = ph.p_filesz - segment_data_offset;
                    if bytes_left > data_len_to_copy {
                        bytes_left = data_len_to_copy;
                    }

                    let src_ptr = file_buf
                        .as_ptr()
                        .add((ph.p_offset + segment_data_offset) as usize);
                    let dst_ptr = frame_ptr.add(page_offset_in_data as usize);
                    core::ptr::copy_nonoverlapping(src_ptr, dst_ptr, bytes_left as usize);
                }

                offset_in_segment += pmm::PAGE_SIZE;
            }
        }
    }

    Ok(header.entry)
}

/// Load and execute a freestanding user mode ELF program
pub unsafe fn run_user_program(filename: &str) -> Result<(), &'static str> {
    // 0. Save and temporarily unmap the parent's active memory mappings
    let mut saved_mappings = [SavedMapping { vaddr: 0, phys: 0 }; 128];
    let mut saved_count = 0;

    // Save parent's heap boundaries
    let mut old_break = 0;
    let mut old_break_start = 0;
    let task = &mut crate::task::scheduler::TASKS[crate::task::scheduler::CURRENT_TASK_IDX];
    if let Some(t) = task {
        old_break = t.program_break;
        old_break_start = t.program_break_start;
    }

    // Save and unmap code/data pages of parent (from 0x40000000 to 0x40030000 - 192KB)
    let mut addr = 0x40000000;
    while addr < 0x40030000 {
        if let Some(phys) = vmm::get_phys_addr(addr) {
            if saved_count < 128 {
                saved_mappings[saved_count] = SavedMapping { vaddr: addr, phys };
                saved_count += 1;
                let _ = vmm::unmap_page(addr);
            }
        }
        addr += pmm::PAGE_SIZE;
    }

    // Save and unmap heap pages of parent (from 0x600000000000 to 0x600000030000 - 192KB)
    let mut addr = 0x600000000000;
    while addr < 0x600000030000 {
        if let Some(phys) = vmm::get_phys_addr(addr) {
            if saved_count < 128 {
                saved_mappings[saved_count] = SavedMapping { vaddr: addr, phys };
                saved_count += 1;
                let _ = vmm::unmap_page(addr);
            }
        }
        addr += pmm::PAGE_SIZE;
    }

    // Save and unmap parent's stack page
    let stack_vaddr = 0x7FFFFFFF0000;
    if let Some(phys) = vmm::get_phys_addr(stack_vaddr) {
        if saved_count < 128 {
            saved_mappings[saved_count] = SavedMapping { vaddr: stack_vaddr, phys };
            saved_count += 1;
            let _ = vmm::unmap_page(stack_vaddr);
        }
    }

    // Track the start offset of the child's loaded pages
    let saved_pages_start = LOADED_PAGES_COUNT;

    // 1. Load the ELF binary and get entry point
    let entry_point = load_elf(filename)?;

    // 2. Allocate a physical page frame for the user space stack
    let stack_frame = pmm::alloc_frame().ok_or("Out of memory for user stack frame")?;
    let user_stack_vaddr = 0x7FFFFFFF0000;

    // 3. Map user stack frame with User and Writable permissions
    vmm::map_page(
        user_stack_vaddr,
        stack_frame,
        vmm::PAGE_USER | vmm::PAGE_WRITABLE | vmm::PAGE_PRESENT,
    )?;

    // Clear user stack frame to zero
    let stack_ptr = user_stack_vaddr as *mut u64;
    for i in 0..512 {
        *stack_ptr.add(i) = 0;
    }

    // 3.5. Initialize heap boundaries for the user process
    let task = &mut crate::task::scheduler::TASKS[crate::task::scheduler::CURRENT_TASK_IDX];
    if let Some(t) = task {
        t.program_break = 0x600000000000;
        t.program_break_start = 0x600000000000;
    }

    // 4. Perform Ring 3 transition using assembly routine
    extern "C" {
        fn jump_to_user(entry: u64, stack_top: u64);
        static mut kernel_stack_temp: u64;
    }

    let old_kernel_stack = kernel_stack_temp;

    // Pass the top of the stack (grows down, so vaddr + PAGE_SIZE)
    jump_to_user(entry_point, user_stack_vaddr + pmm::PAGE_SIZE);

    // Re-enable interrupts in the kernel since the syscall exit path left them disabled
    core::arch::asm!("sti");

    // 4.5. Clean up child's heap memory
    let mut end_break = 0x600000000000;
    let task = &mut crate::task::scheduler::TASKS[crate::task::scheduler::CURRENT_TASK_IDX];
    if let Some(t) = task {
        end_break = t.program_break;
    }
    let mut addr = 0x600000000000;
    while addr < end_break {
        let _ = vmm::free_and_unmap_page(addr);
        addr += pmm::PAGE_SIZE;
    }

    // 5. Clean up child's ELF segment pages using recorded LOADED_PAGES
    for i in saved_pages_start..LOADED_PAGES_COUNT {
        let page_vaddr = LOADED_PAGES[i];
        if page_vaddr != 0 {
            let _ = vmm::free_and_unmap_page(page_vaddr);
        }
    }
    LOADED_PAGES_COUNT = saved_pages_start;

    // 6. Clean up child's stack frame
    let _ = vmm::unmap_page(user_stack_vaddr);
    pmm::free_frame(stack_frame);

    // 7. Restore parent's heap boundaries
    let task = &mut crate::task::scheduler::TASKS[crate::task::scheduler::CURRENT_TASK_IDX];
    if let Some(t) = task {
        t.program_break = old_break;
        t.program_break_start = old_break_start;
    }

    // 8. Restore parent's page mappings
    for i in 0..saved_count {
        let mapping = &saved_mappings[i];
        if mapping.vaddr != 0 {
            let _ = vmm::map_page(
                mapping.vaddr,
                mapping.phys,
                vmm::PAGE_USER | vmm::PAGE_WRITABLE | vmm::PAGE_PRESENT,
            );
        }
    }

    // 9. Restore the kernel stack pointer for the parent process
    kernel_stack_temp = old_kernel_stack;

    Ok(())
}

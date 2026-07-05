//! Keira Kernel: Virtual Memory Manager (VMM)
//!
//! Manages 4-level page tables (PML4, PDPT, PD, PT) on x86_64, enabling dynamic
//! mapping and unmapping of virtual addresses to physical pages.

use crate::mem::pmm;

// Page Table Entry flags
pub const PAGE_PRESENT: u64 = 1 << 0;
pub const PAGE_WRITABLE: u64 = 1 << 1;
pub const PAGE_USER: u64 = 1 << 2; // User mode access allowed
pub const PAGE_NO_EXECUTE: u64 = 1 << 63;

/// Get the physical address of the active PML4 table from the CR3 register
pub unsafe fn active_pml4() -> u64 {
    let cr3: u64;
    core::arch::asm!("mov {}, cr3", out(reg) cr3);
    cr3 & !0xFFF // Clear control flags in the lower 12 bits
}

/// Map a virtual page to a physical frame with specified flags
pub unsafe fn map_page(
    virtual_addr: u64,
    physical_addr: u64,
    flags: u64,
) -> Result<(), &'static str> {
    if virtual_addr % pmm::PAGE_SIZE != 0 || physical_addr % pmm::PAGE_SIZE != 0 {
        return Err("Virtual or Physical address is not page-aligned");
    }

    // Indices for the 4 levels of page tables
    let pml4_idx = ((virtual_addr >> 39) & 0x1FF) as usize;
    let pdpt_idx = ((virtual_addr >> 30) & 0x1FF) as usize;
    let pd_idx = ((virtual_addr >> 21) & 0x1FF) as usize;
    let pt_idx = ((virtual_addr >> 12) & 0x1FF) as usize;

    let pml4_addr = active_pml4();
    let pml4 = pml4_addr as *mut u64;

    // 1. Traverse / Allocate PDPT
    let pdpt_entry = *pml4.add(pml4_idx);
    let pdpt_addr = if (pdpt_entry & PAGE_PRESENT) == 0 {
        let frame = pmm::alloc_frame().ok_or("Out of physical memory for PDPT")?;
        *pml4.add(pml4_idx) = frame | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
        frame
    } else {
        pdpt_entry & !0xFFF
    };
    let pdpt = pdpt_addr as *mut u64;

    // 2. Traverse / Allocate PD
    let pd_entry = *pdpt.add(pdpt_idx);
    let pd_addr = if (pd_entry & PAGE_PRESENT) == 0 {
        let frame = pmm::alloc_frame().ok_or("Out of physical memory for PD")?;
        *pdpt.add(pdpt_idx) = frame | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
        frame
    } else {
        pd_entry & !0xFFF
    };
    let pd = pd_addr as *mut u64;

    // 3. Traverse / Allocate PT
    let pt_entry = *pd.add(pd_idx);
    let pt_addr = if (pt_entry & PAGE_PRESENT) == 0 {
        let frame = pmm::alloc_frame().ok_or("Out of physical memory for PT")?;
        *pd.add(pd_idx) = frame | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
        frame
    } else {
        pt_entry & !0xFFF
    };
    let pt = pt_addr as *mut u64;

    // 4. Set PT entry
    *pt.add(pt_idx) = physical_addr | flags | PAGE_PRESENT;

    // 5. Invalidate page in TLB
    core::arch::asm!("invlpg [{}]", in(reg) virtual_addr);

    Ok(())
}

/// Unmap a virtual page
pub unsafe fn unmap_page(virtual_addr: u64) -> Result<(), &'static str> {
    if virtual_addr % pmm::PAGE_SIZE != 0 {
        return Err("Virtual address is not page-aligned");
    }

    let pml4_idx = ((virtual_addr >> 39) & 0x1FF) as usize;
    let pdpt_idx = ((virtual_addr >> 30) & 0x1FF) as usize;
    let pd_idx = ((virtual_addr >> 21) & 0x1FF) as usize;
    let pt_idx = ((virtual_addr >> 12) & 0x1FF) as usize;

    let pml4_addr = active_pml4();
    let pml4 = pml4_addr as *mut u64;

    let pdpt_entry = *pml4.add(pml4_idx);
    if (pdpt_entry & PAGE_PRESENT) == 0 {
        return Err("Page not mapped (PDPT missing)");
    }
    let pdpt = (pdpt_entry & !0xFFF) as *mut u64;

    let pd_entry = *pdpt.add(pdpt_idx);
    if (pd_entry & PAGE_PRESENT) == 0 {
        return Err("Page not mapped (PD missing)");
    }
    let pd = (pd_entry & !0xFFF) as *mut u64;

    let pt_entry = *pd.add(pd_idx);
    if (pt_entry & PAGE_PRESENT) == 0 {
        return Err("Page not mapped (PT missing)");
    }
    let pt = (pt_entry & !0xFFF) as *mut u64;

    let entry = *pt.add(pt_idx);
    if (entry & PAGE_PRESENT) == 0 {
        return Err("Page not mapped");
    }

    // Free the entry
    *pt.add(pt_idx) = 0;

    // Invalidate in TLB
    core::arch::asm!("invlpg [{}]", in(reg) virtual_addr);

    Ok(())
}

/// Translate a virtual address to its corresponding physical address
pub unsafe fn get_phys_addr(virtual_addr: u64) -> Option<u64> {
    let pml4_idx = ((virtual_addr >> 39) & 0x1FF) as usize;
    let pdpt_idx = ((virtual_addr >> 30) & 0x1FF) as usize;
    let pd_idx = ((virtual_addr >> 21) & 0x1FF) as usize;
    let pt_idx = ((virtual_addr >> 12) & 0x1FF) as usize;

    let pml4 = active_pml4() as *const u64;
    let pdpt_entry = *pml4.add(pml4_idx);
    if (pdpt_entry & PAGE_PRESENT) == 0 {
        return None;
    }

    let pdpt = (pdpt_entry & !0xFFF) as *const u64;
    let pd_entry = *pdpt.add(pdpt_idx);
    if (pd_entry & PAGE_PRESENT) == 0 {
        return None;
    }

    let pd = (pd_entry & !0xFFF) as *const u64;
    let pt_entry = *pd.add(pd_idx);
    if (pt_entry & PAGE_PRESENT) == 0 {
        return None;
    }

    let pt = (pt_entry & !0xFFF) as *const u64;
    let entry = *pt.add(pt_idx);
    if (entry & PAGE_PRESENT) == 0 {
        return None;
    }

    Some((entry & !0xFFF) | (virtual_addr & 0xFFF))
}

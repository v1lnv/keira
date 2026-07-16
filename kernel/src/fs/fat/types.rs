//! Keira Kernel: FAT16 Struct and Type Definitions

#[repr(C, packed)]
#[derive(Copy, Clone)]
pub struct DirectoryEntry {
    pub name: [u8; 11],
    pub attr: u8,
    pub nt_res: u8,
    pub crt_time_tenth: u8,
    pub crt_time: u16,
    pub crt_date: u16,
    pub lst_acc_date: u16,
    pub first_cluster_hi: u16,
    pub wrt_time: u16,
    pub wrt_date: u16,
    pub first_cluster_lo: u16,
    pub file_size: u32,
}

#[repr(C, packed)]
#[derive(Copy, Clone)]
pub struct LfnEntry {
    pub sequence: u8,
    pub name_part1: [u16; 5],
    pub attr: u8, // Always 0x0F
    pub lfn_type: u8, // Always 0
    pub checksum: u8,
    pub name_part2: [u16; 6],
    pub first_cluster: u16, // Always 0
    pub name_part3: [u16; 2],
}

pub struct LfnAccumulator {
    pub chars: [u16; 260],
    pub active: bool,
    pub max_index: usize,
}

impl LfnAccumulator {
    pub const fn new() -> Self {
        Self {
            chars: [0u16; 260],
            active: false,
            max_index: 0,
        }
    }

    pub fn reset(&mut self) {
        self.chars = [0u16; 260];
        self.active = false;
        self.max_index = 0;
    }
}

pub struct Fat16Volume {
    pub bytes_per_sector: u16,
    pub sectors_per_cluster: u8,
    pub reserved_sector_count: u16,
    pub num_fats: u8,
    pub root_entry_count: u16,
    pub fat_start_sector: u32,
    pub sectors_per_fat: u16,
    pub root_dir_start_sector: u32,
    pub root_dir_size_sectors: u32,
    pub data_start_sector: u32,
}

pub struct FoundEntry {
    pub sector: u32,
    pub index: usize,
    pub entry: DirectoryEntry,
}

//! Keira Kernel: FAT16 Filesystem Module Root

pub mod cluster;
pub mod dir;
pub mod file;
pub mod path;
pub mod types;
pub mod volume;

pub use dir::{
    change_directory, create_dir, find_matches, get_dir_cluster, list_files, list_files_in_dir,
};
pub use file::{cat_file, create_file, read_file_content, remove_entry, write_file_content};
pub use path::{filename_to_8_3, find_entry, format_filename, resolve_path};
pub use types::{DirectoryEntry, Fat16Volume, FoundEntry};
pub use volume::{init, print_disk_info};

pub static mut VOLUME: Option<types::Fat16Volume> = None;
pub static mut CURRENT_DIR_CLUSTER: u16 = 0; // 0 = Root Directory

/// Local helper to read a sector from the currently mounted block device
pub unsafe fn read_sector(sector: u32, buffer: &mut [u8; 512]) -> Result<(), &'static str> {
    if let Some(dev) = crate::io::block::get_mounted_device() {
        dev.read_sector(sector, buffer)
    } else {
        Err("FAT16 Error: No block device mounted")
    }
}

/// Local helper to write a sector to the currently mounted block device
pub unsafe fn write_sector(sector: u32, buffer: &[u8; 512]) -> Result<(), &'static str> {
    if let Some(dev) = crate::io::block::get_mounted_device() {
        dev.write_sector(sector, buffer)
    } else {
        Err("FAT16 Error: No block device mounted")
    }
}

//! Keira Kernel: FAT16 Path Resolution and Entry Lookup

use super::dir::for_each_directory_sector;
use super::read_sector;
use super::types::{DirectoryEntry, FoundEntry, LfnEntry, LfnAccumulator};

/// Accumulates unicode character parts from an LFN entry into the accumulator
pub unsafe fn accumulate_lfn(entry: &DirectoryEntry, accum: &mut LfnAccumulator) {
    let lfn = &*(entry as *const DirectoryEntry as *const LfnEntry);
    let seq = lfn.sequence;
    let index = (seq & 0x1F) as usize;
    if index == 0 || index > 20 {
        return;
    }
    
    let char_offset = (index - 1) * 13;
    
    // Copy part 1 (5 chars)
    for i in 0..5 {
        accum.chars[char_offset + i] = lfn.name_part1[i];
    }
    // Copy part 2 (6 chars)
    for i in 0..6 {
        accum.chars[char_offset + 5 + i] = lfn.name_part2[i];
    }
    // Copy part 3 (2 chars)
    for i in 0..2 {
        accum.chars[char_offset + 11 + i] = lfn.name_part3[i];
    }
    
    accum.active = true;
    if index > accum.max_index {
        accum.max_index = index;
    }
}

/// Converts accumulated UTF-16 code units into a UTF-8 string buffer
pub fn get_lfn_utf8(accum: &LfnAccumulator, buf: &mut [u8]) -> Option<usize> {
    if !accum.active || accum.max_index == 0 {
        return None;
    }
    
    let total_chars = accum.max_index * 13;
    let mut utf8_len = 0;
    
    for i in 0..total_chars {
        let c = accum.chars[i];
        if c == 0x0000 || c == 0xFFFF {
            break;
        }
        
        if c < 0x80 {
            if utf8_len < buf.len() {
                buf[utf8_len] = c as u8;
                utf8_len += 1;
            }
        } else if c < 0x800 {
            if utf8_len + 1 < buf.len() {
                buf[utf8_len] = (0xC0 | (c >> 6)) as u8;
                buf[utf8_len + 1] = (0x80 | (c & 0x3F)) as u8;
                utf8_len += 2;
            }
        } else if utf8_len + 2 < buf.len() {
            buf[utf8_len] = (0xE0 | (c >> 12)) as u8;
            buf[utf8_len + 1] = (0x80 | ((c >> 6) & 0x3F)) as u8;
            buf[utf8_len + 2] = (0x80 | (c & 0x3F)) as u8;
            utf8_len += 3;
        }
    }
    
    if utf8_len > 0 {
        Some(utf8_len)
    } else {
        None
    }
}
use super::{CURRENT_DIR_CLUSTER, VOLUME};

/// Helper to format 8.3 FAT filename to standard string
pub fn format_filename(name: &[u8; 11], dest: &mut [u8; 12]) -> usize {
    let mut len = 0;

    // Copy base name (strip trailing spaces)
    let mut base_end = 8;
    while base_end > 0 && name[base_end - 1] == b' ' {
        base_end -= 1;
    }

    for i in 0..base_end {
        dest[len] = name[i].to_ascii_lowercase();
        len += 1;
    }

    // Copy extension (if not empty)
    let mut ext_end = 3;
    while ext_end > 0 && name[8 + ext_end - 1] == b' ' {
        ext_end -= 1;
    }

    if ext_end > 0 {
        dest[len] = b'.';
        len += 1;
        for i in 0..ext_end {
            dest[len] = name[8 + i].to_ascii_lowercase();
            len += 1;
        }
    }

    len
}

/// Helper: Validate and convert filename string to 11-byte 8.3 FAT representation
pub fn filename_to_8_3(input: &str) -> Result<[u8; 11], &'static str> {
    let mut name_bytes = [b' '; 11];
    let mut parts = input.split('.');

    let base = parts.next().ok_or("Invalid filename")?;
    let ext = parts.next();
    if parts.next().is_some() {
        return Err("Filename cannot have multiple extensions");
    }

    if base.is_empty() || base.len() > 8 {
        return Err("Filename base must be between 1 and 8 characters");
    }

    for (i, &b) in base.as_bytes().iter().enumerate() {
        let upper = b.to_ascii_uppercase();
        if !upper.is_ascii_alphanumeric() && upper != b'_' && upper != b'-' {
            return Err("Filename contains invalid characters");
        }
        name_bytes[i] = upper;
    }

    if let Some(e) = ext {
        if e.is_empty() || e.len() > 3 {
            return Err("Extension must be between 1 and 3 characters");
        }
        for (i, &b) in e.as_bytes().iter().enumerate() {
            let upper = b.to_ascii_uppercase();
            if !upper.is_ascii_alphanumeric() && upper != b'_' && upper != b'-' {
                return Err("Extension contains invalid characters");
            }
            name_bytes[8 + i] = upper;
        }
    }

    Ok(name_bytes)
}

/// Resolve a nested path to its parent directory cluster and filename.
/// Supports both absolute (e.g. "/apps/bin/file.txt") and relative (e.g. "bin/file.txt") paths.
pub unsafe fn resolve_path(path: &str) -> Result<(u16, &str), &'static str> {
    let vol_ptr = &raw const VOLUME;
    let _vol = (*vol_ptr).as_ref().ok_or("FAT16: Volume not initialized")?;

    let mut current_cluster = if path.starts_with('/') {
        0
    } else {
        CURRENT_DIR_CLUSTER
    };

    let mut path_trimmed = path;
    if path_trimmed.starts_with('/') {
        path_trimmed = &path_trimmed[1..];
    }
    if path_trimmed.ends_with('/') {
        path_trimmed = &path_trimmed[..path_trimmed.len() - 1];
    }

    if path_trimmed.is_empty() {
        return Ok((0, ""));
    }

    let mut segments = path_trimmed.split('/');
    let mut current_segment = segments.next().ok_or("Invalid empty path")?;

    for next_segment in segments {
        let found = find_entry(current_segment, current_cluster)?;
        if (found.entry.attr & 0x10) == 0 {
            return Err("Path segment is not a directory");
        }
        current_cluster = found.entry.first_cluster_lo;
        current_segment = next_segment;
    }

    Ok((current_cluster, current_segment))
}

pub unsafe fn find_entry(filename: &str, dir_cluster: u16) -> Result<FoundEntry, &'static str> {
    let mut sector_data = [0u8; 512];
    let mut found: Option<FoundEntry> = None;
    let mut lfn_accum = LfnAccumulator::new();

    for_each_directory_sector(dir_cluster, |sector| {
        read_sector(sector, &mut sector_data)?;
        let entries = sector_data.as_ptr() as *const DirectoryEntry;
        for i in 0..16 {
            let entry = &*entries.add(i);
            if entry.name[0] == 0x00 {
                lfn_accum.reset();
                return Ok(false);
            }
            if entry.name[0] == 0xE5 {
                lfn_accum.reset();
                continue;
            }
            if (entry.attr & 0x0F) == 0x0F {
                accumulate_lfn(entry, &mut lfn_accum);
                continue;
            }
            if (entry.attr & 0x08) != 0 {
                lfn_accum.reset();
                continue;
            }

            let mut matched = false;
            let mut lfn_buf = [0u8; 260];
            if let Some(lfn_len) = get_lfn_utf8(&lfn_accum, &mut lfn_buf) {
                if let Ok(lfn_str) = core::str::from_utf8(&lfn_buf[..lfn_len]) {
                    if lfn_str.eq_ignore_ascii_case(filename) {
                        matched = true;
                    }
                }
            }

            if !matched {
                let mut name_buf = [0u8; 12];
                let name_len = format_filename(&entry.name, &mut name_buf);
                if let Ok(name_str) = core::str::from_utf8(&name_buf[..name_len]) {
                    if name_str.eq_ignore_ascii_case(filename) {
                        matched = true;
                    }
                }
            }

            lfn_accum.reset();

            if matched {
                found = Some(FoundEntry {
                    sector,
                    index: i,
                    entry: DirectoryEntry {
                        name: entry.name,
                        attr: entry.attr,
                        nt_res: entry.nt_res,
                        crt_time_tenth: entry.crt_time_tenth,
                        crt_time: entry.crt_time,
                        crt_date: entry.crt_date,
                        lst_acc_date: entry.lst_acc_date,
                        first_cluster_hi: entry.first_cluster_hi,
                        wrt_time: entry.wrt_time,
                        wrt_date: entry.wrt_date,
                        first_cluster_lo: entry.first_cluster_lo,
                        file_size: entry.file_size,
                    },
                });
                return Ok(false);
            }
        }
        Ok(true)
    })?;

    found.ok_or("File not found")
}

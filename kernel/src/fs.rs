//! Keira Kernel: Filesystem Module Root
//!
//! Provides the primary file system interfaces for Keira Kernel, including
//! ELF loader, FAT16 filesystem support, and TAR RAM disk parsing.

pub mod elf;
pub mod fat;
pub mod lock;
pub mod tar;
pub mod vfs;

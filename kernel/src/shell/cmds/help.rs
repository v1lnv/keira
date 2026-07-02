//! Keira Kernel: Shell Command 'help'
//!
//! Simply routes to the 'guide' command for a friendly user experience.

pub fn run(parts: &mut core::str::SplitWhitespace) {
    super::guide::run(parts);
}

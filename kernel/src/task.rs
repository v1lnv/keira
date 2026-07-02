//! Keira Kernel: Task Management Module Root
//!
//! Provides support for task management, process representation, context switching,
//! and cooperative multitasking scheduling.

pub mod scheduler;
pub mod types;

pub use scheduler::{exit_current, init, list_tasks, spawn};
pub use types::{InterruptContext, Task, TaskState};

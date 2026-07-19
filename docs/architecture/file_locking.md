# VFS File Locking (v0.13.0)

This module describes how the Keira Kernel coordinates concurrent write access to VFS file paths using a lightweight locking table.

---

## 1. Design Overview

To prevent race conditions, write conflicts, or data corruption when multiple preemptive tasks open or write to the same file, Keira v0.13.0 implements an exclusive write-locking mechanism.

- **Exclusive Locking**: Only one task can hold a write lock on a specific file path at a time.
- **Reentrancy**: If a task has already acquired a write lock on a file, subsequent write-mode opens on the same path by the *same* task will succeed.
- **Auto-Release**: To prevent resource locks from leaking when a process crashes, terminates, or exits normally, the scheduler automatically releases all write locks held by that task.

---

## 2. Lock Table Structure

The lock table is defined in `kernel/src/fs/lock.rs` using a static array:

```rust
pub const MAX_FILE_LOCKS: usize = 16;

#[derive(Clone, Copy)]
pub struct FileLock {
    pub is_locked: bool,
    pub path: [u8; 128],
    pub path_len: usize,
    pub holder_task_id: usize,
}

pub static mut FILE_LOCKS: [FileLock; MAX_FILE_LOCKS] = [FileLock::new(); MAX_FILE_LOCKS];
```

---

## 3. Execution Flow and API

The kernel interfaces with the file locking module during key system calls:

### 1. File Open (`sys_open` - Syscall 6)
When a task opens a file with `write_mode` set to `1` (true):
- The VFS calls `acquire_lock(path, task_id)`.
- If the file is not locked or is locked by the same task, it occupies a slot and returns `Ok(())`, allowing the file descriptor to open.
- If the file is locked by a different task, `acquire_lock` returns `Err`. The open syscall immediately fails and returns `u64::MAX`.

### 2. File Close (`sys_close` - Syscall 9)
When a task closes a file descriptor:
- If the file descriptor was opened in write mode, the VFS calls `release_lock(path, task_id)`.
- The matching slot in the lock table is marked as unlocked (`is_locked = false`), allowing other processes to open it for writing.

### 3. Task Exit (`sys_exit` - Syscall 2 / Termination)
When a task exits or gets killed:
- Inside the scheduler's tick handler (`schedule_tick`), before deallocating the task structure, the kernel invokes:
  `crate::fs::lock::release_all_locks_for_task(prev_task.id);`
- This ensures all lock table slots held by the terminating task are cleared immediately.

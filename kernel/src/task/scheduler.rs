//! Keira Kernel: Multitasking Scheduler Logic

use super::types::{InterruptContext, Task, TaskState};
use crate::io::serial;
use crate::io::vga;
use crate::mem::pmm;

pub const MAX_TASKS: usize = 8;

pub static mut TASKS: [Option<Task>; MAX_TASKS] = [None, None, None, None, None, None, None, None];
pub static mut CURRENT_TASK_IDX: usize = 0;
pub static mut SCHEDULER_INITIALIZED: bool = false;

/// Initialize the scheduler and register the bootstrap thread as Task 0
pub unsafe fn init() {
    let main_task = Task {
        id: 0,
        name: "kernel_shell",
        rsp: 0,
        stack_addr: 0,
        state: TaskState::Running,
    };
    TASKS[0] = Some(main_task);
    CURRENT_TASK_IDX = 0;
    SCHEDULER_INITIALIZED = true;
}

/// Spawn a new kernel thread
pub unsafe fn spawn(name: &'static str, entry_point: fn()) -> Result<usize, &'static str> {
    let mut slot = None;
    for i in 0..MAX_TASKS {
        if TASKS[i].is_none() {
            slot = Some(i);
            break;
        }
    }

    let slot_idx = slot.ok_or("Scheduler: Maximum task limit reached")?;

    let stack_frame = pmm::alloc_frame().ok_or("Scheduler: Out of memory for task stack")?;
    let stack_top = stack_frame + pmm::PAGE_SIZE;

    let context_ptr =
        (stack_top - core::mem::size_of::<InterruptContext>() as u64) as *mut InterruptContext;

    (*context_ptr).r15 = 0;
    (*context_ptr).r14 = 0;
    (*context_ptr).r13 = 0;
    (*context_ptr).r12 = 0;
    (*context_ptr).r11 = 0;
    (*context_ptr).r10 = 0;
    (*context_ptr).r9 = 0;
    (*context_ptr).r8 = 0;
    (*context_ptr).rdi = 0;
    (*context_ptr).rsi = 0;
    (*context_ptr).rbp = 0;
    (*context_ptr).rbx = 0;
    (*context_ptr).rdx = 0;
    (*context_ptr).rcx = 0;
    (*context_ptr).rax = 0;

    (*context_ptr).rip = entry_point as u64;
    (*context_ptr).cs = 0x08;
    (*context_ptr).rflags = 0x202;
    (*context_ptr).rsp = stack_top;
    (*context_ptr).ss = 0x10;

    let new_task = Task {
        id: slot_idx,
        name,
        rsp: context_ptr as u64,
        stack_addr: stack_frame,
        state: TaskState::Ready,
    };

    TASKS[slot_idx] = Some(new_task);

    serial::print_str("Scheduler: Spawned task '");
    serial::print_str(name);
    serial::print_str("' in slot ");
    print_decimal(slot_idx as u64);
    serial::print_str("\n");

    Ok(slot_idx)
}

/// Terminate the currently running task
pub unsafe fn exit_current() {
    core::arch::asm!("cli");
    let idx = CURRENT_TASK_IDX;
    if idx != 0 {
        if let Some(ref mut task) = TASKS[idx] {
            task.state = TaskState::Terminated;
            serial::print_str("Scheduler: Task '");
            serial::print_str(task.name);
            serial::print_str("' exited\n");
        }

        core::arch::asm!("sti");
        loop {
            core::arch::asm!("hlt");
        }
    } else {
        core::arch::asm!("sti");
    }
}

/// Preemptive scheduler tick called from PIT timer interrupt
#[no_mangle]
pub unsafe extern "C" fn schedule_tick(current_rsp: u64) -> u64 {
    if !SCHEDULER_INITIALIZED {
        return current_rsp;
    }

    let current_idx = CURRENT_TASK_IDX;
    
    // Check if the current task was terminated
    let mut current_terminated = false;
    if let Some(ref mut task) = TASKS[current_idx] {
        if task.state == TaskState::Terminated {
            current_terminated = true;
        } else if task.state == TaskState::Running {
            task.rsp = current_rsp;
            task.state = TaskState::Ready;
        }
    }

    let mut next_idx = current_idx;
    loop {
        next_idx = (next_idx + 1) % MAX_TASKS;
        if let Some(ref mut task) = TASKS[next_idx] {
            if task.state == TaskState::Ready {
                task.state = TaskState::Running;
                CURRENT_TASK_IDX = next_idx;
                
                // Safe deallocation: free the stack of the terminated task *after* switching away
                if current_terminated {
                    if let Some(ref prev_task) = TASKS[current_idx] {
                        pmm::free_frame(prev_task.stack_addr);
                    }
                    TASKS[current_idx] = None;
                }
                
                return task.rsp;
            }
        }
        if next_idx == current_idx {
            break;
        }
    }

    // Fallback to kernel shell if the current task exited and no other task is ready
    if current_terminated {
        if let Some(ref mut main_task) = TASKS[0] {
            main_task.state = TaskState::Running;
            CURRENT_TASK_IDX = 0;
            
            if let Some(ref prev_task) = TASKS[current_idx] {
                pmm::free_frame(prev_task.stack_addr);
            }
            if current_idx != 0 {
                TASKS[current_idx] = None;
            }
            return main_task.rsp;
        }
    }

    if let Some(ref mut task) = TASKS[current_idx] {
        task.state = TaskState::Running;
    }

    current_rsp
}

pub unsafe fn list_tasks() {
    vga::set_color(vga::Color::LightBlue, vga::Color::Black);
    vga::print_str("PID    TASK NAME             STATE\n");
    vga::set_color(vga::Color::White, vga::Color::Black);
    for i in 0..MAX_TASKS {
        if let Some(ref task) = TASKS[i] {
            // Print PID
            vga::print_u64(task.id as u64);
            let mut pid_len = 0;
            let mut temp = task.id;
            if temp == 0 {
                pid_len = 1;
            } else {
                while temp > 0 {
                    pid_len += 1;
                    temp /= 10;
                }
            }
            for _ in 0..(7 - pid_len) {
                vga::print_str(" ");
            }

            // Print Name
            vga::print_str(task.name);
            for _ in 0..(22 - task.name.len()) {
                vga::print_str(" ");
            }

            // Print State
            match task.state {
                TaskState::Running => {
                    vga::set_color(vga::Color::LightGreen, vga::Color::Black);
                    vga::print_str("RUNNING\n");
                }
                TaskState::Ready => {
                    vga::set_color(vga::Color::LightBlue, vga::Color::Black);
                    vga::print_str("READY\n");
                }
                TaskState::Terminated => {
                    vga::set_color(vga::Color::Red, vga::Color::Black);
                    vga::print_str("TERMINATED\n");
                }
            }
            vga::set_color(vga::Color::White, vga::Color::Black);
        }
    }
    vga::set_color(vga::Color::LightGrey, vga::Color::Black);
}

fn print_decimal(val: u64) {
    let mut buf = [0u8; 20];
    let mut idx = 20;
    let mut temp = val;
    if temp == 0 {
        serial::print_str("0");
        return;
    }
    while temp > 0 {
        idx -= 1;
        buf[idx] = b'0' + (temp % 10) as u8;
        temp /= 10;
    }
    if let Ok(s) = core::str::from_utf8(&buf[idx..]) {
        serial::print_str(s);
    }
}

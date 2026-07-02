//! Keira Kernel: Task and Interrupt Context Definitions

#[derive(Copy, Clone, PartialEq)]
pub enum TaskState {
    Ready,
    Running,
    Terminated,
}

pub struct Task {
    pub id: usize,
    pub name: &'static str,
    pub rsp: u64,
    pub stack_addr: u64, // Physical frame address for the stack
    pub state: TaskState,
}

#[repr(C, packed)]
pub struct InterruptContext {
    // Pushed by pushaq macro
    pub r15: u64,
    pub r14: u64,
    pub r13: u64,
    pub r12: u64,
    pub r11: u64,
    pub r10: u64,
    pub r9: u64,
    pub r8: u64,
    pub rdi: u64,
    pub rsi: u64,
    pub rbp: u64,
    pub rbx: u64,
    pub rdx: u64,
    pub rcx: u64,
    pub rax: u64,
    // Pushed by CPU on interrupt
    pub rip: u64,
    pub cs: u64,
    pub rflags: u64,
    pub rsp: u64,
    pub ss: u64,
}

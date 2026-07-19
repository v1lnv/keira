# Preemptive Task Scheduler

This module details the design of the cooperative/preemptive multitasking thread scheduler in the Keira Kernel.

---

## 1. Task Control Block (TCB)

Every thread of execution is represented by a Task Control Block (TCB) structure:
- **Thread ID (TID)**: A unique identifier integer.
- **State**: The scheduler manages tasks across four distinct lifecycle states:
  - `Ready`: Waiting in queue to be scheduled.
  - `Running`: Actively executing on the CPU.
  - `Blocked`: Waiting for a timer delay (`wait`) or I/O resource.
  - `Dead`: Completed task awaiting resources cleanup.
- **Stack Pointer (RSP)**: Saves the top address of the thread's stack.
- **Context Frame**: When a thread is suspended, its register state is pushed onto its own stack.

---

## 2. Preemptive Round-Robin Scheduling

The scheduling algorithm uses a Round-Robin queue:
- **Time Slice**: The system timer (PIT) ticks at 1000Hz.
- On each tick, the current thread's remaining time slice is decremented. If it reaches zero, a context switch is triggered preemptively.
- The scheduler iterates through the TCB list, skipping `Blocked` or `Dead` tasks, and selects the next `Ready` thread.

---

## 3. Context Switch Mechanics

The switch is executed via assembly register swapping:
1. **Interrupt Entry**: The CPU pushes RIP, CS, RFLAGS, RSP, and SS automatically.
2. **Push Registers**: The ISR pushes general purpose registers (RAX, RBX, etc.) onto the active thread's stack.
3. **Save Stack Pointer**: The current task's TCB is updated with the active RSP.
4. **Load Next Stack Pointer**: The scheduler selects the next TCB, updates the active page directory (`CR3`), loads `TSS.rsp0` and `kernel_stack_temp` with the new task's kernel stack top, and loads its saved RSP into the CPU's RSP register.
5. **Pop Registers**: General registers of the new task are popped from its stack.
6. **Interrupt Exit**: Executing `iretq` restores RIP and flags, transitioning back to User Mode (Ring 3) or Kernel Mode (Ring 0) depending on CS/SS selectors.

---

## 4. Cleaning Terminated Tasks (v0.13.0)

When a task exits or is terminated (via `sys_exit` or unhandled exception):
- The task state is changed to `Terminated`.
- The task yields CPU control immediately.
- On the next scheduler tick, the scheduler reaps the `Terminated` task:
  1. Recursively releases all VFS file locks held by the task.
  2. Traverses and frees all physical pages and page table frames in its PML4 space.
  3. Frees the kernel stack frame allocated for the task.
  4. Removes the task from the scheduler queue by setting its slot to `None`.
- If a parent process was in `WaitChild` state waiting for this child, the scheduler automatically unblocks the parent by changing its state to `Ready`.

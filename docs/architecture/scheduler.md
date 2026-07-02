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
4. **Load Next Stack Pointer**: The scheduler selects the next TCB and loads its saved RSP into the CPU's RSP register.
5. **Pop Registers**: General registers of the new task are popped from its stack.
6. **Interrupt Exit**: Executing `iretq` restores RIP and flags, resuming the new thread.

---

## 4. Cleaning Dead Tasks

When a thread completes its function execution, it calls an exit routine:
- Marks the task state as `Dead`.
- Yields CPU control immediately to trigger a reschedule.
- During the next idle loop, the scheduler reaps `Dead` tasks, freeing their allocated stack frames to prevent kernel memory leaks.

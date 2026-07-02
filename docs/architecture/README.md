# Architecture Documentation Index

This directory documents the core hardware-level systems, kernel execution context, and CPU privilege boundaries of the Keira Kernel.

## Modules

- **[Bootstrapping & 64-Bit Transition](bootstrapping.md)**
  Transition from Multiboot2 entry (32-bit protected mode) into 64-bit long mode.
- **[Memory Management](memory.md)**
  Physical Memory bitmap frames and Virtual Memory 4-level page directory maps.
- **[Interrupt Handling](interrupts.md)**
  IDT exceptions, assembly ISR stubs, and PIC IRQ routing.
- **[Preemptive Task Scheduler](scheduler.md)**
  Preemptive Round-Robin threads, TCB context buffers, and thread stacks.
- **[System Calls](syscalls.md)**
  Ring 3 user applications privilege boundaries and `syscall` interfaces.

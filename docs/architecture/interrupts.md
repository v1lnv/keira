# Interrupt Handling (IDT & PIC)

This module explains hardware exception and interrupt trapping, detailing the configuration of the Interrupt Descriptor Table (IDT) and the Programmable Interrupt Controller (PIC).

---

## 1. Interrupt Descriptor Table (IDT)

The IDT defines gate descriptors routing CPU exceptions and hardware interrupts to their handlers.
- **Descriptors**: Each of the 256 gates defines a 64-bit address pointer, a code segment selector (`0x08`), gate flags, and privilege attributes.
- **Task Gates vs. Trap Gates**: Exceptions and hardware IRQs use 64-bit Interrupt Gates (which disable interrupts upon entry).

---

## 2. Assembly ISR Stubs (`isr.asm`)

To handle interrupts without modifying general CPU registers, low-level handlers are declared in `arch/x86/kernel/isr.asm`:
1. **Common Code Entry**: Stubs push the interrupt number and error code (or a dummy zero) onto the stack.
2. **Context Preservation**: Registers are pushed in order (RAX, RBX, RCX, RDX, RSI, RDI, RBP, R8-R15).
3. **Rust/C Link**: Call is made to the C handler `isr_handler` or `irq_handler`.
4. **Context Restoration**: General registers are popped back, and execution returns using `iretq`.

---

## 3. 8259 PIC Hardware Remapping

The IBM PC uses two cascaded 8259 Programmable Interrupt Controllers to manage hardware signals:
- By default, the PIC maps IRQ 0-7 to vectors `0x08`-`0x0F` (which conflict with CPU exceptions like Double Faults).
- **Remapping Procedure**: Defined in `arch/x86/kernel/pic.c`. By sending Initialization Command Words (ICWs) to ports `0x20`/`0x21` and `0xA0`/`0xA1`:
  - **Master PIC**: Remapped to vectors `0x20`-`0x27` (IRQ 0-7).
  - **Slave PIC**: Remapped to vectors `0x28`-`0x2F` (IRQ 8-15).

---

## 4. Hardware IRQ Mappings

The remapped hardware interrupts are routed to specific drivers:

| IRQ | Vector | Device | Handler Location |
| --- | ------ | ------ | ---------------- |
| IRQ0 | `0x20` | PIT System Timer | `pit_handler()` in `pit.c` |
| IRQ1 | `0x21` | PS/2 Keyboard | `keyboard_handler()` in `keyboard.c` |
| IRQ12 | `0x2C` | PS/2 Mouse | `mouse_handler()` in `mouse.c` |

All interrupts must acknowledge completion by sending the End of Interrupt (`0x20`) byte to the PIC status registers (ports `0x20` and `0xA0`) to enable subsequent interrupts.

#include "../../../drivers/keyboard/include/keyboard.h"
#include "../../../drivers/mouse/include/mouse.h"
#include <asm/idt.h>
#include <asm/pic.h>
#include <asm/pit.h>

/* The IDT itself: 256 entries */
static idt_entry_t idt[256];

/* Pointer passed to the `lidt` instruction */
static idt_ptr_t idtp;

/* External ASM function to load the IDT */
extern void idt_load(uint64_t idt_ptr);

extern void isr32(void); /* PIT Timer */
extern void isr33(void); /* Keyboard */
extern void isr44(void); /* PS/2 Mouse */

/* External exception handlers */
extern void exception0(void);
extern void exception1(void);
extern void exception2(void);
extern void exception3(void);
extern void exception4(void);
extern void exception5(void);
extern void exception6(void);
extern void exception7(void);
extern void exception8(void);
extern void exception9(void);
extern void exception10(void);
extern void exception11(void);
extern void exception12(void);
extern void exception13(void);
extern void exception14(void);
extern void exception15(void);
extern void exception16(void);
extern void exception17(void);
extern void exception18(void);
extern void exception19(void);
extern void exception20(void);
extern void exception21(void);
extern void exception22(void);
extern void exception23(void);
extern void exception24(void);
extern void exception25(void);
extern void exception26(void);
extern void exception27(void);
extern void exception28(void);
extern void exception29(void);
extern void exception30(void);
extern void exception31(void);

/**
 * Common interrupt dispatcher.
 * Called by the assembly ISR stubs.
 */
void isr_handler(uint64_t vector) {
    if (vector == 32) {
        pit_handler();
    } else if (vector == 33) {
        keyboard_handler();
    } else if (vector == 44) {
        mouse_handler();
    } else {
        /* Unhandled interrupt */
        pic_eoi(vector >= 40 ? 8 : 0);
    }
}

void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags, uint8_t ist) {
    idt[num].offset_low = (base & 0xFFFF);
    idt[num].offset_mid = (base >> 16) & 0xFFFF;
    idt[num].offset_high = (base >> 32) & 0xFFFFFFFF;
    idt[num].selector = sel;
    idt[num].ist = ist;
    idt[num].type_attr = flags;
    idt[num].zero = 0;
}

void idt_init(void) {
    /* Set up the IDT pointer */
    idtp.limit = (sizeof(idt_entry_t) * 256) - 1;
    idtp.base = (uint64_t)&idt;

    /* Clear out the entire IDT, initializing it to zeros */
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0, 0);
    }

    /*
     * Register CPU Exception Handlers (Gates 0-31)
     */
    idt_set_gate(0, (uint64_t)exception0, 0x08, 0x8E, 0);
    idt_set_gate(1, (uint64_t)exception1, 0x08, 0x8E, 0);
    idt_set_gate(2, (uint64_t)exception2, 0x08, 0x8E, 0);
    idt_set_gate(3, (uint64_t)exception3, 0x08, 0x8E, 0);
    idt_set_gate(4, (uint64_t)exception4, 0x08, 0x8E, 0);
    idt_set_gate(5, (uint64_t)exception5, 0x08, 0x8E, 0);
    idt_set_gate(6, (uint64_t)exception6, 0x08, 0x8E, 0);
    idt_set_gate(7, (uint64_t)exception7, 0x08, 0x8E, 0);
    idt_set_gate(8, (uint64_t)exception8, 0x08, 0x8E, 0);
    idt_set_gate(9, (uint64_t)exception9, 0x08, 0x8E, 0);
    idt_set_gate(10, (uint64_t)exception10, 0x08, 0x8E, 0);
    idt_set_gate(11, (uint64_t)exception11, 0x08, 0x8E, 0);
    idt_set_gate(12, (uint64_t)exception12, 0x08, 0x8E, 0);
    idt_set_gate(13, (uint64_t)exception13, 0x08, 0x8E, 0);
    idt_set_gate(14, (uint64_t)exception14, 0x08, 0x8E, 0);
    idt_set_gate(15, (uint64_t)exception15, 0x08, 0x8E, 0);
    idt_set_gate(16, (uint64_t)exception16, 0x08, 0x8E, 0);
    idt_set_gate(17, (uint64_t)exception17, 0x08, 0x8E, 0);
    idt_set_gate(18, (uint64_t)exception18, 0x08, 0x8E, 0);
    idt_set_gate(19, (uint64_t)exception19, 0x08, 0x8E, 0);
    idt_set_gate(20, (uint64_t)exception20, 0x08, 0x8E, 0);
    idt_set_gate(21, (uint64_t)exception21, 0x08, 0x8E, 0);
    idt_set_gate(22, (uint64_t)exception22, 0x08, 0x8E, 0);
    idt_set_gate(23, (uint64_t)exception23, 0x08, 0x8E, 0);
    idt_set_gate(24, (uint64_t)exception24, 0x08, 0x8E, 0);
    idt_set_gate(25, (uint64_t)exception25, 0x08, 0x8E, 0);
    idt_set_gate(26, (uint64_t)exception26, 0x08, 0x8E, 0);
    idt_set_gate(27, (uint64_t)exception27, 0x08, 0x8E, 0);
    idt_set_gate(28, (uint64_t)exception28, 0x08, 0x8E, 0);
    idt_set_gate(29, (uint64_t)exception29, 0x08, 0x8E, 0);
    idt_set_gate(30, (uint64_t)exception30, 0x08, 0x8E, 0);
    idt_set_gate(31, (uint64_t)exception31, 0x08, 0x8E, 0);

    /*
     * Register IRQ handlers
     * PIC maps IRQ0-7 to vectors 32-39
     * PIC maps IRQ8-15 to vectors 40-47
     */
    /* 0x08 is the code segment selector (see arch/x86/kernel/gdt.asm) */
    /* 0x8E is a 64-bit Interrupt Gate, Present, Ring 0 */
    idt_set_gate(32, (uint64_t)isr32, 0x08, 0x8E, 0);
    idt_set_gate(33, (uint64_t)isr33, 0x08, 0x8E, 0);
    idt_set_gate(44, (uint64_t)isr44, 0x08, 0x8E, 0);

    /* Load the IDT into the CPU */
    idt_load((uint64_t)&idtp);
}

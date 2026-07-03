#include "include/keyboard.h"
#include "scancodes.h"
#include <asm/io.h>
#include <asm/pic.h>

/* The data port for the keyboard */
#define KBD_DATA_PORT 0x60

/* External Rust function to handle keypresses */
extern void shell_handle_keypress(uint8_t c);

/* Keyboard layout arrays and scan code macros are included from scancodes.h */

/* State to track shift key status (simple toggle for now) */
static int shift_pressed = 0;
static int ctrl_pressed = 0;

void keyboard_init(void) {
    /* Keyboard initialization (PIC mask clearing is done elsewhere, but we could
     * do it here) */
    pic_clear_mask(1);
}

void keyboard_handler(void) {
    uint8_t scancode;

    /* Read from the keyboard's data buffer */
    scancode = inb(KBD_DATA_PORT);

    /* If the top bit is set, a key was just released */
    if (scancode & 0x80) {
        /* Check for shift/control release */
        if (scancode == (KEY_LSHIFT | 0x80) || scancode == (KEY_RSHIFT | 0x80)) {
            shift_pressed = 0;
        } else if (scancode == (KEY_LCTRL | 0x80)) {
            ctrl_pressed = 0;
        }
    } else {
        /* A key was just pressed */
        if (scancode == KEY_LSHIFT || scancode == KEY_RSHIFT) {
            shift_pressed = 1;
        } else if (scancode == KEY_LCTRL) {
            ctrl_pressed = 1;
        } else if (scancode == KEY_UP) {
            shell_handle_keypress(0x80);
        } else if (scancode == KEY_DOWN) {
            shell_handle_keypress(0x81);
        } else if (scancode == KEY_LEFT) {
            shell_handle_keypress(0x82);
        } else if (scancode == KEY_RIGHT) {
            shell_handle_keypress(0x83);
        } else if (scancode == KEY_F3) {
            shell_handle_keypress(0x84);
        } else if (scancode == KEY_F10) {
            shell_handle_keypress(0x85);
        } else {
            /* Map scan code to ASCII using layout tables */
            if (scancode < 128) {
                unsigned char c =
                    shift_pressed ? kbd_us_shifted_layout[scancode] : kbd_us_layout[scancode];

                if (c != 0) {
                    /* Check Ctrl key shortcuts first */
                    if (ctrl_pressed) {
                        if (c == 's' || c == 'S') {
                            shell_handle_keypress(19); // Ctrl+S (Save)
                            pic_eoi(1);
                            return;
                        }
                        if (c == 'q' || c == 'Q') {
                            shell_handle_keypress(17); // Ctrl+Q (Quit)
                            pic_eoi(1);
                            return;
                        }
                    }

                    /* Pass character to the shell */
                    shell_handle_keypress(c);
                }
            }
        }
    }

    /* Acknowledge the interrupt to the PIC (IRQ 1) */
    pic_eoi(1);
}

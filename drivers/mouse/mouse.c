#include "include/mouse.h"
#include "../vga/include/vga.h"
#include "regs.h"
#include <asm/io.h>
#include <asm/pic.h>

/* Mouse State Machine */
static uint8_t mouse_cycle = 0;
static int8_t mouse_byte[3];

/* Mouse Position (VGA Text Mode: 80x25) */
static int32_t mouse_x = 40; /* Start in middle of screen */
static int32_t mouse_y = 12;
static int32_t mouse_fx = 40 * 256; /* Fixed-point coordinates (coordinate * 256) */
static int32_t mouse_fy = 12 * 256;

static int32_t mouse_max_x = 80;
static int32_t mouse_max_y = 25;
static int32_t mouse_sensitivity_x = 24;
static int32_t mouse_sensitivity_y = 12;

void mouse_set_resolution(int32_t width, int32_t height) {
    mouse_max_x = width;
    mouse_max_y = height;
    mouse_x = width / 2;
    mouse_y = height / 2;
    mouse_fx = mouse_x * 256;
    mouse_fy = mouse_y * 256;

    if (width > 80) {
        // Graphics mode: needs square pixels and much higher sensitivity
        mouse_sensitivity_x = 512;
        mouse_sensitivity_y = 512;
    } else {
        // Text mode
        mouse_sensitivity_x = 24;
        mouse_sensitivity_y = 12;
    }
}

/* PS/2 Controller wait functions */
static inline void mouse_wait(uint8_t a_type) {
    uint32_t timeout = 100000;
    if (a_type == 0) {
        /* Wait for data to be readable */
        while (timeout--) {
            if ((inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_FULL) == PS2_STATUS_OUTPUT_FULL) {
                return;
            }
        }
    } else {
        /* Wait for buffer to be empty before writing */
        while (timeout--) {
            if ((inb(PS2_STATUS_PORT) & PS2_STATUS_INPUT_FULL) == 0) {
                return;
            }
        }
    }
}

static inline void mouse_write(uint8_t a_write) {
    /* Tell controller we are sending a command to the mouse */
    mouse_wait(1);
    outb(PS2_COMMAND_PORT, PS2_CMD_WRITE_MOUSE);
    /* Send the data */
    mouse_wait(1);
    outb(PS2_DATA_PORT, a_write);
}

static inline uint8_t mouse_read(void) {
    mouse_wait(0);
    return inb(PS2_DATA_PORT);
}

void mouse_init(void) {
    uint8_t status;

    /* Enable the auxiliary mouse device */
    mouse_wait(1);
    outb(PS2_COMMAND_PORT, PS2_CMD_ENABLE_MOUSE);

    /* Fetch the "Compaq status byte" */
    mouse_wait(1);
    outb(PS2_COMMAND_PORT, PS2_CMD_READ_CONFIG);
    mouse_wait(0);
    status = inb(PS2_DATA_PORT);

    /* Enable IRQ12 (bit 1) */
    status |= 2;
    /* Clear bit 5 (disable mouse clock line) just in case */
    status &= ~0x20;

    /* Write the status byte back */
    mouse_wait(1);
    outb(PS2_COMMAND_PORT, PS2_CMD_WRITE_CONFIG);
    mouse_wait(1);
    outb(PS2_DATA_PORT, status);

    /* Tell the mouse to use default settings */
    mouse_write(MOUSE_CMD_SET_DEFAULTS);
    mouse_read(); /* Acknowledge */

    /* Enable Packet Streaming */
    mouse_write(MOUSE_CMD_ENABLE_STREAM);
    mouse_read(); /* Acknowledge */

    /* Unmask IRQ12 on the Slave PIC */
    pic_clear_mask(MOUSE_IRQ);

    /* Draw the initial mouse cursor */
    vga_draw_mouse(mouse_x, mouse_y);
}

void mouse_handler(void) {
    uint8_t status = inb(PS2_STATUS_PORT);

    /* Ensure data is from mouse (bit 5 must be 1) and buffer is full (bit 0 is 1) */
    if ((status & PS2_STATUS_MOUSE_DATA) == PS2_STATUS_MOUSE_DATA &&
        (status & PS2_STATUS_OUTPUT_FULL) == PS2_STATUS_OUTPUT_FULL) {
        mouse_byte[mouse_cycle++] = inb(PS2_DATA_PORT);

        if (mouse_cycle == 1 && (mouse_byte[0] & MOUSE_FLAGS_SYNC) == 0) {
            mouse_cycle = 0;
            pic_eoi(MOUSE_IRQ);
            return;
        }

        if (mouse_cycle == 3) {
            mouse_cycle = 0;

            /* Parse the packet */
            /* Ignore packets if x or y overflowed */
            if ((mouse_byte[0] & (MOUSE_FLAGS_X_OVERFLOW | MOUSE_FLAGS_Y_OVERFLOW)) == 0) {
                /* Accumulate in fixed-point to allow smooth low-speed movement and prevent hyper-sensitivity */
                mouse_fx += (int32_t)mouse_byte[1] * mouse_sensitivity_x;
                mouse_fy -= (int32_t)mouse_byte[2] * mouse_sensitivity_y;

                /* Clamp to screen boundaries (in 1/256 fixed-point) */
                if (mouse_fx < 0)
                    mouse_fx = 0;
                if (mouse_fx >= mouse_max_x * 256)
                    mouse_fx = (mouse_max_x - 1) * 256;
                if (mouse_fy < 0)
                    mouse_fy = 0;
                if (mouse_fy >= mouse_max_y * 256)
                    mouse_fy = (mouse_max_y - 1) * 256;

                int32_t new_x = mouse_fx / 256;
                int32_t new_y = mouse_fy / 256;

                /* Only redraw if the mouse has actually moved to a new character cell */
                if (new_x != mouse_x || new_y != mouse_y) {
                    /* Clear the old cursor */
                    vga_clear_mouse(mouse_x, mouse_y);

                    mouse_x = new_x;
                    mouse_y = new_y;

                    /* Draw the new cursor */
                    vga_draw_mouse(mouse_x, mouse_y);
                }
            }
        }
    } else {
        /* Even if not a valid mouse packet, we must read PS2_DATA_PORT to clear the
         * interrupt */
        if (status & PS2_STATUS_OUTPUT_FULL) {
            inb(PS2_DATA_PORT);
        }
    }

    /* Acknowledge the interrupt to the PIC (IRQ 12) */
    pic_eoi(MOUSE_IRQ);
}

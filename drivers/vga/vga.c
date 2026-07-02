/**
 * Keira Kernel: VGA Text Mode Driver (Implementation)
 *
 * Direct memory-mapped driver for the VGA text mode display (80×25).
 *
 * The VGA text buffer is a 2D array of 16-bit entries at physical address
 * 0xB8000. Each entry consists of:
 *   - Bits [7:0]   : ASCII character code
 *   - Bits [11:8]  : Foreground color (4-bit CGA)
 *   - Bits [14:12] : Background color (3-bit, high bit = blink)
 *   - Bit  [15]    : Blink enable
 *
 * The buffer is 80 columns × 25 rows = 4000 bytes total.
 *
 * Reference: https://wiki.osdev.org/Text_UI
 */

#include "include/vga.h"
#include "../../arch/x86/include/asm/io.h"

/* Display Constants */
#define VGA_BUFFER ((volatile uint16_t *)0xB8000)
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

/**
 * Driver State
 *
 * Static variables track the current cursor position and color attribute.
 * These are placed in BSS (zero-initialized by the bootstrap).
 */
static uint16_t cursor_row;
static uint16_t cursor_col;
static uint8_t current_attr = 0x07; // Default: Light Grey on Black
static uint8_t vga_fb_mode = 0;     // Set to 1 when running in LFB graphics mode

void vga_set_fb_mode(uint8_t enabled) {
    vga_fb_mode = enabled;
}

static uint16_t mouse_cursor_x = 0xFFFF;
static uint16_t mouse_cursor_y = 0xFFFF;
static uint16_t saved_mouse_entry = 0;
static uint8_t mouse_is_visible = 0;

static void vga_hide_mouse_internal(void) {
    if (mouse_is_visible) {
        uint16_t index = mouse_cursor_y * VGA_WIDTH + mouse_cursor_x;
        VGA_BUFFER[index] = saved_mouse_entry;
        mouse_is_visible = 0;
    }
}

static void vga_show_mouse_internal(void) {
    if (!mouse_is_visible && mouse_cursor_x < VGA_WIDTH && mouse_cursor_y < VGA_HEIGHT) {
        uint16_t index = mouse_cursor_y * VGA_WIDTH + mouse_cursor_x;
        uint16_t entry = VGA_BUFFER[index];
        saved_mouse_entry = entry;

        // Extract the original background color of the cell
        uint8_t bg = (entry >> 12) & 0x0F;

        // Custom premium pointer: Windows/Linux style white arrow on transparent background
        uint8_t pointer_char = 24; // '↑'
        uint8_t pointer_attr;

        // Dynamic contrast: use black arrow if background is light, white arrow otherwise
        if (bg == VGA_COLOR_WHITE || bg == VGA_COLOR_LIGHT_GREY || bg == VGA_COLOR_YELLOW ||
            bg == VGA_COLOR_LIGHT_CYAN) {
            pointer_attr = VGA_COLOR_BLACK | (bg << 4);
        } else {
            pointer_attr = VGA_COLOR_WHITE | (bg << 4);
        }

        VGA_BUFFER[index] = (uint16_t)pointer_char | ((uint16_t)pointer_attr << 8);
        mouse_is_visible = 1;
    }
}

/**
 * Combine character and attribute into a VGA cell value.
 *
 * @param c    ASCII character
 * @param attr Color attribute byte (fg | bg << 4)
 * @return     16-bit VGA text buffer entry
 */
static inline uint16_t vga_make_entry(char c, uint8_t attr) {
    return (uint16_t)c | ((uint16_t)attr << 8);
}

/**
 * Combine foreground and background into attribute byte.
 *
 * @param fg Foreground color (0–15)
 * @param bg Background color (0–15)
 * @return   Attribute byte
 */
static inline uint8_t vga_make_color(uint8_t fg, uint8_t bg) {
    return fg | (bg << 4);
}

/**
 * Scroll the display up by one line.
 *
 * Copies each row up by one position and clears the bottom row.
 * Called automatically when the cursor moves past the last row.
 */
static void vga_scroll(void) {
    /* Move rows 1..24 up to rows 0..23 */
    for (uint16_t row = 1; row < VGA_HEIGHT; row++) {
        for (uint16_t col = 0; col < VGA_WIDTH; col++) {
            uint16_t src_index = row * VGA_WIDTH + col;
            uint16_t dst_index = (row - 1) * VGA_WIDTH + col;
            VGA_BUFFER[dst_index] = VGA_BUFFER[src_index];
        }
    }

    /* Clear the bottom row */
    uint16_t blank = vga_make_entry(' ', current_attr);
    uint16_t last_row_start = (VGA_HEIGHT - 1) * VGA_WIDTH;
    for (uint16_t col = 0; col < VGA_WIDTH; col++) {
        VGA_BUFFER[last_row_start + col] = blank;
    }
}

/**
 * Update the hardware cursor on the screen.
 */
static void vga_update_cursor(void) {
    uint16_t pos = cursor_row * VGA_WIDTH + cursor_col;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

/**
 * Enable and configure the hardware blinking cursor.
 * Uses CRTC registers 0x0A (cursor start) and 0x0B (cursor end).
 *
 * @param cursor_start Start scanline (0-15)
 * @param cursor_end   End scanline (0-15)
 */
void vga_enable_cursor(uint8_t cursor_start, uint8_t cursor_end) {
    /* Cursor Start Register: bits 0-4 = start scanline, bit 5 = disable cursor */
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | cursor_start);

    /* Cursor End Register: bits 0-4 = end scanline */
    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | cursor_end);
}

/* Public API Implementation */

void vga_init(void) {
    if (vga_fb_mode)
        return;
    vga_hide_mouse_internal();
    cursor_row = 0;
    cursor_col = 0;

    /* Clear entire screen */
    uint16_t blank = vga_make_entry(' ', current_attr);
    for (uint16_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VGA_BUFFER[i] = blank;
    }

    /* Enable blinking underline cursor (scanlines 13-15) */
    vga_enable_cursor(13, 15);
    vga_update_cursor();
    vga_show_mouse_internal();
}

void vga_set_color(uint8_t fg, uint8_t bg) {
    if (vga_fb_mode)
        return;
    current_attr = vga_make_color(fg, bg);
}

void vga_putchar(char c) {
    if (vga_fb_mode)
        return;
    vga_hide_mouse_internal();
    if (c == '\n') {
        /* Newline: move to start of next row */
        cursor_col = 0;
        cursor_row++;
    } else {
        /* Write character at current position */
        uint16_t index = cursor_row * VGA_WIDTH + cursor_col;
        VGA_BUFFER[index] = vga_make_entry(c, current_attr);
        cursor_col++;

        /* Wrap to next line if past right edge */
        if (cursor_col >= VGA_WIDTH) {
            cursor_col = 0;
            cursor_row++;
        }
    }

    /* Scroll if cursor has moved past the bottom */
    if (cursor_row >= VGA_HEIGHT) {
        vga_scroll();
        cursor_row = VGA_HEIGHT - 1;
    }

    vga_update_cursor();
    vga_show_mouse_internal();
}

void vga_print(const char *str) {
    while (*str) {
        vga_putchar(*str);
        str++;
    }
}

void vga_backspace(void) {
    if (vga_fb_mode)
        return;
    vga_hide_mouse_internal();
    if (cursor_col == 0) {
        if (cursor_row > 0) {
            cursor_row--;
            cursor_col = VGA_WIDTH - 1;
        }
    } else {
        cursor_col--;
    }

    uint16_t index = cursor_row * VGA_WIDTH + cursor_col;
    VGA_BUFFER[index] = vga_make_entry(' ', current_attr);
    vga_update_cursor();
    vga_show_mouse_internal();
}

void vga_draw_mouse_text(uint16_t x, uint16_t y) {
    vga_hide_mouse_internal();
    mouse_cursor_x = x;
    mouse_cursor_y = y;
    vga_show_mouse_internal();
}

void vga_clear_mouse_text(uint16_t x, uint16_t y) {
    (void)x;
    (void)y;
    vga_hide_mouse_internal();
}

uint16_t vga_get_cursor_col(void) {
    return cursor_col;
}

uint16_t vga_get_cursor_row(void) {
    return cursor_row;
}

void vga_set_cursor_pos(uint16_t row, uint16_t col) {
    cursor_row = row;
    cursor_col = col;
    vga_update_cursor();
}

void vga_clear_line_from(uint16_t col) {
    if (vga_fb_mode)
        return;
    vga_hide_mouse_internal();
    uint16_t blank = vga_make_entry(' ', current_attr);
    for (uint16_t c = col; c < VGA_WIDTH; c++) {
        uint16_t index = cursor_row * VGA_WIDTH + c;
        VGA_BUFFER[index] = blank;
    }
    vga_show_mouse_internal();
}

/**
 * Keira Kernel: VGA Text Mode Driver (Header)
 *
 * Public interface for the VGA text mode (80×25) driver.
 * Writes directly to the VGA text buffer at physical address 0xB8000.
 *
 * Each character cell is 2 bytes:
 *   Byte 0: ASCII character code
 *   Byte 1: Attribute (foreground | background << 4)
 *
 * Usage:
 *   vga_init();
 *   vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
 *   vga_print("Keira Kernel v0.1.0\n");
 */

#ifndef KEIRA_VGA_H
#define KEIRA_VGA_H

#include <stdint.h>

/* VGA Color Constants (4-bit CGA palette) */
typedef enum {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_YELLOW = 14,
    VGA_COLOR_WHITE = 15,
} vga_color_t;

/**
 * Clear the screen and reset cursor to (0, 0).
 *
 * Fills the entire 80×25 text buffer with spaces using the default
 * color (light grey on black). Must be called before any VGA output.
 */
void vga_init(void);

/**
 * Set the current text color attribute.
 *
 * @param fg Foreground color (0–15, from vga_color_t).
 * @param bg Background color (0–15, from vga_color_t).
 */
void vga_set_color(uint8_t fg, uint8_t bg);

/**
 * Write a single character at the current cursor position.
 *
 * Handles newline ('\n') by advancing to the next row. When the cursor
 * reaches the bottom of the screen, the display scrolls up by one line.
 *
 * @param c The ASCII character to display.
 */
void vga_putchar(char c);

/**
 * Write a null-terminated string to the VGA display.
 *
 * @param str Pointer to null-terminated string.
 */
void vga_print(const char *str);

/**
 * Handle a backspace character by moving the cursor left and erasing.
 */
void vga_backspace(void);

/**
 * Draw a mouse cursor at the specified coordinates by inverting colors.
 */
void vga_draw_mouse(uint16_t x, uint16_t y);

/**
 * Clear the mouse cursor at the specified coordinates.
 */
void vga_clear_mouse(uint16_t x, uint16_t y);

/**
 * Enable and configure the hardware blinking cursor.
 */
void vga_enable_cursor(uint8_t cursor_start, uint8_t cursor_end);

/**
 * Get the current cursor column.
 */
uint16_t vga_get_cursor_col(void);

/**
 * Get the current cursor row.
 */
uint16_t vga_get_cursor_row(void);

/**
 * Set the cursor position.
 */
void vga_set_cursor_pos(uint16_t row, uint16_t col);

/**
 * Clear the current line from the given column to end of line.
 */
void vga_clear_line_from(uint16_t col);

#endif /* KEIRA_VGA_H */

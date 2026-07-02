#ifndef MOUSE_REGS_H
#define MOUSE_REGS_H

/* PS/2 Controller Ports */
#define PS2_DATA_PORT 0x60
#define PS2_STATUS_PORT 0x64
#define PS2_COMMAND_PORT 0x64

/* PS/2 Status Register Bits */
#define PS2_STATUS_OUTPUT_FULL 0x01 /* Bit 0: Output buffer full (data ready to read) */
#define PS2_STATUS_INPUT_FULL 0x02  /* Bit 1: Input buffer full (cannot write yet) */
#define PS2_STATUS_MOUSE_DATA                                          \
    0x20 /* Bit 5: Output buffer data is from auxiliary (mouse) device \
          */

/* PS/2 Controller Commands */
#define PS2_CMD_READ_CONFIG 0x20  /* Read "Compaq" Command Byte */
#define PS2_CMD_WRITE_CONFIG 0x60 /* Write "Compaq" Command Byte */
#define PS2_CMD_ENABLE_MOUSE 0xA8 /* Enable Auxiliary (Mouse) Device */
#define PS2_CMD_WRITE_MOUSE 0xD4  /* Write Byte to Auxiliary Device */

/* PS/2 Mouse Commands */
#define MOUSE_CMD_SET_DEFAULTS 0xF6  /* Set default settings */
#define MOUSE_CMD_ENABLE_STREAM 0xF4 /* Enable packet streaming */

/* Mouse IRQ Number */
#define MOUSE_IRQ 12

/* PS/2 Mouse Packet Byte 0 Flag Masks */
#define MOUSE_FLAGS_LEFT_BUTTON 0x01
#define MOUSE_FLAGS_RIGHT_BUTTON 0x02
#define MOUSE_FLAGS_MIDDLE_BUTTON 0x04
#define MOUSE_FLAGS_SYNC 0x08 /* Must be 1 in first byte of valid packet */
#define MOUSE_FLAGS_X_SIGN 0x10
#define MOUSE_FLAGS_Y_SIGN 0x20
#define MOUSE_FLAGS_X_OVERFLOW 0x40
#define MOUSE_FLAGS_Y_OVERFLOW 0x80

#endif /* MOUSE_REGS_H */

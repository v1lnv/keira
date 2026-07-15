/**
 * Keira User Space: Init Process
 *
 * The entry point for the user space init program. Displays demonstration
 * output for string manipulation, number formatting, sleep, and exit system calls.
 */

#include "../../lib/include/stdio.h"   // IWYU pragma: keep
#include "../../lib/include/string.h"  // IWYU pragma: keep
#include "../../lib/include/syscall.h" // IWYU pragma: keep

void _start(void) {
    printf("Keira Freestanding User Program\n");
    printf("Running in Ring 3 (User Mode) CPU protection level.\n\n");

    // String demonstration
    const char *greet = "Hello from freestanding ELF!";
    char buffer[64];
    memset(buffer, 0, sizeof(buffer));
    strncpy(buffer, greet, sizeof(buffer) - 1);
    printf("Testing String Copy & Format: '%s' (Length: %d)\n", buffer, (int)strlen(buffer));

    // Number formatting demonstration
    int val_dec = -12345;
    unsigned int val_hex = 0xABCDEF12;
    printf("Testing Decimal Formatting  : %d\n", val_dec);
    printf("Testing Hexadecimal Formatting: %x\n\n", val_hex);

    // Sleep syscall demonstration
    printf("Testing Sleep System Call (waiting 1 second)... ");
    sys_sleep(1000);
    printf("Done!\n\n");

    printf("Exiting User Mode and returning to Kernel shell.\n");
    sys_exit();
}

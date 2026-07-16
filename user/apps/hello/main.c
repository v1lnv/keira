/**
 * Keira User Space: Hello Program
 *
 * A standalone user-space application demonstrating process identity,
 * heap allocation, and clean exit. Can be spawned from init or run directly.
 */

#include "../../lib/include/stdio.h"
#include "../../lib/include/string.h"
#include "../../lib/include/syscall.h"
#include "../../lib/include/malloc.h"

void _start(void) {
    printf("=== Hello from Keira User App ===\n");
    printf("  Process ID (PID): %d\n", sys_getpid());

    // Demonstrate working directory
    char cwd[128];
    memset(cwd, 0, sizeof(cwd));
    int cwd_len = sys_getcwd(cwd, sizeof(cwd) - 1);
    if (cwd_len > 0) {
        printf("  Working Directory: %s\n", cwd);
    }

    // Demonstrate heap allocation
    int *numbers = (int *)malloc(5 * sizeof(int));
    if (numbers != NULL) {
        for (int i = 0; i < 5; i++) {
            numbers[i] = (i + 1) * 10;
        }
        printf("  Heap Array: [%d, %d, %d, %d, %d]\n",
               numbers[0], numbers[1], numbers[2], numbers[3], numbers[4]);
        free(numbers);
    }

    printf("=== Hello program exiting ===\n");
    sys_exit();
}

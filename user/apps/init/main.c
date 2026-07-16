/**
 * Keira User Space: Init Process
 *
 * The entry point for the user space init program. Displays demonstration
 * output for string manipulation, number formatting, sleep, and exit system calls.
 */

#include "../../lib/include/stdio.h"   // IWYU pragma: keep
#include "../../lib/include/string.h"  // IWYU pragma: keep
#include "../../lib/include/syscall.h" // IWYU pragma: keep
#include "../../lib/include/malloc.h"


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

    // File system syscall demonstration
    printf("Testing File System Syscalls:\n");
    int fd = sys_open("/data/log/test.log", 1);
    if (fd >= 0) {
        printf("  Opened /data/log/test.log in write mode (FD: %d)\n", fd);
        const char *msg = "Keira File System Syscalls Work!";
        int written = sys_write(fd, msg, (int)strlen(msg));
        printf("  Wrote %d bytes to file.\n", written);
        
        sys_seek(fd, 0);
        
        char read_buf[64];
        memset(read_buf, 0, sizeof(read_buf));
        int read_bytes = sys_read(fd, read_buf, sizeof(read_buf) - 1);
        printf("  Read %d bytes back: '%s'\n", read_bytes, read_buf);
        
        sys_close(fd);
        printf("  Closed file.\n\n");
    } else {
        printf("  Failed to open /data/log/test.log\n\n");
    }

    // Dynamic memory test
    printf("Testing Dynamic Memory (malloc & free):\n");
    char *heap_str = (char *)malloc(32);
    if (heap_str != NULL) {
        strncpy(heap_str, "Heap Allocation Works!", 31);
        printf("  Allocated 32 bytes on heap: '%s'\n", heap_str);
        free(heap_str);
        printf("  Freed heap memory successfully.\n\n");
    } else {
        printf("  Failed to allocate memory on heap.\n\n");
    }

    // Process identity test
    printf("Testing Process Identity:\n");
    printf("  My PID: %d\n", sys_getpid());
    char cwd[128];
    memset(cwd, 0, sizeof(cwd));
    int cwd_len = sys_getcwd(cwd, sizeof(cwd) - 1);
    if (cwd_len > 0) {
        printf("  Current Working Directory: %s\n\n", cwd);
    }

    // Child process spawn test
    printf("Testing Process Spawn (launching hello.elf):\n");
    int spawn_result = sys_spawn("/apps/bin/hello.elf");
    if (spawn_result == 0) {
        printf("  Child process completed successfully.\n\n");
    } else {
        printf("  Child process spawn failed or not found.\n\n");
    }

    printf("Exiting User Mode and returning to Kernel shell.\n");
    sys_exit();
}

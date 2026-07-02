/**
 * Keira User Space Library: System Call Wrappers (Implementation)
 *
 * Implements wrappers for x86_64 syscall instructions to interface with the kernel.
 */

#include "include/syscall.h"

void sys_print_char(char c) {
    __asm__ volatile("syscall" : : "a"(1), "D"((unsigned long)c) : "rcx", "r11", "memory");
}

void sys_exit(void) {
    __asm__ volatile("syscall" : : "a"(2) : "rcx", "r11", "memory");
    while (1)
        ; // Guard against exit failure
}

void sys_sleep(unsigned long ms) {
    __asm__ volatile("syscall" : : "a"(3), "D"(ms) : "rcx", "r11", "memory");
}

unsigned long sys_uptime(void) {
    unsigned long res;
    __asm__ volatile("syscall" : "=a"(res) : "a"(4) : "rcx", "r11", "memory");
    return res;
}

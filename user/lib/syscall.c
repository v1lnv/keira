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

int sys_exec(const char *filename) {
    unsigned long res;
    __asm__ volatile("syscall" : "=a"(res) : "a"(5), "D"((unsigned long)filename) : "rcx", "r11", "memory");
    return (int)res;
}

int sys_open(const char *path, int write_mode) {
    unsigned long res;
    __asm__ volatile("syscall" : "=a"(res) : "a"(6), "D"((unsigned long)path), "S"((unsigned long)write_mode) : "rcx", "r11", "memory");
    return (int)res;
}

int sys_read(int fd, void *buf, int len) {
    unsigned long res;
    __asm__ volatile("syscall" : "=a"(res) : "a"(7), "D"((unsigned long)fd), "S"((unsigned long)buf), "d"((unsigned long)len) : "rcx", "r11", "memory");
    return (int)res;
}

int sys_write(int fd, const void *buf, int len) {
    unsigned long res;
    __asm__ volatile("syscall" : "=a"(res) : "a"(8), "D"((unsigned long)fd), "S"((unsigned long)buf), "d"((unsigned long)len) : "rcx", "r11", "memory");
    return (int)res;
}

int sys_close(int fd) {
    unsigned long res;
    __asm__ volatile("syscall" : "=a"(res) : "a"(9), "D"((unsigned long)fd) : "rcx", "r11", "memory");
    return (int)res;
}

int sys_seek(int fd, unsigned long offset) {
    unsigned long res;
    __asm__ volatile("syscall" : "=a"(res) : "a"(10), "D"((unsigned long)fd), "S"(offset) : "rcx", "r11", "memory");
    return (int)res;
}

void *sys_sbrk(long increment) {
    unsigned long res;
    __asm__ volatile("syscall" : "=a"(res) : "a"(11), "D"((unsigned long)increment) : "rcx", "r11", "memory");
    return (void *)res;
}

int sys_spawn(const char *path) {
    unsigned long res;
    __asm__ volatile("syscall" : "=a"(res) : "a"(12), "D"((unsigned long)path) : "rcx", "r11", "memory");
    return (int)res;
}

int sys_waitpid(int pid) {
    unsigned long res;
    __asm__ volatile("syscall" : "=a"(res) : "a"(13), "D"((unsigned long)pid) : "rcx", "r11", "memory");
    return (int)res;
}

int sys_getpid(void) {
    unsigned long res;
    __asm__ volatile("syscall" : "=a"(res) : "a"(14) : "rcx", "r11", "memory");
    return (int)res;
}

int sys_getcwd(char *buf, int len) {
    unsigned long res;
    __asm__ volatile("syscall" : "=a"(res) : "a"(15), "D"((unsigned long)buf), "S"((unsigned long)len) : "rcx", "r11", "memory");
    return (int)res;
}

int sys_chdir(const char *path) {
    unsigned long res;
    __asm__ volatile("syscall" : "=a"(res) : "a"(16), "D"((unsigned long)path) : "rcx", "r11", "memory");
    return (int)res;
}

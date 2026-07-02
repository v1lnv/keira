#ifndef USER_SYSCALL_H
#define USER_SYSCALL_H

/**
 * System Call Wrappers for Keira User Space
 */

void sys_print_char(char c);
void sys_exit(void) __attribute__((noreturn));
void sys_sleep(unsigned long ms);
unsigned long sys_uptime(void);

#endif // USER_SYSCALL_H

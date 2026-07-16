#ifndef USER_SYSCALL_H
#define USER_SYSCALL_H

/**
 * System Call Wrappers for Keira User Space
 */

void sys_print_char(char c);
void sys_exit(void) __attribute__((noreturn));
void sys_sleep(unsigned long ms);
unsigned long sys_uptime(void);
int sys_exec(const char *filename);
int sys_open(const char *path, int write_mode);
int sys_read(int fd, void *buf, int len);
int sys_write(int fd, const void *buf, int len);
int sys_close(int fd);
int sys_seek(int fd, unsigned long offset);
void *sys_sbrk(long increment);

#endif // USER_SYSCALL_H

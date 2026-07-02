#ifndef USER_STRING_H
#define USER_STRING_H

/**
 * Standard String and Memory operations for Keira User Space
 */

unsigned long strlen(const char *s);
void *memcpy(void *dest, const void *src, unsigned long n);
void *memset(void *s, int c, unsigned long n);
int strcmp(const char *s1, const char *s2);
char *strcpy(char *dest, const char *src);

#endif // USER_STRING_H

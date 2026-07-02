/**
 * Keira User Space Library: String Operations (Implementation)
 *
 * Provides standard string manipulation and memory functions (strlen, memcpy,
 * memset, strcmp, strcpy) for user space applications.
 */

#include "include/string.h"

unsigned long strlen(const char *s) {
    unsigned long len = 0;
    while (*s++)
        len++;
    return len;
}

void *memcpy(void *dest, const void *src, unsigned long n) {
    char *d = (char *)dest;
    const char *s = (const char *)src;
    for (unsigned long i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

void *memset(void *s, int c, unsigned long n) {
    char *p = (char *)s;
    for (unsigned long i = 0; i < n; i++) {
        p[i] = (char)c;
    }
    return s;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

char *strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++))
        ;
    return dest;
}

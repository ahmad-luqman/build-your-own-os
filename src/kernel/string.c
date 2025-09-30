/*
 * MiniOS Kernel String Utilities
 * Basic string functions for kernel use
 */

#include "kernel.h"

int strlen(const char *str)
{
    int len = 0;
    if (!str) return 0;
    while (str[len]) len++;
    return len;
}

int strcmp(const char *s1, const char *s2)
{
    if (!s1 || !s2) return -1;
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    if (!s1 || !s2 || n == 0) return 0;
    while (n-- && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    if (n == (size_t)-1) return 0;
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

char *strcpy(char *dest, const char *src)
{
    if (!dest || !src) return dest;
    char *orig_dest = dest;
    while ((*dest++ = *src++));
    return orig_dest;
}

char *strncpy(char *dest, const char *src, size_t n)
{
    if (!dest || !src) return dest;
    char *orig_dest = dest;
    while (n-- && (*dest++ = *src++));
    while (n-- > 0) *dest++ = '\0';
    return orig_dest;
}

char *strcat(char *dest, const char *src)
{
    if (!dest || !src) return dest;
    char *orig_dest = dest;
    while (*dest) dest++;  // Find end of dest
    while ((*dest++ = *src++));
    return orig_dest;
}

char *strchr(const char *str, int c)
{
    if (!str) return NULL;
    while (*str) {
        if (*str == c) return (char*)str;
        str++;
    }
    return (c == '\0') ? (char*)str : NULL;
}

char *strrchr(const char *str, int c)
{
    if (!str) return NULL;
    const char *last = NULL;
    while (*str) {
        if (*str == c) last = str;
        str++;
    }
    return (c == '\0') ? (char*)str : (char*)last;
}

void *memset(void *s, int c, size_t n)
{
    if (!s) return s;
    unsigned char *p = (unsigned char*)s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

void *memcpy(void *dest, const void *src, size_t n)
{
    if (!dest || !src) return dest;
    unsigned char *d = (unsigned char*)dest;
    const unsigned char *s = (const unsigned char*)src;
    while (n--) *d++ = *s++;
    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
    if (!s1 || !s2) return 0;
    const unsigned char *p1 = (const unsigned char*)s1;
    const unsigned char *p2 = (const unsigned char*)s2;
    while (n--) {
        if (*p1 != *p2) return *p1 - *p2;
        p1++;
        p2++;
    }
    return 0;
}
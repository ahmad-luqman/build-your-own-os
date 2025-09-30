#ifndef MINIOS_STRING_H
#define MINIOS_STRING_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Memory functions
void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
void *memchr(const void *s, int c, size_t n);

// String functions
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
char *strcat(char *dest, const char *src);
char *strncat(char *dest, const char *src, size_t n);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
int strcoll(const char *s1, const char *s2);
size_t strxfrm(char *dest, const char *src, size_t n);

// String search functions
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
size_t strcspn(const char *s1, const char *s2);
size_t strspn(const char *s1, const char *s2);
char *strpbrk(const char *s1, const char *s2);
char *strstr(const char *haystack, const char *needle);
char *strtok(char *str, const char *delim);

// String properties
size_t strlen(const char *s);

// Error functions
char *strerror(int errnum);

// Additional POSIX functions (commonly used)
char *strdup(const char *s);
int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, size_t n);

#ifdef __cplusplus
}
#endif

#endif /* MINIOS_STRING_H */
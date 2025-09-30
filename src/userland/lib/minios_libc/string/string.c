#include "../string.h"
#include "../stdlib.h"

// Memory functions
void *memcpy(void *dest, const void *src, size_t n) {
    char *d = (char *)dest;
    const char *s = (const char *)src;
    
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    
    return dest;
}

void *memmove(void *dest, const void *src, size_t n) {
    char *d = (char *)dest;
    const char *s = (const char *)src;
    
    if (d < s) {
        // Forward copy
        for (size_t i = 0; i < n; i++) {
            d[i] = s[i];
        }
    } else if (d > s) {
        // Backward copy
        for (size_t i = n; i > 0; i--) {
            d[i - 1] = s[i - 1];
        }
    }
    
    return dest;
}

void *memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    unsigned char value = (unsigned char)c;
    
    for (size_t i = 0; i < n; i++) {
        p[i] = value;
    }
    
    return s;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;
    
    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }
    
    return 0;
}

void *memchr(const void *s, int c, size_t n) {
    const unsigned char *p = (const unsigned char *)s;
    unsigned char value = (unsigned char)c;
    
    for (size_t i = 0; i < n; i++) {
        if (p[i] == value) {
            return (void *)(p + i);
        }
    }
    
    return NULL;
}

// String functions
char *strcpy(char *dest, const char *src) {
    if (!dest || !src) return dest;
    
    char *d = dest;
    while ((*d++ = *src++) != '\0') {
        // Copy characters until null terminator
    }
    
    return dest;
}

char *strncpy(char *dest, const char *src, size_t n) {
    if (!dest || !src) return dest;
    
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    
    // Pad with null bytes if necessary
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    
    return dest;
}

char *strcat(char *dest, const char *src) {
    if (!dest || !src) return dest;
    
    char *d = dest;
    
    // Find end of destination string
    while (*d) d++;
    
    // Append source string
    while ((*d++ = *src++) != '\0') {
        // Copy characters until null terminator
    }
    
    return dest;
}

char *strncat(char *dest, const char *src, size_t n) {
    if (!dest || !src) return dest;
    
    char *d = dest;
    
    // Find end of destination string
    while (*d) d++;
    
    // Append up to n characters from source
    for (size_t i = 0; i < n && src[i] != '\0'; i++) {
        *d++ = src[i];
    }
    
    *d = '\0';  // Ensure null termination
    return dest;
}

int strcmp(const char *s1, const char *s2) {
    if (!s1 || !s2) return 0;
    
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    
    return (unsigned char)*s1 - (unsigned char)*s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    if (!s1 || !s2 || n == 0) return 0;
    
    for (size_t i = 0; i < n; i++) {
        if (s1[i] != s2[i]) {
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        }
        if (s1[i] == '\0') {
            break;
        }
    }
    
    return 0;
}

// String search functions
char *strchr(const char *s, int c) {
    if (!s) return NULL;
    
    char target = (char)c;
    while (*s) {
        if (*s == target) {
            return (char *)s;
        }
        s++;
    }
    
    // Check for null terminator match
    if (target == '\0') {
        return (char *)s;
    }
    
    return NULL;
}

char *strrchr(const char *s, int c) {
    if (!s) return NULL;
    
    const char *last = NULL;
    char target = (char)c;
    
    while (*s) {
        if (*s == target) {
            last = s;
        }
        s++;
    }
    
    // Check for null terminator match
    if (target == '\0') {
        return (char *)s;
    }
    
    return (char *)last;
}

size_t strcspn(const char *s1, const char *s2) {
    if (!s1 || !s2) return 0;
    
    size_t count = 0;
    
    while (s1[count]) {
        const char *p = s2;
        while (*p) {
            if (s1[count] == *p) {
                return count;
            }
            p++;
        }
        count++;
    }
    
    return count;
}

size_t strspn(const char *s1, const char *s2) {
    if (!s1 || !s2) return 0;
    
    size_t count = 0;
    
    while (s1[count]) {
        const char *p = s2;
        int found = 0;
        
        while (*p) {
            if (s1[count] == *p) {
                found = 1;
                break;
            }
            p++;
        }
        
        if (!found) {
            break;
        }
        count++;
    }
    
    return count;
}

char *strstr(const char *haystack, const char *needle) {
    if (!haystack || !needle) return NULL;
    
    if (*needle == '\0') {
        return (char *)haystack;
    }
    
    while (*haystack) {
        const char *h = haystack;
        const char *n = needle;
        
        while (*h && *n && (*h == *n)) {
            h++;
            n++;
        }
        
        if (*n == '\0') {
            return (char *)haystack;
        }
        
        haystack++;
    }
    
    return NULL;
}

// Simple strtok implementation (not thread-safe)
static char *strtok_saved = NULL;

char *strtok(char *str, const char *delim) {
    if (!delim) return NULL;
    
    if (str) {
        strtok_saved = str;
    } else if (!strtok_saved) {
        return NULL;
    }
    
    // Skip leading delimiters
    while (*strtok_saved && strchr(delim, *strtok_saved)) {
        strtok_saved++;
    }
    
    if (*strtok_saved == '\0') {
        return NULL;
    }
    
    char *token_start = strtok_saved;
    
    // Find end of token
    while (*strtok_saved && !strchr(delim, *strtok_saved)) {
        strtok_saved++;
    }
    
    if (*strtok_saved) {
        *strtok_saved = '\0';
        strtok_saved++;
    }
    
    return token_start;
}

// String properties
size_t strlen(const char *s) {
    if (!s) return 0;
    
    size_t len = 0;
    while (s[len]) {
        len++;
    }
    
    return len;
}

// Error functions (stub)
char *strerror(int errnum) {
    static char error_buf[64];
    
    switch (errnum) {
        case 0: return "Success";
        case 1: return "Operation not permitted";
        case 2: return "No such file or directory";
        case 3: return "No such process";
        case 12: return "Out of memory";
        case 22: return "Invalid argument";
        default:
            // Simple error number formatting
            strcpy(error_buf, "Unknown error ");
            // Convert errnum to string (simplified)
            int len = strlen(error_buf);
            if (errnum < 10) {
                error_buf[len] = '0' + errnum;
                error_buf[len + 1] = '\0';
            } else {
                error_buf[len] = '0' + (errnum / 10);
                error_buf[len + 1] = '0' + (errnum % 10);
                error_buf[len + 2] = '\0';
            }
            return error_buf;
    }
}

// Additional functions
char *strdup(const char *s) {
    if (!s) return NULL;
    
    size_t len = strlen(s) + 1;
    char *dup = malloc(len);
    if (dup) {
        memcpy(dup, s, len);
    }
    
    return dup;
}

// Case-insensitive comparison helper
static int _tolower(int c) {
    if (c >= 'A' && c <= 'Z') {
        return c + ('a' - 'A');
    }
    return c;
}

int strcasecmp(const char *s1, const char *s2) {
    if (!s1 || !s2) return 0;
    
    while (*s1 && (_tolower(*s1) == _tolower(*s2))) {
        s1++;
        s2++;
    }
    
    return _tolower(*s1) - _tolower(*s2);
}

int strncasecmp(const char *s1, const char *s2, size_t n) {
    if (!s1 || !s2 || n == 0) return 0;
    
    for (size_t i = 0; i < n; i++) {
        int c1 = _tolower(s1[i]);
        int c2 = _tolower(s2[i]);
        
        if (c1 != c2) {
            return c1 - c2;
        }
        if (s1[i] == '\0') {
            break;
        }
    }
    
    return 0;
}
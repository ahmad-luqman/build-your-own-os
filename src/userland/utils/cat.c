/*
 * Cat - User-space implementation of the cat command
 * Demonstrates file I/O in user programs
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// User-space file I/O functions (would use system calls)
int user_open(const char *path, int flags);
int user_read(int fd, void *buf, size_t count);
int user_close(int fd);
int user_printf(const char *format, ...);
int user_puts(const char *str);

#define BUFFER_SIZE 1024

int cat_file(const char *filename) {
    int fd = user_open(filename, 0);
    if (fd < 0) {
        user_printf("cat: cannot open '%s'\n", filename);
        return -1;
    }
    
    char buffer[BUFFER_SIZE];
    int bytes_read;
    
    while ((bytes_read = user_read(fd, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[bytes_read] = '\0';
        user_printf("%s", buffer);
    }
    
    user_close(fd);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        user_puts("Usage: cat <file1> [file2] ...");
        return 1;
    }
    
    int result = 0;
    for (int i = 1; i < argc; i++) {
        if (cat_file(argv[i]) != 0) {
            result = 1;
        }
    }
    
    return result;
}

// Stub implementations
int user_open(const char *path, int flags) { return -1; }
int user_read(int fd, void *buf, size_t count) { return -1; }
int user_close(int fd) { return -1; }
int user_printf(const char *format, ...) { return 0; }
int user_puts(const char *str) { return 0; }
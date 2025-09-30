/*
 * Ls - User-space implementation of the ls command
 * Demonstrates directory operations in user programs
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// User-space directory functions (would use system calls)
int user_opendir(const char *path);
int user_readdir(int dirfd, char *name, size_t name_len);
int user_closedir(int dirfd);
int user_printf(const char *format, ...);
int user_puts(const char *str);

int list_directory(const char *path) {
    int dirfd = user_opendir(path);
    if (dirfd < 0) {
        user_printf("ls: cannot access '%s'\n", path);
        return -1;
    }
    
    char filename[256];
    while (user_readdir(dirfd, filename, sizeof(filename)) > 0) {
        user_printf("%s\n", filename);
    }
    
    user_closedir(dirfd);
    return 0;
}

int main(int argc, char *argv[]) {
    const char *path = ".";  // Default to current directory
    
    if (argc >= 2) {
        path = argv[1];
    }
    
    user_printf("Contents of '%s':\n", path);
    return list_directory(path);
}

// Stub implementations
int user_opendir(const char *path) { return -1; }
int user_readdir(int dirfd, char *name, size_t name_len) { return -1; }
int user_closedir(int dirfd) { return -1; }
int user_printf(const char *format, ...) { return 0; }
int user_puts(const char *str) { return 0; }
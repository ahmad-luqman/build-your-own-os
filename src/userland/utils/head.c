/*
 * Head - Display first lines of file(s)
 * Shows the first N lines of one or more files
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// User I/O functions
int user_printf(const char *format, ...);
int user_puts(const char *str);
int user_open(const char *path, int flags);
int user_read(int fd, void *buf, size_t count);
int user_close(int fd);

#define DEFAULT_LINES   10
#define BUFFER_SIZE     1024

// String to integer conversion
int str_to_int(const char *str) {
    int result = 0;
    int i = 0;
    
    if (str[0] == '-') {
        i = 1;
    }
    
    while (str[i] >= '0' && str[i] <= '9') {
        result = result * 10 + (str[i] - '0');
        i++;
    }
    
    return result;
}

// Display first N lines of a file
int head_file(const char *filename, int lines) {
    int fd = user_open(filename, 0);
    if (fd < 0) {
        user_printf("head: cannot open '%s'\n", filename);
        return -1;
    }
    
    char buffer[BUFFER_SIZE];
    int bytes_read;
    int lines_shown = 0;
    
    while (lines_shown < lines && (bytes_read = user_read(fd, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[bytes_read] = '\0';
        
        for (int i = 0; i < bytes_read && lines_shown < lines; i++) {
            user_printf("%c", buffer[i]);
            if (buffer[i] == '\n') {
                lines_shown++;
            }
        }
    }
    
    user_close(fd);
    return 0;
}

int main(int argc, char *argv[]) {
    int lines = DEFAULT_LINES;
    int file_start = 1;
    
    // Parse arguments
    if (argc >= 2 && argv[1][0] == '-') {
        if (argv[1][1] >= '0' && argv[1][1] <= '9') {
            // -N format
            lines = str_to_int(&argv[1][1]);
        } else if (argc >= 3 && strcmp(argv[1], "-n") == 0) {
            // -n N format
            lines = str_to_int(argv[2]);
            file_start = 3;
        } else {
            user_puts("Usage: head [-n lines | -lines] [file1] [file2] ...");
            user_puts("       head [file1] [file2] ...");
            return 1;
        }
        file_start = 2;
        
        if (argc >= 3 && strcmp(argv[1], "-n") == 0) {
            file_start = 3;
        }
    }
    
    if (file_start >= argc) {
        user_puts("Usage: head [-n lines | -lines] [file1] [file2] ...");
        return 1;
    }
    
    // Process files
    for (int i = file_start; i < argc; i++) {
        if (argc - file_start > 1) {
            user_printf("==> %s <==\n", argv[i]);
        }
        
        if (head_file(argv[i], lines) != 0) {
            continue;
        }
        
        if (i < argc - 1) {
            user_puts("\n");
        }
    }
    
    return 0;
}

// Stub implementations
int user_printf(const char *format, ...) { return 0; }
int user_puts(const char *str) { return 0; }
int user_open(const char *path, int flags) { return -1; }
int user_read(int fd, void *buf, size_t count) { return -1; }
int user_close(int fd) { return -1; }
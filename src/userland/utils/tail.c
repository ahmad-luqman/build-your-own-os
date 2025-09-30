/*
 * Tail - Display last lines of file(s)
 * Shows the last N lines of one or more files
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
int user_lseek(int fd, int offset, int whence);

#define DEFAULT_LINES   10
#define BUFFER_SIZE     1024
#define MAX_LINES       1000

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

// Simple line buffer for storing last N lines
struct line_buffer {
    char lines[MAX_LINES][256];
    int count;
    int start;
    int max_lines;
};

void line_buffer_init(struct line_buffer *lb, int max_lines) {
    lb->count = 0;
    lb->start = 0;
    lb->max_lines = max_lines;
}

void line_buffer_add(struct line_buffer *lb, const char *line) {
    int pos = (lb->start + lb->count) % lb->max_lines;
    
    strncpy(lb->lines[pos], line, 255);
    lb->lines[pos][255] = '\0';
    
    if (lb->count < lb->max_lines) {
        lb->count++;
    } else {
        lb->start = (lb->start + 1) % lb->max_lines;
    }
}

void line_buffer_print(struct line_buffer *lb) {
    for (int i = 0; i < lb->count; i++) {
        int pos = (lb->start + i) % lb->max_lines;
        user_puts(lb->lines[pos]);
    }
}

// Display last N lines of a file
int tail_file(const char *filename, int lines) {
    int fd = user_open(filename, 0);
    if (fd < 0) {
        user_printf("tail: cannot open '%s'\n", filename);
        return -1;
    }
    
    struct line_buffer lb;
    line_buffer_init(&lb, lines);
    
    char buffer[BUFFER_SIZE];
    char current_line[256];
    int line_pos = 0;
    int bytes_read;
    
    while ((bytes_read = user_read(fd, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[bytes_read] = '\0';
        
        for (int i = 0; i < bytes_read; i++) {
            if (buffer[i] == '\n') {
                current_line[line_pos] = '\n';
                current_line[line_pos + 1] = '\0';
                line_buffer_add(&lb, current_line);
                line_pos = 0;
            } else if (line_pos < 255) {
                current_line[line_pos++] = buffer[i];
            }
        }
    }
    
    // Add any remaining line
    if (line_pos > 0) {
        current_line[line_pos] = '\n';
        current_line[line_pos + 1] = '\0';
        line_buffer_add(&lb, current_line);
    }
    
    // Print the last lines
    line_buffer_print(&lb);
    
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
            user_puts("Usage: tail [-n lines | -lines] [file1] [file2] ...");
            user_puts("       tail [file1] [file2] ...");
            return 1;
        }
        file_start = 2;
        
        if (argc >= 3 && strcmp(argv[1], "-n") == 0) {
            file_start = 3;
        }
    }
    
    if (file_start >= argc) {
        user_puts("Usage: tail [-n lines | -lines] [file1] [file2] ...");
        return 1;
    }
    
    // Process files
    for (int i = file_start; i < argc; i++) {
        if (argc - file_start > 1) {
            user_printf("==> %s <==\n", argv[i]);
        }
        
        if (tail_file(argv[i], lines) != 0) {
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
int user_lseek(int fd, int offset, int whence) { return -1; }
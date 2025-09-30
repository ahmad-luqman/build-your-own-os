#include "../stdio.h"
#include "../stdlib.h"
#include "../string.h"
#include <stdarg.h>

// System call interface (will be provided by kernel)
extern int sys_open(const char *path, int flags);
extern int sys_close(int fd);
extern ssize_t sys_read(int fd, void *buffer, size_t count);
extern ssize_t sys_write(int fd, const void *buffer, size_t count);
extern int sys_lseek(int fd, off_t offset, int whence);

// Standard streams (initialized by runtime)
static FILE _stdin = {0, 0, 0, NULL, 0, 0};
static FILE _stdout = {1, 0, 0, NULL, 0, 0};
static FILE _stderr = {2, 0, 0, NULL, 0, 0};

FILE *stdin = &_stdin;
FILE *stdout = &_stdout;
FILE *stderr = &_stderr;

// Internal helper functions
static int _file_mode_to_flags(const char *mode) {
    if (!mode) return -1;
    
    switch (mode[0]) {
        case 'r':
            return mode[1] == '+' ? 0x02 : 0x00;  // O_RDONLY or O_RDWR
        case 'w':
            return mode[1] == '+' ? 0x42 : 0x41;  // O_WRONLY|O_CREAT or O_RDWR|O_CREAT
        case 'a':
            return mode[1] == '+' ? 0x402 : 0x401; // O_WRONLY|O_CREAT|O_APPEND or O_RDWR|O_CREAT|O_APPEND
        default:
            return -1;
    }
}

// File operations
FILE *fopen(const char *filename, const char *mode) {
    if (!filename || !mode) {
        return NULL;
    }
    
    int flags = _file_mode_to_flags(mode);
    if (flags < 0) {
        return NULL;
    }
    
    int fd = sys_open(filename, flags);
    if (fd < 0) {
        return NULL;
    }
    
    FILE *file = malloc(sizeof(FILE));
    if (!file) {
        sys_close(fd);
        return NULL;
    }
    
    file->fd = fd;
    file->flags = flags;
    file->error = 0;
    file->buffer = NULL;
    file->buffer_size = 0;
    file->buffer_pos = 0;
    
    return file;
}

int fclose(FILE *stream) {
    if (!stream || stream == stdin || stream == stdout || stream == stderr) {
        return EOF;
    }
    
    int result = sys_close(stream->fd);
    
    if (stream->buffer) {
        free(stream->buffer);
    }
    
    free(stream);
    return result;
}

int fflush(FILE *stream) {
    // For now, we don't implement buffering, so this is a no-op
    (void)stream;
    return 0;
}

// Character I/O
int fgetc(FILE *stream) {
    if (!stream) return EOF;
    
    char c;
    ssize_t result = sys_read(stream->fd, &c, 1);
    if (result != 1) {
        return EOF;
    }
    
    return (unsigned char)c;
}

int fputc(int character, FILE *stream) {
    if (!stream) return EOF;
    
    char c = (char)character;
    ssize_t result = sys_write(stream->fd, &c, 1);
    if (result != 1) {
        return EOF;
    }
    
    return (unsigned char)c;
}

int getchar(void) {
    return fgetc(stdin);
}

int putchar(int character) {
    return fputc(character, stdout);
}

int puts(const char *str) {
    if (!str) return EOF;
    
    // Write the string
    size_t len = strlen(str);
    if (sys_write(stdout->fd, str, len) != (ssize_t)len) {
        return EOF;
    }
    
    // Write newline
    if (sys_write(stdout->fd, "\n", 1) != 1) {
        return EOF;
    }
    
    return (int)len + 1;
}

int fputs(const char *str, FILE *stream) {
    if (!str || !stream) return EOF;
    
    size_t len = strlen(str);
    if (sys_write(stream->fd, str, len) != (ssize_t)len) {
        return EOF;
    }
    
    return (int)len;
}

// Block I/O
size_t fread(void *ptr, size_t size, size_t count, FILE *stream) {
    if (!ptr || !stream || size == 0) {
        return 0;
    }
    
    size_t total_bytes = size * count;
    ssize_t bytes_read = sys_read(stream->fd, ptr, total_bytes);
    
    if (bytes_read < 0) {
        return 0;
    }
    
    return (size_t)bytes_read / size;
}

size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream) {
    if (!ptr || !stream || size == 0) {
        return 0;
    }
    
    size_t total_bytes = size * count;
    ssize_t bytes_written = sys_write(stream->fd, ptr, total_bytes);
    
    if (bytes_written < 0) {
        return 0;
    }
    
    return (size_t)bytes_written / size;
}

// File positioning
int fseek(FILE *stream, long offset, int origin) {
    if (!stream) return -1;
    
    return sys_lseek(stream->fd, offset, origin);
}

long ftell(FILE *stream) {
    if (!stream) return -1;
    
    return sys_lseek(stream->fd, 0, SEEK_CUR);
}

void rewind(FILE *stream) {
    if (stream) {
        sys_lseek(stream->fd, 0, SEEK_SET);
    }
}

// Error handling  
int feof(FILE *stream) {
    // Simplified implementation
    (void)stream;
    return 0;
}

int ferror(FILE *stream) {
    return stream ? stream->error : 1;
}

void clearerr(FILE *stream) {
    if (stream) {
        stream->error = 0;
    }
}

// Simple printf implementation
int printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    // Very basic printf - only supports %s, %d, %c, %x
    int count = 0;
    const char *p = format;
    
    while (*p) {
        if (*p == '%' && *(p + 1)) {
            p++;
            switch (*p) {
                case 's': {
                    const char *str = va_arg(args, const char*);
                    if (str) {
                        count += fputs(str, stdout);
                    }
                    break;
                }
                case 'd': {
                    int num = va_arg(args, int);
                    char buf[32];
                    // Simple integer to string conversion
                    int i = 0, negative = 0;
                    if (num < 0) {
                        negative = 1;
                        num = -num;
                    }
                    do {
                        buf[i++] = '0' + (num % 10);
                        num /= 10;
                    } while (num);
                    if (negative) buf[i++] = '-';
                    
                    // Reverse and print
                    for (int j = i - 1; j >= 0; j--) {
                        putchar(buf[j]);
                        count++;
                    }
                    break;
                }
                case 'c': {
                    int c = va_arg(args, int);
                    putchar(c);
                    count++;
                    break;
                }
                case 'x': {
                    unsigned int num = va_arg(args, unsigned int);
                    char buf[32];
                    int i = 0;
                    if (num == 0) {
                        buf[i++] = '0';
                    } else {
                        while (num) {
                            int digit = num % 16;
                            buf[i++] = digit < 10 ? '0' + digit : 'a' + digit - 10;
                            num /= 16;
                        }
                    }
                    
                    // Reverse and print
                    for (int j = i - 1; j >= 0; j--) {
                        putchar(buf[j]);
                        count++;
                    }
                    break;
                }
                case '%':
                    putchar('%');
                    count++;
                    break;
            }
        } else {
            putchar(*p);
            count++;
        }
        p++;
    }
    
    va_end(args);
    return count;
}
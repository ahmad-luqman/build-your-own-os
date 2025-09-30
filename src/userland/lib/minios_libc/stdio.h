#ifndef MINIOS_STDIO_H
#define MINIOS_STDIO_H

#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

// Type definitions
typedef long long fpos_t;       // File position type
typedef long ssize_t;           // Signed size type
typedef long off_t;             // File offset type

// File handle structure
typedef struct {
    int fd;                     // File descriptor
    int flags;                  // File flags
    int error;                  // Error state
    char *buffer;               // Internal buffer
    size_t buffer_size;         // Buffer size
    size_t buffer_pos;          // Current position in buffer
} FILE;

// Standard streams
extern FILE *stdin;
extern FILE *stdout; 
extern FILE *stderr;

// Standard constants
#define EOF         (-1)
#define BUFSIZ      1024
#define FILENAME_MAX 256

// File modes
#define _IOFBF      0           // Full buffering
#define _IOLBF      1           // Line buffering
#define _IONBF      2           // No buffering

// Seek constants
#define SEEK_SET    0           // Seek from beginning
#define SEEK_CUR    1           // Seek from current position
#define SEEK_END    2           // Seek from end

// File operations
FILE *fopen(const char *filename, const char *mode);
FILE *freopen(const char *filename, const char *mode, FILE *stream);
int fclose(FILE *stream);
int fflush(FILE *stream);

// Character I/O
int fgetc(FILE *stream);
char *fgets(char *str, int num, FILE *stream);
int fputc(int character, FILE *stream);
int fputs(const char *str, FILE *stream);
int getc(FILE *stream);
int getchar(void);
int putc(int character, FILE *stream);
int putchar(int character);
int puts(const char *str);
int ungetc(int character, FILE *stream);

// Block I/O
size_t fread(void *ptr, size_t size, size_t count, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream);

// File positioning
int fgetpos(FILE *stream, fpos_t *pos);
int fseek(FILE *stream, long offset, int origin);
int fsetpos(FILE *stream, const fpos_t *pos);
long ftell(FILE *stream);
void rewind(FILE *stream);

// Error handling
void clearerr(FILE *stream);
int feof(FILE *stream);
int ferror(FILE *stream);
void perror(const char *str);

// Formatted I/O
int fprintf(FILE *stream, const char *format, ...);
int fscanf(FILE *stream, const char *format, ...);
int printf(const char *format, ...);
int scanf(const char *format, ...);
int snprintf(char *str, size_t size, const char *format, ...);
int sprintf(char *str, const char *format, ...);
int sscanf(const char *str, const char *format, ...);
int vfprintf(FILE *stream, const char *format, va_list arg);
int vprintf(const char *format, va_list arg);
int vsnprintf(char *str, size_t size, const char *format, va_list arg);
int vsprintf(char *str, const char *format, va_list arg);

// File operations
int remove(const char *filename);
int rename(const char *oldname, const char *newname);
FILE *tmpfile(void);
char *tmpnam(char *str);

// Buffer operations
int setvbuf(FILE *stream, char *buffer, int mode, size_t size);
void setbuf(FILE *stream, char *buffer);

#ifdef __cplusplus
}
#endif

#endif /* MINIOS_STDIO_H */
#ifndef FD_H
#define FD_H

// Include kernel standard types
#include "kernel.h"
#include "vfs.h"

#ifdef __cplusplus
extern "C" {
#endif

// Maximum number of open files per process
#define MAX_OPEN_FILES      32

// Standard file descriptors
#define STDIN_FD           0
#define STDOUT_FD          1
#define STDERR_FD          2

// File descriptor flags
#define FD_FLAG_USED       0x01
#define FD_FLAG_CLOEXEC    0x02

// File descriptor structure
struct file_descriptor {
    int flags;                              // FD flags
    struct file *file;                      // Associated file
    int open_flags;                         // Open flags (O_RDONLY, etc.)
    int mode;                               // File mode
};

// File descriptor table (per process)
struct fd_table {
    struct file_descriptor fds[MAX_OPEN_FILES];
    int next_fd;                            // Hint for next available FD
    int ref_count;                          // Reference count for sharing
};

// File descriptor management functions
int fd_init(void);
struct fd_table *fd_table_create(void);
void fd_table_destroy(struct fd_table *table);
struct fd_table *fd_table_clone(struct fd_table *src);

// File descriptor operations
int fd_allocate(struct fd_table *table);
void fd_free(struct fd_table *table, int fd);
struct file_descriptor *fd_get(struct fd_table *table, int fd);
int fd_assign(struct fd_table *table, int fd, struct file *file, int flags);
int fd_is_valid(struct fd_table *table, int fd);

// Process-level FD operations (uses current process FD table)
int fd_open(const char *path, int flags, int mode);
ssize_t fd_read(int fd, void *buf, size_t count);
ssize_t fd_write(int fd, const void *buf, size_t count);
off_t fd_seek(int fd, off_t offset, int whence);
int fd_close(int fd);
int fd_sync(int fd);

// File descriptor utilities
void fd_dump_table(struct fd_table *table);
int fd_count_open(struct fd_table *table);

// Standard file descriptor setup
int fd_setup_stdio(void);

#ifdef __cplusplus
}
#endif

#endif /* FD_H */
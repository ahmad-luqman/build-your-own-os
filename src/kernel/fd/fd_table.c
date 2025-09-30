/*
 * MiniOS File Descriptor Management
 * Per-process file descriptor tables
 */

#include "fd.h"
#include "memory.h"
#include "kernel.h"
#include "process.h"

// Global FD system state
static int fd_initialized = 0;
static struct fd_table *current_fd_table = NULL;

int fd_init(void)
{
    if (fd_initialized) {
        return VFS_SUCCESS;
    }
    
    early_print("Initializing file descriptor system...\n");
    
    // Create initial FD table (for kernel/init process)
    current_fd_table = fd_table_create();
    if (!current_fd_table) {
        early_print("Failed to create initial FD table\n");
        return VFS_ENOMEM;
    }
    
    fd_initialized = 1;
    early_print("File descriptor system initialized\n");
    
    return VFS_SUCCESS;
}

struct fd_table *fd_table_create(void)
{
    struct fd_table *table = kmalloc(sizeof(struct fd_table));
    if (!table) {
        return NULL;
    }
    
    // Initialize all file descriptors as unused
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        table->fds[i].flags = 0;  // Not FD_FLAG_USED
        table->fds[i].file = NULL;
        table->fds[i].open_flags = 0;
        table->fds[i].mode = 0;
    }
    
    table->next_fd = 0;
    table->ref_count = 1;
    
    return table;
}

void fd_table_destroy(struct fd_table *table)
{
    if (!table) {
        return;
    }
    
    // Close all open files
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (table->fds[i].flags & FD_FLAG_USED) {
            fd_free(table, i);
        }
    }
    
    kfree(table);
}

struct fd_table *fd_table_clone(struct fd_table *src)
{
    if (!src) {
        return NULL;
    }
    
    struct fd_table *new_table = fd_table_create();
    if (!new_table) {
        return NULL;
    }
    
    // Copy file descriptors
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (src->fds[i].flags & FD_FLAG_USED) {
            new_table->fds[i] = src->fds[i];
            
            // Increment file reference count
            if (new_table->fds[i].file) {
                new_table->fds[i].file->ref_count++;
            }
        }
    }
    
    new_table->next_fd = src->next_fd;
    
    return new_table;
}

int fd_allocate(struct fd_table *table)
{
    if (!table) {
        return -1;
    }
    
    // Search for unused file descriptor starting from hint
    int start = table->next_fd;
    int fd = start;
    
    do {
        if (!(table->fds[fd].flags & FD_FLAG_USED)) {
            table->fds[fd].flags |= FD_FLAG_USED;
            table->next_fd = (fd + 1) % MAX_OPEN_FILES;
            return fd;
        }
        
        fd = (fd + 1) % MAX_OPEN_FILES;
    } while (fd != start);
    
    // No free file descriptors
    return -1;
}

void fd_free(struct fd_table *table, int fd)
{
    if (!table || fd < 0 || fd >= MAX_OPEN_FILES) {
        return;
    }
    
    if (table->fds[fd].flags & FD_FLAG_USED) {
        // Close associated file if any
        if (table->fds[fd].file) {
            struct file *file = table->fds[fd].file;
            
            // Decrement reference count
            if (file->ref_count > 0) {
                file->ref_count--;
            }
            
            // Close file if no more references
            if (file->ref_count == 0 && file->ops && file->ops->close) {
                file->ops->close(file);
                kfree(file);
            }
        }
        
        // Clear file descriptor
        table->fds[fd].flags = 0;
        table->fds[fd].file = NULL;
        table->fds[fd].open_flags = 0;
        table->fds[fd].mode = 0;
    }
}

struct file_descriptor *fd_get(struct fd_table *table, int fd)
{
    if (!table || fd < 0 || fd >= MAX_OPEN_FILES) {
        return NULL;
    }
    
    if (table->fds[fd].flags & FD_FLAG_USED) {
        return &table->fds[fd];
    }
    
    return NULL;
}

int fd_assign(struct fd_table *table, int fd, struct file *file, int flags)
{
    if (!table || !file || fd < 0 || fd >= MAX_OPEN_FILES) {
        return VFS_EINVAL;
    }
    
    if (!(table->fds[fd].flags & FD_FLAG_USED)) {
        return VFS_EINVAL;  // FD not allocated
    }
    
    table->fds[fd].file = file;
    table->fds[fd].open_flags = flags;
    table->fds[fd].mode = 0644;  // Default mode
    
    // Increment file reference count
    file->ref_count++;
    
    return VFS_SUCCESS;
}

int fd_is_valid(struct fd_table *table, int fd)
{
    if (!table || fd < 0 || fd >= MAX_OPEN_FILES) {
        return 0;
    }
    
    return (table->fds[fd].flags & FD_FLAG_USED) != 0;
}

// Process-level FD operations (use current process FD table)
static struct fd_table *get_current_fd_table(void)
{
    // In a full implementation, this would get the FD table from the current process
    // For now, use the global FD table
    return current_fd_table;
}

int fd_open(const char *path, int flags, int mode)
{
    if (!fd_initialized || !path) {
        return -1;
    }
    
    struct fd_table *table = get_current_fd_table();
    if (!table) {
        return -1;
    }
    
    // Allocate file descriptor
    int fd = fd_allocate(table);
    if (fd < 0) {
        return -1;  // No free file descriptors
    }
    
    // Open file through VFS
    int vfs_fd = vfs_open(path, flags, mode);
    if (vfs_fd < 0) {
        fd_free(table, fd);
        return -1;  // VFS open failed
    }
    
    // For now, just create a dummy file structure
    // In a full implementation, VFS would return a file structure
    struct file *file = kmalloc(sizeof(struct file));
    if (!file) {
        vfs_close(vfs_fd);
        fd_free(table, fd);
        return -1;
    }
    
    // Initialize file structure
    file->inode = NULL;  // Would be set by VFS
    file->fs = NULL;     // Would be set by VFS
    file->position = 0;
    file->flags = flags;
    file->mode = mode;
    file->ref_count = 0; // Will be incremented by fd_assign
    file->ops = NULL;    // Would be set by filesystem
    
    // Assign file to FD
    if (fd_assign(table, fd, file, flags) != VFS_SUCCESS) {
        kfree(file);
        vfs_close(vfs_fd);
        fd_free(table, fd);
        return -1;
    }
    
    return fd;
}

ssize_t fd_read(int fd, void *buf, size_t count)
{
    if (!fd_initialized || !buf || count == 0) {
        return -1;
    }
    
    struct fd_table *table = get_current_fd_table();
    if (!table) {
        return -1;
    }
    
    struct file_descriptor *fdesc = fd_get(table, fd);
    if (!fdesc || !fdesc->file) {
        return -1;
    }
    
    // Check if file is readable
    if ((fdesc->open_flags & VFS_O_WRONLY) == VFS_O_WRONLY) {
        return -1;  // Write-only file
    }
    
    struct file *file = fdesc->file;
    if (file->ops && file->ops->read) {
        ssize_t result = file->ops->read(file, buf, count, file->position);
        if (result > 0) {
            file->position += result;
        }
        return result;
    }
    
    // For now, return VFS read (placeholder)
    return vfs_read(fd, buf, count);
}

ssize_t fd_write(int fd, const void *buf, size_t count)
{
    if (!fd_initialized || !buf || count == 0) {
        return -1;
    }
    
    struct fd_table *table = get_current_fd_table();
    if (!table) {
        return -1;
    }
    
    struct file_descriptor *fdesc = fd_get(table, fd);
    if (!fdesc || !fdesc->file) {
        return -1;
    }
    
    // Check if file is writable
    if ((fdesc->open_flags & VFS_O_RDONLY) == VFS_O_RDONLY) {
        return -1;  // Read-only file
    }
    
    struct file *file = fdesc->file;
    if (file->ops && file->ops->write) {
        ssize_t result = file->ops->write(file, buf, count, file->position);
        if (result > 0) {
            file->position += result;
        }
        return result;
    }
    
    // For now, return VFS write (placeholder)
    return vfs_write(fd, buf, count);
}

off_t fd_seek(int fd, off_t offset, int whence)
{
    if (!fd_initialized) {
        return -1;
    }
    
    struct fd_table *table = get_current_fd_table();
    if (!table) {
        return -1;
    }
    
    struct file_descriptor *fdesc = fd_get(table, fd);
    if (!fdesc || !fdesc->file) {
        return -1;
    }
    
    struct file *file = fdesc->file;
    if (file->ops && file->ops->seek) {
        return file->ops->seek(file, offset, whence);
    }
    
    // Simple seek implementation
    switch (whence) {
        case VFS_SEEK_SET:
            file->position = offset;
            break;
        case VFS_SEEK_CUR:
            file->position += offset;
            break;
        case VFS_SEEK_END:
            // Would need file size from inode
            file->position = offset;  // Placeholder
            break;
        default:
            return -1;
    }
    
    return file->position;
}

int fd_close(int fd)
{
    if (!fd_initialized) {
        return -1;
    }
    
    struct fd_table *table = get_current_fd_table();
    if (!table) {
        return -1;
    }
    
    if (!fd_is_valid(table, fd)) {
        return -1;
    }
    
    fd_free(table, fd);
    return 0;
}

int fd_sync(int fd)
{
    if (!fd_initialized) {
        return -1;
    }
    
    struct fd_table *table = get_current_fd_table();
    if (!table) {
        return -1;
    }
    
    struct file_descriptor *fdesc = fd_get(table, fd);
    if (!fdesc || !fdesc->file) {
        return -1;
    }
    
    struct file *file = fdesc->file;
    if (file->ops && file->ops->sync) {
        return file->ops->sync(file);
    }
    
    return 0;  // No sync operation available
}

void fd_dump_table(struct fd_table *table)
{
    if (!table) {
        early_print("FD table: NULL\n");
        return;
    }
    
    early_print("File Descriptor Table:\n");
    
    int open_count = 0;
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (table->fds[i].flags & FD_FLAG_USED) {
            early_print("  FD ");
            // Simple number to string
            char num_str[16];
            int val = i;
            int pos = 0;
            if (val == 0) {
                num_str[pos++] = '0';
            } else {
                char temp[16];
                int temp_pos = 0;
                while (val > 0) {
                    temp[temp_pos++] = '0' + (val % 10);
                    val /= 10;
                }
                for (int j = temp_pos - 1; j >= 0; j--) {
                    num_str[pos++] = temp[j];
                }
            }
            num_str[pos] = '\0';
            early_print(num_str);
            early_print(": ");
            
            if (table->fds[i].file) {
                early_print("open");
            } else {
                early_print("allocated");
            }
            early_print("\n");
            
            open_count++;
        }
    }
    
    if (open_count == 0) {
        early_print("  No open files\n");
    }
}

int fd_count_open(struct fd_table *table)
{
    if (!table) {
        return 0;
    }
    
    int count = 0;
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (table->fds[i].flags & FD_FLAG_USED) {
            count++;
        }
    }
    
    return count;
}

int fd_setup_stdio(void)
{
    // For now, just reserve the standard file descriptors
    // In a full implementation, these would be connected to console devices
    
    struct fd_table *table = get_current_fd_table();
    if (!table) {
        return VFS_ERROR;
    }
    
    // Allocate standard FDs
    int stdin_fd = fd_allocate(table);
    int stdout_fd = fd_allocate(table);
    int stderr_fd = fd_allocate(table);
    
    if (stdin_fd != STDIN_FD || stdout_fd != STDOUT_FD || stderr_fd != STDERR_FD) {
        early_print("Warning: Standard FDs not allocated in expected order\n");
    }
    
    early_print("Standard file descriptors allocated\n");
    return VFS_SUCCESS;
}

// Public function to get current fd table
struct fd_table *fd_get_current_table(void)
{
    return get_current_fd_table();
}
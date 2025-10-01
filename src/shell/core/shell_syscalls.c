/*
 * MiniOS Shell System Calls
 * Shell-specific system call implementations
 */

#include "shell.h"
#include "kernel.h"
#include "syscall.h"
#include "vfs.h"

// Get current working directory
long syscall_getcwd(long buf_ptr, long size, long unused2, long unused3, long unused4, long unused5)
{
    (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    
    char *buffer = (char *)buf_ptr;
    size_t buffer_size = (size_t)size;
    
    if (!buffer || buffer_size == 0) {
        return SYSCALL_EINVAL;
    }
    
    // For now, return root directory
    // In the future, this would get the actual current working directory from process context
    const char *cwd = "/";
    size_t cwd_len = strlen(cwd);
    
    if (cwd_len >= buffer_size) {
        return SYSCALL_EINVAL;  // Buffer too small
    }
    
    strcpy(buffer, cwd);
    return cwd_len;
}

// Change directory
long syscall_chdir(long path_ptr, long unused1, long unused2, long unused3, long unused4, long unused5)
{
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    
    const char *path = (const char *)path_ptr;
    
    if (!path) {
        return SYSCALL_EINVAL;
    }
    
    // For now, just validate path length
    // In the future, this would check if directory exists and update process working directory
    if (strlen(path) >= VFS_MAX_PATH) {
        return SYSCALL_EINVAL;
    }
    
    // Simulate successful directory change
    return SYSCALL_SUCCESS;
}

// Get file/directory information
long syscall_stat(long path_ptr, long stat_buf_ptr, long unused2, long unused3, long unused4, long unused5)
{
    (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    
    const char *path = (const char *)path_ptr;
    void *stat_buf = (void *)stat_buf_ptr;
    
    if (!path || !stat_buf) {
        return SYSCALL_EINVAL;
    }
    
    // For now, just return success for any path
    // In the future, this would populate stat structure with file information
    return SYSCALL_SUCCESS;
}

// Read directory entries
long syscall_readdir(long fd, long entries_ptr, long count, long unused3, long unused4, long unused5)
{
    (void)unused3; (void)unused4; (void)unused5;
    
    int file_fd = (int)fd;
    void *entries = (void *)entries_ptr;
    size_t entry_count = (size_t)count;
    
    if (file_fd < 0 || !entries || entry_count == 0) {
        return SYSCALL_EINVAL;
    }
    
    // For now, return 0 entries (empty directory)
    // In the future, this would read actual directory entries
    return 0;
}

// Execute program
long syscall_exec(long path_ptr, long argv_ptr, long unused2, long unused3, long unused4, long unused5)
{
    (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    
    const char *path = (const char *)path_ptr;
    (void)argv_ptr;  // Suppress unused variable warning
    
    if (!path) {
        return SYSCALL_EINVAL;
    }
    
    // For now, just return not implemented
    // In the future, this would load and execute program from filesystem
    return SYSCALL_ENOENT;
}

// Register shell system calls
void register_shell_syscalls(void)
{
    syscall_register(SYSCALL_GETCWD, syscall_getcwd);
    syscall_register(SYSCALL_CHDIR, syscall_chdir);
    syscall_register(SYSCALL_STAT, syscall_stat);
    syscall_register(SYSCALL_READDIR, syscall_readdir);
    syscall_register(SYSCALL_EXEC, syscall_exec);
}
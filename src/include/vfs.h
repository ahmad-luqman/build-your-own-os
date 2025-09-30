#ifndef VFS_H
#define VFS_H

// Include kernel standard types
#include "kernel.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
struct file;
struct file_system;
struct block_device;
struct inode;

// File types
#define VFS_FILE_REGULAR    0x1000
#define VFS_FILE_DIRECTORY  0x4000
#define VFS_FILE_SYMLINK    0x8000

// File flags
#define VFS_O_RDONLY       0x0000
#define VFS_O_WRONLY       0x0001
#define VFS_O_RDWR         0x0002
#define VFS_O_CREAT        0x0100
#define VFS_O_EXCL         0x0200
#define VFS_O_TRUNC        0x1000
#define VFS_O_APPEND       0x2000

// Seek origins
#define VFS_SEEK_SET       0
#define VFS_SEEK_CUR       1
#define VFS_SEEK_END       2

// Maximum filename length
#define VFS_MAX_NAME       255
#define VFS_MAX_PATH       1024

// Error codes
#define VFS_SUCCESS        0
#define VFS_ERROR         -1
#define VFS_EINVAL        -2
#define VFS_ENOENT        -3
#define VFS_EEXIST        -4
#define VFS_EPERM         -5
#define VFS_ENOMEM        -6
#define VFS_ENOSPC        -7
#define VFS_EIO           -8

// File operations structure
struct file_operations {
    int (*open)(struct file *file, int flags, int mode);
    int (*close)(struct file *file);
    ssize_t (*read)(struct file *file, void *buf, size_t count, off_t offset);
    ssize_t (*write)(struct file *file, const void *buf, size_t count, off_t offset);
    int (*ioctl)(struct file *file, unsigned int cmd, unsigned long arg);
    off_t (*seek)(struct file *file, off_t offset, int whence);
    int (*sync)(struct file *file);
};

// Directory operations structure
struct directory_operations {
    int (*readdir)(struct file *dir, void *buffer, size_t buffer_size, off_t *offset);
    int (*mkdir)(struct file_system *fs, const char *path, int mode);
    int (*rmdir)(struct file_system *fs, const char *path);
    struct inode *(*lookup)(struct file_system *fs, struct inode *parent, const char *name);
};

// File system type structure
struct file_system_type {
    const char *name;                       // File system name (e.g., "sfs", "ramfs")
    struct file_system *(*mount)(struct block_device *dev, unsigned long flags);
    void (*unmount)(struct file_system *fs);
    int (*format)(struct block_device *dev);
    struct file_operations *file_ops;
    struct directory_operations *dir_ops;
};

// File system instance
struct file_system {
    struct file_system_type *type;
    struct block_device *device;
    void *private_data;                     // FS-specific data
    struct vfs_mount *mount_point;
    int flags;
};

// File structure
struct file {
    struct inode *inode;                    // Associated inode
    struct file_system *fs;                 // File system
    off_t position;                         // Current file position
    int flags;                              // Open flags
    int mode;                               // File mode
    int ref_count;                          // Reference count
    struct file_operations *ops;            // File operations
};

// Inode structure (simplified)
struct inode {
    uint32_t ino;                          // Inode number
    uint32_t mode;                         // File type and permissions
    uint32_t size;                         // File size
    uint32_t blocks;                       // Number of blocks
    uint32_t created_time;                 // Creation time
    uint32_t modified_time;                // Modification time
    uint32_t accessed_time;                // Access time
    struct file_system *fs;                // File system
    void *private_data;                    // FS-specific inode data
    int ref_count;                         // Reference count
};

// VFS mount structure
struct vfs_mount {
    char mountpoint[VFS_MAX_PATH];         // Mount point path
    struct file_system *fs;                // Mounted file system
    int flags;                             // Mount flags
    struct vfs_mount *next;                // Next mount in list
};

// Directory entry for readdir operations
struct dirent {
    uint32_t ino;                          // Inode number
    uint16_t type;                         // File type
    uint16_t name_len;                     // Name length
    char name[VFS_MAX_NAME];               // Filename
};

// VFS core functions
int vfs_init(void);
int vfs_shutdown(void);

// File system registration
int vfs_register_filesystem(struct file_system_type *fs_type);
int vfs_unregister_filesystem(const char *name);

// Mount/unmount operations
int vfs_mount(const char *device, const char *mountpoint, const char *fstype, unsigned long flags);
int vfs_unmount(const char *mountpoint);
struct file_system *vfs_get_filesystem(const char *path);

// File operations
int vfs_open(const char *path, int flags, int mode);
ssize_t vfs_read(int fd, void *buf, size_t count);
ssize_t vfs_write(int fd, const void *buf, size_t count);
off_t vfs_seek(int fd, off_t offset, int whence);
int vfs_close(int fd);
int vfs_sync(int fd);

// Directory operations
int vfs_mkdir(const char *path, int mode);
int vfs_rmdir(const char *path);
int vfs_readdir(int fd, struct dirent *entries, size_t count);

// File management
int vfs_unlink(const char *path);
int vfs_rename(const char *oldpath, const char *newpath);
int vfs_stat(const char *path, struct inode *stat_buf);

// Path resolution
char *vfs_resolve_path(const char *path);
struct inode *vfs_lookup(const char *path);
struct file_system *vfs_find_mount(const char *path);

// Utility functions
int vfs_is_absolute_path(const char *path);
char *vfs_get_filename(const char *path);
char *vfs_get_dirname(const char *path);

// VFS debugging and information
void vfs_dump_info(void);

#ifdef __cplusplus
}
#endif

#endif /* VFS_H */
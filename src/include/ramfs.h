#ifndef RAMFS_H
#define RAMFS_H

#include "kernel.h"
#include "vfs.h"
#include "block_device.h"

#ifdef __cplusplus
extern "C" {
#endif

// RAMFS constants
#define RAMFS_MAX_NAME       255
#define RAMFS_MAX_FILES      256
#define RAMFS_MAX_FILE_SIZE  (64 * 1024)  // 64KB max file size for now

// RAMFS node structure (file or directory)
struct ramfs_node {
    char name[RAMFS_MAX_NAME + 1];
    uint32_t mode;                        // File type and permissions
    uint32_t size;                        // File size
    uint32_t ino;                         // Inode number
    void *data;                           // File data (for files)
    struct ramfs_node *parent;            // Parent directory
    struct ramfs_node *children;          // First child (for directories)
    struct ramfs_node *next;              // Next sibling
    uint32_t created_time;
    uint32_t modified_time;
    uint32_t accessed_time;
};

// RAMFS filesystem private data
struct ramfs_fs_data {
    struct ramfs_node *root;              // Root directory
    uint32_t next_ino;                    // Next inode number
    uint32_t file_count;                  // Total files/directories
    uint32_t used_memory;                 // Total memory used
};

// RAMFS file system operations
extern struct file_system_type ramfs_fs_type;

// RAMFS initialization and management
int ramfs_init(void);
struct file_system *ramfs_mount(struct block_device *dev, unsigned long flags);
void ramfs_unmount(struct file_system *fs);
int ramfs_format(struct block_device *dev);

// RAMFS node operations
struct ramfs_node *ramfs_create_node(struct ramfs_fs_data *fs_data, const char *name, uint32_t mode);
void ramfs_destroy_node(struct ramfs_node *node);
struct ramfs_node *ramfs_find_node(struct ramfs_node *parent, const char *name);
struct ramfs_node *ramfs_resolve_path(struct ramfs_fs_data *fs_data, const char *path);
int ramfs_add_child(struct ramfs_node *parent, struct ramfs_node *child);
int ramfs_remove_child(struct ramfs_node *parent, const char *name);

// RAMFS file operations
int ramfs_create_file(struct file_system *fs, const char *path, uint32_t mode);
int ramfs_delete_file(struct file_system *fs, const char *path);
ssize_t ramfs_read_file(struct file *file, void *buf, size_t count, off_t offset);
ssize_t ramfs_write_file(struct file *file, const void *buf, size_t count, off_t offset);

// RAMFS directory operations
int ramfs_create_directory(struct file_system *fs, const char *path, uint32_t mode);
int ramfs_delete_directory(struct file_system *fs, const char *path);
struct inode *ramfs_lookup(struct file_system *fs, struct inode *parent, const char *name);
int ramfs_readdir(struct file *dir, void *buffer, size_t buffer_size, off_t *offset);

// RAMFS utility functions
int ramfs_populate_initial_files(struct file_system *fs);
void ramfs_dump_tree(struct ramfs_node *node, int depth);
void ramfs_dump_filesystem_info(struct file_system *fs);

#ifdef __cplusplus
}
#endif

#endif /* RAMFS_H */

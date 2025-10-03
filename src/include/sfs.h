#ifndef SFS_H
#define SFS_H

// Include kernel standard types
#include "kernel.h"
#include "vfs.h"
#include "block_device.h"

#ifdef __cplusplus
extern "C" {
#endif

// SFS (Simple File System) constants
#define SFS_MAGIC           0x53465300      // "SFS\0"
#define SFS_VERSION         1
#define SFS_BLOCK_SIZE      4096
#define SFS_MAX_NAME        255
#define SFS_MAX_PATH        1024

// SFS layout constants
#define SFS_SUPERBLOCK_BLOCK    0           // Superblock at block 0
#define SFS_BITMAP_START        1           // Bitmap starts at block 1
#define SFS_INODE_START         8           // Inodes start at block 8 (7 bitmap blocks)
#define SFS_DATA_START          64          // Data blocks start at block 64

// SFS inode constants
#define SFS_DIRECT_BLOCKS       12          // Number of direct block pointers
#define SFS_INODES_PER_BLOCK    (SFS_BLOCK_SIZE / sizeof(struct sfs_inode))

// File types
#define SFS_TYPE_FILE           0x1000
#define SFS_TYPE_DIRECTORY      0x4000
#define SFS_TYPE_SYMLINK        0x8000

// File permissions
#define SFS_PERM_READ           0x0004
#define SFS_PERM_WRITE          0x0002
#define SFS_PERM_EXEC           0x0001

// SFS superblock structure
struct sfs_superblock {
    uint32_t magic;                         // Magic number (SFS_MAGIC)
    uint32_t version;                       // Filesystem version
    uint32_t block_size;                    // Block size in bytes
    uint32_t total_blocks;                  // Total number of blocks
    uint32_t inode_blocks;                  // Number of inode blocks
    uint32_t data_blocks;                   // Number of data blocks
    uint32_t free_blocks;                   // Number of free blocks
    uint32_t free_inodes;                   // Number of free inodes
    uint32_t root_inode;                    // Root directory inode number
    uint32_t first_data_block;              // First data block number
    uint32_t bitmap_blocks;                 // Number of bitmap blocks
    uint32_t created_time;                  // Filesystem creation time
    uint32_t modified_time;                 // Last modification time
    uint32_t mount_count;                   // Number of times mounted
    char label[32];                         // Volume label
    uint8_t reserved[SFS_BLOCK_SIZE - 88];  // Reserved space
};

// SFS inode structure
struct sfs_inode {
    uint32_t mode;                          // File type and permissions
    uint32_t size;                          // File size in bytes
    uint32_t blocks;                        // Number of blocks allocated
    uint32_t direct[SFS_DIRECT_BLOCKS];     // Direct block pointers
    uint32_t indirect;                      // Single indirect block pointer
    uint32_t created_time;                  // Creation timestamp
    uint32_t modified_time;                 // Last modification timestamp
    uint32_t accessed_time;                 // Last access timestamp
    uint32_t links;                         // Number of hard links
    uint32_t flags;                         // Inode flags
    uint8_t reserved[SFS_BLOCK_SIZE / 64 - 60]; // Reserved space to fill cache line
};

// SFS directory entry structure
struct sfs_dirent {
    uint32_t inode;                         // Inode number (0 = unused entry)
    uint16_t rec_len;                       // Length of this directory entry
    uint16_t name_len;                      // Length of filename
    char name[SFS_MAX_NAME];                // Filename (null-terminated)
};

// SFS filesystem private data
struct sfs_fs_data {
    struct sfs_superblock superblock;       // Cached superblock
    struct block_device *device;            // Block device
    uint8_t *block_bitmap;                  // Block allocation bitmap
    uint32_t bitmap_size;                   // Size of bitmap in bytes
};

// SFS inode private data
struct sfs_inode_data {
    struct sfs_inode disk_inode;            // On-disk inode data
    uint32_t inode_num;                     // Inode number
    int dirty;                              // Needs to be written to disk
};

// SFS file system operations
extern struct file_system_type sfs_fs_type;

// SFS initialization and management
int sfs_init(void);
struct file_system *sfs_mount(struct block_device *dev, unsigned long flags);
void sfs_unmount(struct file_system *fs);
int sfs_format(struct block_device *dev);

// SFS superblock operations
int sfs_read_superblock(struct block_device *dev, struct sfs_superblock *sb);
int sfs_write_superblock(struct block_device *dev, const struct sfs_superblock *sb);
int sfs_validate_superblock(const struct sfs_superblock *sb);

// SFS inode operations
struct inode *sfs_alloc_inode(struct file_system *fs, uint32_t mode);
void sfs_free_inode(struct file_system *fs, struct inode *inode);
struct inode *sfs_get_inode(struct file_system *fs, uint32_t inode_num);
int sfs_put_inode(struct inode *inode);
int sfs_sync_inode(struct inode *inode);
struct inode *sfs_resolve_path(struct file_system *fs, const char *path);

// SFS block operations
uint32_t sfs_alloc_block(struct file_system *fs);
void sfs_free_block(struct file_system *fs, uint32_t block_num);
int sfs_read_block(struct file_system *fs, uint32_t block_num, void *buffer);
int sfs_write_block(struct file_system *fs, uint32_t block_num, const void *buffer);

// SFS file operations
int sfs_create_file(struct file_system *fs, const char *name, uint32_t mode);
int sfs_delete_file(struct file_system *fs, const char *name);
ssize_t sfs_read_file(struct file *file, void *buf, size_t count, off_t offset);
ssize_t sfs_write_file(struct file *file, const void *buf, size_t count, off_t offset);

// SFS directory operations
int sfs_create_directory(struct file_system *fs, const char *name, uint32_t mode);
int sfs_delete_directory(struct file_system *fs, const char *name);
struct inode *sfs_lookup(struct file_system *fs, struct inode *parent, const char *name);
int sfs_add_dirent(struct file_system *fs, struct inode *dir_inode, const char *name, uint32_t inode_num);
int sfs_remove_dirent(struct file_system *fs, struct inode *dir_inode, const char *name);
int sfs_readdir(struct file *dir, void *buffer, size_t buffer_size, off_t *offset);

// SFS utility functions
int sfs_is_directory(const struct sfs_inode *inode);
int sfs_is_file(const struct sfs_inode *inode);
uint32_t sfs_get_block_for_offset(struct file_system *fs, struct inode *inode, off_t offset);
int sfs_expand_file(struct file_system *fs, struct inode *inode, size_t new_size);
int sfs_truncate_file(struct file_system *fs, struct inode *inode, size_t new_size);

// SFS bitmap operations
int sfs_test_bit(const uint8_t *bitmap, uint32_t bit);
void sfs_set_bit(uint8_t *bitmap, uint32_t bit);
void sfs_clear_bit(uint8_t *bitmap, uint32_t bit);
uint32_t sfs_find_free_bit(const uint8_t *bitmap, uint32_t size);

// SFS debugging
void sfs_dump_superblock(const struct sfs_superblock *sb);
void sfs_dump_inode(const struct sfs_inode *inode, uint32_t inode_num);
void sfs_dump_filesystem_info(struct file_system *fs);

#ifdef __cplusplus
}
#endif

#endif /* SFS_H */

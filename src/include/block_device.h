#ifndef BLOCK_DEVICE_H
#define BLOCK_DEVICE_H

// Include kernel standard types
#include "kernel.h"

#ifdef __cplusplus
extern "C" {
#endif

// Block device types
#define BLOCK_DEVICE_RAM        0
#define BLOCK_DEVICE_DISK       1
#define BLOCK_DEVICE_CDROM      2

// Block device flags
#define BLOCK_DEVICE_READABLE   0x01
#define BLOCK_DEVICE_WRITABLE   0x02
#define BLOCK_DEVICE_REMOVABLE  0x04

// Standard block sizes
#define BLOCK_SIZE_512          512
#define BLOCK_SIZE_1024         1024
#define BLOCK_SIZE_2048         2048
#define BLOCK_SIZE_4096         4096

// Error codes
#define BLOCK_SUCCESS           0
#define BLOCK_ERROR            -1
#define BLOCK_EINVAL           -2
#define BLOCK_EIO              -3
#define BLOCK_ENOSPC           -4
#define BLOCK_ENODEV           -5
#define BLOCK_EPERM            -6

// Forward declarations
struct block_device;

// Block device operations
struct block_device_operations {
    int (*read_block)(struct block_device *dev, uint32_t block_num, void *buffer);
    int (*write_block)(struct block_device *dev, uint32_t block_num, const void *buffer);
    int (*read_blocks)(struct block_device *dev, uint32_t start_block, uint32_t count, void *buffer);
    int (*write_blocks)(struct block_device *dev, uint32_t start_block, uint32_t count, const void *buffer);
    int (*sync)(struct block_device *dev);
    int (*ioctl)(struct block_device *dev, unsigned int cmd, unsigned long arg);
};

// Block device structure
struct block_device {
    char name[64];                          // Device name (e.g., "ram0", "hda")
    uint32_t device_type;                   // Device type (ram, disk, etc.)
    uint32_t block_size;                    // Block size in bytes
    uint32_t num_blocks;                    // Total number of blocks
    uint32_t flags;                         // Device flags
    
    struct block_device_operations *ops;    // Device operations
    void *private_data;                     // Device-specific data
    
    // Statistics
    uint64_t reads;                         // Number of read operations
    uint64_t writes;                        // Number of write operations
    uint64_t bytes_read;                    // Total bytes read
    uint64_t bytes_written;                 // Total bytes written
    
    // List management
    struct block_device *next;              // Next device in list
};

// Block device manager
struct block_device_manager {
    struct block_device *devices;           // List of registered devices
    uint32_t device_count;                  // Number of registered devices
};

// Block device initialization and management
int block_device_init(void);
int block_device_register(struct block_device *dev);
int block_device_unregister(const char *name);
struct block_device *block_device_find(const char *name);
void block_device_list_all(void);

// Block device I/O operations
int block_device_read(struct block_device *dev, uint32_t block, void *buffer);
int block_device_write(struct block_device *dev, uint32_t block, const void *buffer);
int block_device_read_blocks(struct block_device *dev, uint32_t start_block, uint32_t count, void *buffer);
int block_device_write_blocks(struct block_device *dev, uint32_t start_block, uint32_t count, const void *buffer);
int block_device_sync(struct block_device *dev);

// Block device utility functions
size_t block_device_get_size(struct block_device *dev);
int block_device_is_readable(struct block_device *dev);
int block_device_is_writable(struct block_device *dev);
void block_device_print_stats(struct block_device *dev);

// Block buffer management (simple caching)
struct block_buffer {
    struct block_device *device;
    uint32_t block_num;
    void *data;
    int dirty;
    int ref_count;
    struct block_buffer *next;
};

int block_buffer_init(void);
struct block_buffer *block_buffer_get(struct block_device *dev, uint32_t block);
int block_buffer_put(struct block_buffer *buf);
int block_buffer_sync(struct block_buffer *buf);
int block_buffer_sync_all(void);

// RAM disk functions
struct block_device *ramdisk_create(const char *name, size_t size);
void ramdisk_destroy(struct block_device *dev);
int ramdisk_format_test(struct block_device *dev);

#ifdef __cplusplus
}
#endif

#endif /* BLOCK_DEVICE_H */
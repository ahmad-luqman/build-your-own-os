/*
 * MiniOS RAM Disk Implementation
 * In-memory block device for testing and temporary storage
 */

#include "block_device.h"
#include "memory.h"
#include "kernel.h"

// RAM disk private data
struct ramdisk_data {
    void *memory;                           // Allocated memory
    size_t size;                            // Total size
    uint32_t block_size;                    // Block size
    uint32_t num_blocks;                    // Number of blocks
};

// RAM disk operations
static int ramdisk_read_block(struct block_device *dev, uint32_t block_num, void *buffer);
static int ramdisk_write_block(struct block_device *dev, uint32_t block_num, const void *buffer);
static int ramdisk_read_blocks(struct block_device *dev, uint32_t start_block, uint32_t count, void *buffer);
static int ramdisk_write_blocks(struct block_device *dev, uint32_t start_block, uint32_t count, const void *buffer);
static int ramdisk_sync(struct block_device *dev);

// RAM disk operations structure
static struct block_device_operations ramdisk_ops = {
    .read_block = ramdisk_read_block,
    .write_block = ramdisk_write_block,
    .read_blocks = ramdisk_read_blocks,
    .write_blocks = ramdisk_write_blocks,
    .sync = ramdisk_sync,
    .ioctl = NULL
};

struct block_device *ramdisk_create(const char *name, size_t size)
{
    if (!name || size == 0) {
        return NULL;
    }
    
    early_print("Creating RAM disk: ");
    early_print(name);
    early_print(" (");
    
    // Print size
    if (size >= 1024 * 1024) {
        char size_str[32];
        int mb = size / (1024 * 1024);
        int pos = 0;
        if (mb == 0) {
            size_str[pos++] = '0';
        } else {
            char temp[32];
            int temp_pos = 0;
            while (mb > 0) {
                temp[temp_pos++] = '0' + (mb % 10);
                mb /= 10;
            }
            for (int i = temp_pos - 1; i >= 0; i--) {
                size_str[pos++] = temp[i];
            }
        }
        size_str[pos] = '\0';
        early_print(size_str);
        early_print("MB)\n");
    } else if (size >= 1024) {
        char size_str[32];
        int kb = size / 1024;
        int pos = 0;
        if (kb == 0) {
            size_str[pos++] = '0';
        } else {
            char temp[32];
            int temp_pos = 0;
            while (kb > 0) {
                temp[temp_pos++] = '0' + (kb % 10);
                kb /= 10;
            }
            for (int i = temp_pos - 1; i >= 0; i--) {
                size_str[pos++] = temp[i];
            }
        }
        size_str[pos] = '\0';
        early_print(size_str);
        early_print("KB)\n");
    } else {
        early_print("bytes)\n");
    }
    
    // Allocate device structure
    struct block_device *dev = memory_alloc(sizeof(struct block_device), MEMORY_ALIGN_4K);
    if (!dev) {
        early_print("Failed to allocate RAM disk device structure\n");
        return NULL;
    }

    // Allocate private data
    struct ramdisk_data *data = memory_alloc(sizeof(struct ramdisk_data), MEMORY_ALIGN_4K);
    if (!data) {
        early_print("Failed to allocate RAM disk private data\n");
        memory_free(dev);
        return NULL;
    }

    // Allocate RAM disk memory - use smaller size for testing
    size_t alloc_size = size > 64*1024 ? 64*1024 : size;  // Cap at 64KB for testing
    void *memory = memory_alloc(alloc_size, MEMORY_ALIGN_4K);
    if (!memory) {
        early_print("Failed to allocate RAM disk memory\n");
        memory_free(data);
        memory_free(dev);
        return NULL;
    }
    
    // Initialize RAM disk data (zero-fill)
    memset(memory, 0, alloc_size);

    // Setup private data
    data->memory = memory;
    data->size = alloc_size;  // Use actual allocated size
    data->block_size = BLOCK_SIZE_4096;  // Use 4KB blocks
    data->num_blocks = alloc_size / data->block_size;
    
    // Setup device structure
    strncpy(dev->name, name, sizeof(dev->name) - 1);
    dev->name[sizeof(dev->name) - 1] = '\0';
    dev->device_type = BLOCK_DEVICE_RAM;
    dev->block_size = data->block_size;
    dev->num_blocks = data->num_blocks;
    dev->flags = BLOCK_DEVICE_READABLE | BLOCK_DEVICE_WRITABLE;
    dev->ops = &ramdisk_ops;
    dev->private_data = data;
    dev->next = NULL;
    
    // Register device
    if (block_device_register(dev) != BLOCK_SUCCESS) {
        early_print("Failed to register RAM disk\n");
        memory_free(memory);
        memory_free(data);
        memory_free(dev);
        return NULL;
    }
    
    return dev;
}

void ramdisk_destroy(struct block_device *dev)
{
    if (!dev || dev->device_type != BLOCK_DEVICE_RAM) {
        return;
    }
    
    struct ramdisk_data *data = (struct ramdisk_data *)dev->private_data;
    if (data) {
        if (data->memory) {
            memory_free(data->memory);
        }
        memory_free(data);
    }

    // Note: Should also unregister from device manager
    memory_free(dev);
}

static int ramdisk_read_block(struct block_device *dev, uint32_t block_num, void *buffer)
{
    if (!dev || !buffer) {
        return BLOCK_EINVAL;
    }
    
    struct ramdisk_data *data = (struct ramdisk_data *)dev->private_data;
    if (!data || !data->memory) {
        return BLOCK_EIO;
    }
    
    if (block_num >= data->num_blocks) {
        return BLOCK_EINVAL;
    }
    
    // Calculate offset and copy data
    size_t offset = block_num * data->block_size;
    memcpy(buffer, (char *)data->memory + offset, data->block_size);
    
    return BLOCK_SUCCESS;
}

static int ramdisk_write_block(struct block_device *dev, uint32_t block_num, const void *buffer)
{
    if (!dev || !buffer) {
        return BLOCK_EINVAL;
    }
    
    struct ramdisk_data *data = (struct ramdisk_data *)dev->private_data;
    if (!data || !data->memory) {
        return BLOCK_EIO;
    }
    
    if (block_num >= data->num_blocks) {
        return BLOCK_EINVAL;
    }
    
    // Calculate offset and copy data
    size_t offset = block_num * data->block_size;
    memcpy((char *)data->memory + offset, buffer, data->block_size);
    
    return BLOCK_SUCCESS;
}

static int ramdisk_read_blocks(struct block_device *dev, uint32_t start_block, uint32_t count, void *buffer)
{
    if (!dev || !buffer || count == 0) {
        return BLOCK_EINVAL;
    }
    
    struct ramdisk_data *data = (struct ramdisk_data *)dev->private_data;
    if (!data || !data->memory) {
        return BLOCK_EIO;
    }
    
    if (start_block + count > data->num_blocks) {
        return BLOCK_EINVAL;
    }
    
    // Calculate offset and copy data
    size_t offset = start_block * data->block_size;
    size_t total_size = count * data->block_size;
    memcpy(buffer, (char *)data->memory + offset, total_size);
    
    return BLOCK_SUCCESS;
}

static int ramdisk_write_blocks(struct block_device *dev, uint32_t start_block, uint32_t count, const void *buffer)
{
    if (!dev || !buffer || count == 0) {
        return BLOCK_EINVAL;
    }
    
    struct ramdisk_data *data = (struct ramdisk_data *)dev->private_data;
    if (!data || !data->memory) {
        return BLOCK_EIO;
    }
    
    if (start_block + count > data->num_blocks) {
        return BLOCK_EINVAL;
    }
    
    // Calculate offset and copy data
    size_t offset = start_block * data->block_size;
    size_t total_size = count * data->block_size;
    memcpy((char *)data->memory + offset, buffer, total_size);
    
    return BLOCK_SUCCESS;
}

static int ramdisk_sync(struct block_device *dev)
{
    // RAM disk is always synchronized (no persistent storage)
    (void)dev;  // Suppress unused parameter warning
    return BLOCK_SUCCESS;
}

// Utility function to format RAM disk with test pattern
int ramdisk_format_test(struct block_device *dev)
{
    if (!dev || dev->device_type != BLOCK_DEVICE_RAM) {
        return BLOCK_EINVAL;
    }
    
    struct ramdisk_data *data = (struct ramdisk_data *)dev->private_data;
    if (!data || !data->memory) {
        return BLOCK_EIO;
    }
    
    early_print("Formatting RAM disk with test pattern...\n");
    
    // Fill with test pattern
    char *mem = (char *)data->memory;
    for (size_t i = 0; i < data->size; i++) {
        mem[i] = (char)(i & 0xFF);  // Simple pattern
    }
    
    return BLOCK_SUCCESS;
}
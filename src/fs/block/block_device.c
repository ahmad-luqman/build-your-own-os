/*
 * MiniOS Block Device Layer
 * Provides abstraction for storage devices
 */

#include "block_device.h"
#include "memory.h"
#include "kernel.h"

// Global block device manager
static struct block_device_manager device_manager = {
    .devices = NULL,
    .device_count = 0
};

// Block buffer cache (simple implementation)
#define MAX_BUFFERS 32
static struct block_buffer buffer_cache[MAX_BUFFERS];
static int buffer_cache_initialized = 0;

int block_device_init(void)
{
    early_print("Initializing block device layer...\n");
    
    // Initialize device manager
    device_manager.devices = NULL;
    device_manager.device_count = 0;
    
    // Initialize buffer cache
    if (block_buffer_init() != 0) {
        early_print("Failed to initialize block buffer cache\n");
        return BLOCK_ERROR;
    }
    
    early_print("Block device layer initialized\n");
    return BLOCK_SUCCESS;
}

int block_device_register(struct block_device *dev)
{
    if (!dev || !dev->name[0] || !dev->ops) {
        return BLOCK_EINVAL;
    }
    
    // Check if device already exists
    if (block_device_find(dev->name)) {
        early_print("Block device already exists: ");
        early_print(dev->name);
        early_print("\n");
        return BLOCK_ERROR;
    }
    
    // Add to device list
    dev->next = device_manager.devices;
    device_manager.devices = dev;
    device_manager.device_count++;
    
    // Compiler barrier to ensure list updates complete before statistics init
    barrier();
    
    // Initialize statistics
    // Using volatile fields prevents compiler from optimizing away writes
    dev->reads = 0;
    dev->writes = 0;
    dev->bytes_read = 0;
    dev->bytes_written = 0;
    
    // Compiler barrier to ensure all initialization completes
    barrier();
    
    early_print("Registered block device: ");
    early_print(dev->name);
    early_print(" (");
    
    // Print size information
    size_t total_size = (size_t)dev->num_blocks * dev->block_size;
    if (total_size >= 1024 * 1024) {
        char size_str[32];
        int mb = total_size / (1024 * 1024);
        // Simple integer to string conversion
        int pos = 0;
        int temp = mb;
        if (temp == 0) {
            size_str[pos++] = '0';
        } else {
            char temp_str[32];
            int temp_pos = 0;
            while (temp > 0) {
                temp_str[temp_pos++] = '0' + (temp % 10);
                temp /= 10;
            }
            // Reverse the string
            for (int i = temp_pos - 1; i >= 0; i--) {
                size_str[pos++] = temp_str[i];
            }
        }
        size_str[pos] = '\0';
        early_print(size_str);
        early_print("MB)\n");
    } else if (total_size >= 1024) {
        char size_str[32];
        int kb = total_size / 1024;
        // Simple integer to string conversion
        int pos = 0;
        int temp = kb;
        if (temp == 0) {
            size_str[pos++] = '0';
        } else {
            char temp_str[32];
            int temp_pos = 0;
            while (temp > 0) {
                temp_str[temp_pos++] = '0' + (temp % 10);
                temp /= 10;
            }
            // Reverse the string
            for (int i = temp_pos - 1; i >= 0; i--) {
                size_str[pos++] = temp_str[i];
            }
        }
        size_str[pos] = '\0';
        early_print(size_str);
        early_print("KB)\n");
    } else {
        early_print("small)\n");
    }
    
    return BLOCK_SUCCESS;
}

struct block_device *block_device_find(const char *name)
{
    if (!name) {
        return NULL;
    }
    
    struct block_device *dev = device_manager.devices;
    while (dev) {
        if (strcmp(dev->name, name) == 0) {
            return dev;
        }
        dev = dev->next;
    }
    
    return NULL;
}

int block_device_read(struct block_device *dev, uint32_t block, void *buffer)
{
    if (!dev || !buffer || !dev->ops || !dev->ops->read_block) {
        return BLOCK_EINVAL;
    }
    
    if (block >= dev->num_blocks) {
        return BLOCK_EINVAL;
    }
    
    if (!(dev->flags & BLOCK_DEVICE_READABLE)) {
        return BLOCK_EPERM;
    }
    
    int result = dev->ops->read_block(dev, block, buffer);
    if (result == BLOCK_SUCCESS) {
        dev->reads++;
        dev->bytes_read += dev->block_size;
    }
    
    return result;
}

int block_device_write(struct block_device *dev, uint32_t block, const void *buffer)
{
    if (!dev || !buffer || !dev->ops || !dev->ops->write_block) {
        return BLOCK_EINVAL;
    }
    
    if (block >= dev->num_blocks) {
        return BLOCK_EINVAL;
    }
    
    if (!(dev->flags & BLOCK_DEVICE_WRITABLE)) {
        return BLOCK_EPERM;
    }
    
    int result = dev->ops->write_block(dev, block, buffer);
    if (result == BLOCK_SUCCESS) {
        dev->writes++;
        dev->bytes_written += dev->block_size;
    }
    
    return result;
}

int block_device_read_blocks(struct block_device *dev, uint32_t start_block, uint32_t count, void *buffer)
{
    if (!dev || !buffer || count == 0) {
        return BLOCK_EINVAL;
    }
    
    if (start_block + count > dev->num_blocks) {
        return BLOCK_EINVAL;
    }
    
    // Use multi-block operation if available
    if (dev->ops->read_blocks) {
        int result = dev->ops->read_blocks(dev, start_block, count, buffer);
        if (result == BLOCK_SUCCESS) {
            dev->reads += count;
            dev->bytes_read += count * dev->block_size;
        }
        return result;
    }
    
    // Fall back to single-block operations
    char *buf = (char *)buffer;
    for (uint32_t i = 0; i < count; i++) {
        int result = block_device_read(dev, start_block + i, buf + (i * dev->block_size));
        if (result != BLOCK_SUCCESS) {
            return result;
        }
    }
    
    return BLOCK_SUCCESS;
}

int block_device_write_blocks(struct block_device *dev, uint32_t start_block, uint32_t count, const void *buffer)
{
    if (!dev || !buffer || count == 0) {
        return BLOCK_EINVAL;
    }
    
    if (start_block + count > dev->num_blocks) {
        return BLOCK_EINVAL;
    }
    
    // Use multi-block operation if available
    if (dev->ops->write_blocks) {
        int result = dev->ops->write_blocks(dev, start_block, count, buffer);
        if (result == BLOCK_SUCCESS) {
            dev->writes += count;
            dev->bytes_written += count * dev->block_size;
        }
        return result;
    }
    
    // Fall back to single-block operations
    const char *buf = (const char *)buffer;
    for (uint32_t i = 0; i < count; i++) {
        int result = block_device_write(dev, start_block + i, buf + (i * dev->block_size));
        if (result != BLOCK_SUCCESS) {
            return result;
        }
    }
    
    return BLOCK_SUCCESS;
}

int block_device_sync(struct block_device *dev)
{
    if (!dev || !dev->ops) {
        return BLOCK_EINVAL;
    }
    
    if (dev->ops->sync) {
        return dev->ops->sync(dev);
    }
    
    return BLOCK_SUCCESS;
}

size_t block_device_get_size(struct block_device *dev)
{
    if (!dev) {
        return 0;
    }
    
    return (size_t)dev->num_blocks * dev->block_size;
}

int block_device_is_readable(struct block_device *dev)
{
    return dev && (dev->flags & BLOCK_DEVICE_READABLE);
}

int block_device_is_writable(struct block_device *dev)
{
    return dev && (dev->flags & BLOCK_DEVICE_WRITABLE);
}

void block_device_list_all(void)
{
    early_print("Block devices:\n");
    
    struct block_device *dev = device_manager.devices;
    if (!dev) {
        early_print("  No devices registered\n");
        return;
    }
    
    while (dev) {
        early_print("  ");
        early_print(dev->name);
        early_print(": ");
        
        // Print device info
        size_t total_size = block_device_get_size(dev);
        if (total_size >= 1024 * 1024) {
            char size_str[32];
            int mb = total_size / (1024 * 1024);
            // Convert to string (simplified)
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
            early_print("MB");
        } else {
            char size_str[32];
            int kb = total_size / 1024;
            // Convert to string (simplified)
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
            early_print("KB");
        }
        
        if (dev->flags & BLOCK_DEVICE_READABLE) early_print(" R");
        if (dev->flags & BLOCK_DEVICE_WRITABLE) early_print("W");
        early_print("\n");
        
        dev = dev->next;
    }
}

// Simple block buffer implementation
int block_buffer_init(void)
{
    if (buffer_cache_initialized) {
        return BLOCK_SUCCESS;
    }
    
    // Initialize buffer cache
    for (int i = 0; i < MAX_BUFFERS; i++) {
        buffer_cache[i].device = NULL;
        buffer_cache[i].block_num = 0;
        buffer_cache[i].data = NULL;
        buffer_cache[i].dirty = 0;
        buffer_cache[i].ref_count = 0;
        buffer_cache[i].next = NULL;
    }
    
    buffer_cache_initialized = 1;
    return BLOCK_SUCCESS;
}

struct block_buffer *block_buffer_get(struct block_device *dev, uint32_t block)
{
    if (!dev || !buffer_cache_initialized) {
        return NULL;
    }
    
    // Simple implementation: find unused buffer
    for (int i = 0; i < MAX_BUFFERS; i++) {
        if (buffer_cache[i].ref_count == 0) {
            buffer_cache[i].device = dev;
            buffer_cache[i].block_num = block;
            buffer_cache[i].dirty = 0;
            buffer_cache[i].ref_count = 1;
            
            // Allocate data buffer if needed
            if (!buffer_cache[i].data) {
                buffer_cache[i].data = kmalloc(dev->block_size);
                if (!buffer_cache[i].data) {
                    return NULL;
                }
            }
            
            // Read block data
            if (block_device_read(dev, block, buffer_cache[i].data) != BLOCK_SUCCESS) {
                buffer_cache[i].ref_count = 0;
                return NULL;
            }
            
            return &buffer_cache[i];
        }
    }
    
    return NULL; // No free buffers
}

int block_buffer_put(struct block_buffer *buf)
{
    if (!buf) {
        return BLOCK_EINVAL;
    }
    
    if (buf->ref_count > 0) {
        buf->ref_count--;
        
        // Sync if dirty and no more references
        if (buf->ref_count == 0 && buf->dirty) {
            return block_buffer_sync(buf);
        }
    }
    
    return BLOCK_SUCCESS;
}

int block_buffer_sync(struct block_buffer *buf)
{
    if (!buf || !buf->device || !buf->data) {
        return BLOCK_EINVAL;
    }
    
    if (buf->dirty) {
        int result = block_device_write(buf->device, buf->block_num, buf->data);
        if (result == BLOCK_SUCCESS) {
            buf->dirty = 0;
        }
        return result;
    }
    
    return BLOCK_SUCCESS;
}

int block_buffer_sync_all(void)
{
    if (!buffer_cache_initialized) {
        return BLOCK_SUCCESS;
    }
    
    for (int i = 0; i < MAX_BUFFERS; i++) {
        if (buffer_cache[i].ref_count > 0 && buffer_cache[i].dirty) {
            int result = block_buffer_sync(&buffer_cache[i]);
            if (result != BLOCK_SUCCESS) {
                return result;
            }
        }
    }
    
    return BLOCK_SUCCESS;
}
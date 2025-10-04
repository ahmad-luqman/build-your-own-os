/*
 * MiniOS Simple File System (SFS) Core
 * Basic file system implementation for educational purposes
 */

#include "sfs.h"
#include "memory.h"
#include "kernel.h"
#include <string.h>

// Disable optimizations for this entire file to prevent SIMD generation
#pragma GCC push_options
#pragma GCC optimize ("-O0")
#pragma GCC target ("general-regs-only")

// Helper functions to prevent GCC SIMD vectorization bugs
static inline void sfs_sync_inode_data_safe(struct sfs_inode *disk_inode, const struct inode *inode)
    __attribute__((always_inline, nonnull));
static inline void sfs_sync_inode_data_safe(struct sfs_inode *disk_inode, const struct inode *inode) {
    __asm__ volatile("dmb ish" ::: "memory");
    disk_inode->size = inode->size;
    __asm__ volatile("dmb ish" ::: "memory");
    disk_inode->blocks = inode->blocks;
    __asm__ volatile("dmb ish" ::: "memory");
    disk_inode->modified_time = inode->modified_time;
    __asm__ volatile("dmb ish" ::: "memory");
    disk_inode->accessed_time = inode->accessed_time;
    __asm__ volatile("dmb ish" ::: "memory");
}

// SFS file operations
static int sfs_file_open(struct file *file, int flags, int mode);
static int sfs_file_close(struct file *file);
static ssize_t sfs_file_read(struct file *file, void *buf, size_t count, off_t offset);
static ssize_t sfs_file_write(struct file *file, const void *buf, size_t count, off_t offset);
static off_t sfs_file_seek(struct file *file, off_t offset, int whence);
static int sfs_file_sync(struct file *file);

// SFS directory operations
static int sfs_dir_readdir(struct file *dir, void *buffer, size_t buffer_size, off_t *offset);
static int sfs_dir_mkdir(struct file_system *fs, const char *path, int mode);
static int sfs_dir_rmdir(struct file_system *fs, const char *path);
static struct inode *sfs_dir_lookup(struct file_system *fs, struct inode *parent, const char *name);

// SFS operations structures
static struct file_operations sfs_file_ops = {
    .open = sfs_file_open,
    .close = sfs_file_close,
    .read = sfs_file_read,
    .write = sfs_file_write,
    .seek = sfs_file_seek,
    .sync = sfs_file_sync,
    .ioctl = NULL
};

static struct directory_operations sfs_dir_ops = {
    .readdir = sfs_dir_readdir,
    .mkdir = sfs_dir_mkdir,
    .rmdir = sfs_dir_rmdir,
    .lookup = sfs_dir_lookup
};

// Internal helpers
static int sfs_sync_superblock(struct file_system *fs);
static int sfs_flush_bitmap_bit(struct file_system *fs, uint32_t bit_index);

// SFS file system type
struct file_system_type sfs_fs_type = {
    .name = "sfs",
    .mount = sfs_mount,
    .unmount = sfs_unmount,
    .format = sfs_format,
    .file_ops = &sfs_file_ops,
    .dir_ops = &sfs_dir_ops
};

int sfs_init(void)
{
    early_print("Initializing Simple File System (SFS)...\n");
    
    // Register SFS with VFS
    if (vfs_register_filesystem(&sfs_fs_type) != VFS_SUCCESS) {
        early_print("Failed to register SFS filesystem\n");
        return VFS_ERROR;
    }
    
    early_print("SFS initialized\n");
    return VFS_SUCCESS;
}

int sfs_format(struct block_device *dev)
{
    if (!dev) {
        return VFS_EINVAL;
    }
    
    early_print("Formatting device with SFS...\n");
    
    // Calculate filesystem parameters
    uint32_t total_blocks = dev->num_blocks;
    uint32_t bitmap_blocks = (total_blocks + (SFS_BLOCK_SIZE * 8) - 1) / (SFS_BLOCK_SIZE * 8);
    uint32_t inode_blocks = total_blocks / 8;  // 1 inode per 8 data blocks
    uint32_t data_blocks = total_blocks - 1 - bitmap_blocks - inode_blocks;  // -1 for superblock
    
    // Create superblock
    struct sfs_superblock sb;
    memset(&sb, 0, sizeof(sb));
    
    sb.magic = SFS_MAGIC;
    sb.version = SFS_VERSION;
    sb.block_size = SFS_BLOCK_SIZE;
    sb.total_blocks = total_blocks;
    sb.inode_blocks = inode_blocks;
    sb.data_blocks = data_blocks;
    sb.free_blocks = data_blocks - 1;  // -1 for root directory
    sb.free_inodes = (inode_blocks * SFS_INODES_PER_BLOCK) - 1;  // -1 for root inode
    sb.root_inode = 1;  // Root inode number
    sb.first_data_block = 1 + bitmap_blocks + inode_blocks;
    sb.bitmap_blocks = bitmap_blocks;
    sb.created_time = 0;  // Would use real timestamp
    sb.modified_time = 0;
    sb.mount_count = 0;
    strcpy(sb.label, "MiniOS SFS");
    
    // Write superblock
    if (sfs_write_superblock(dev, &sb) != VFS_SUCCESS) {
        early_print("Failed to write superblock\n");
        return VFS_ERROR;
    }
    early_print("SFS format: superblock written\n");
    
    // Initialize block bitmap (all blocks free initially)
    void *bitmap_block = kmalloc(SFS_BLOCK_SIZE);
    if (!bitmap_block) {
        early_print("Failed to allocate bitmap block\n");
        return VFS_ENOMEM;
    }
    early_print("SFS format: bitmap block allocated\n");
    
    memset(bitmap_block, 0, SFS_BLOCK_SIZE);
    
    // Mark superblock, bitmap blocks, and inode blocks as used
    uint8_t *bitmap = (uint8_t *)bitmap_block;
    sfs_set_bit(bitmap, 0);  // Superblock
    
    for (uint32_t i = 0; i < bitmap_blocks + inode_blocks; i++) {
        sfs_set_bit(bitmap, i + 1);
    }
    
    // Write bitmap blocks
    for (uint32_t i = 0; i < bitmap_blocks; i++) {
        if (block_device_write(dev, SFS_BITMAP_START + i, bitmap_block) != BLOCK_SUCCESS) {
            early_print("Failed to write bitmap block\n");
            kfree(bitmap_block);
            return VFS_ERROR;
        }
        early_print("SFS format: bitmap block written\n");
        
        // Clear bitmap for subsequent blocks
        if (i == 0) {
            memset(bitmap_block, 0, SFS_BLOCK_SIZE);
        }
    }
    
    kfree(bitmap_block);
    
    // Initialize inode blocks
    void *inode_block = kmalloc(SFS_BLOCK_SIZE);
    if (!inode_block) {
        early_print("Failed to allocate inode block\n");
        return VFS_ENOMEM;
    }
    early_print("SFS format: inode block allocated\n");
    
    // Ensure allocation completed with memory barrier
    barrier();
    
    // Validate pointer before use
    if ((uint64_t)inode_block & 7) {
        early_print("ERROR: inode_block not 8-byte aligned\n");
        kfree(inode_block);
        return VFS_ERROR;
    }
    
    early_print("SFS format: about to memset inode block\n");
    memset(inode_block, 0, SFS_BLOCK_SIZE);
    early_print("SFS format: memset complete\n");
    
    // Create root directory inode
    early_print("SFS format: setting up root inode\n");
    struct sfs_inode *root_inode = (struct sfs_inode *)inode_block;
    
    // Use volatile pointer to prevent compiler optimization issues
    volatile struct sfs_inode *vroot = root_inode;
    
    early_print("SFS format: setting mode\n");
    vroot->mode = SFS_TYPE_DIRECTORY | SFS_PERM_READ | SFS_PERM_WRITE | SFS_PERM_EXEC;
    barrier();
    
    early_print("SFS format: setting sizes\n");
    vroot->size = 0;
    vroot->blocks = 0;
    barrier();
    
    early_print("SFS format: setting timestamps\n");
    vroot->created_time = 0;
    vroot->modified_time = 0;
    vroot->accessed_time = 0;
    barrier();
    
    early_print("SFS format: setting links and flags\n");
    vroot->links = 1;
    vroot->flags = 0;
    barrier();
    
    early_print("SFS format: initializing direct blocks\n");
    // Initialize direct blocks to 0 (no blocks allocated yet)
    for (int i = 0; i < SFS_DIRECT_BLOCKS; i++) {
        vroot->direct[i] = 0;
    }
    vroot->indirect = 0;
    barrier();
    
    early_print("SFS format: root inode setup complete\n");
    
    // Write first inode block (contains root inode)
    if (block_device_write(dev, sb.first_data_block - inode_blocks, inode_block) != BLOCK_SUCCESS) {
        early_print("Failed to write root inode\n");
        kfree(inode_block);
        return VFS_ERROR;
    }
    early_print("SFS format: root inode block written\n");
    
    // Write remaining inode blocks (empty)
    memset(inode_block, 0, SFS_BLOCK_SIZE);
    for (uint32_t i = 1; i < inode_blocks; i++) {
        if (block_device_write(dev, sb.first_data_block - inode_blocks + i, inode_block) != BLOCK_SUCCESS) {
            early_print("Failed to write inode block\n");
            kfree(inode_block);
            return VFS_ERROR;
        }
    }
    
    kfree(inode_block);
    
    early_print("SFS format complete\n");
    early_print("  Total blocks: ");
    
    // Simple number to string conversion for debugging
    char num_str[16];
    int val = total_blocks;
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
        for (int i = temp_pos - 1; i >= 0; i--) {
            num_str[pos++] = temp[i];
        }
    }
    num_str[pos] = '\0';
    early_print(num_str);
    
    early_print(", Data blocks: ");
    val = data_blocks;
    pos = 0;
    if (val == 0) {
        num_str[pos++] = '0';
    } else {
        char temp[16];
        int temp_pos = 0;
        while (val > 0) {
            temp[temp_pos++] = '0' + (val % 10);
            val /= 10;
        }
        for (int i = temp_pos - 1; i >= 0; i--) {
            num_str[pos++] = temp[i];
        }
    }
    num_str[pos] = '\0';
    early_print(num_str);
    early_print("\n");
    
    return VFS_SUCCESS;
}

struct file_system *sfs_mount(struct block_device *dev, unsigned long flags)
{
    if (!dev) {
        return NULL;
    }
    
    early_print("Mounting SFS filesystem...\n");
    
    // Allocate file system structure
    struct file_system *fs = kmalloc(sizeof(struct file_system));
    if (!fs) {
        early_print("Failed to allocate filesystem structure\n");
        return NULL;
    }
    
    // Allocate private data
    struct sfs_fs_data *data = kmalloc(sizeof(struct sfs_fs_data));
    if (!data) {
        early_print("Failed to allocate SFS private data\n");
        kfree(fs);
        return NULL;
    }
    
    // Read and validate superblock
    if (sfs_read_superblock(dev, &data->superblock) != VFS_SUCCESS) {
        early_print("Failed to read superblock\n");
        kfree(data);
        kfree(fs);
        return NULL;
    }
    
    if (sfs_validate_superblock(&data->superblock) != VFS_SUCCESS) {
        early_print("Invalid superblock\n");
        kfree(data);
        kfree(fs);
        return NULL;
    }
    
    // Allocate and read block bitmap
    data->bitmap_size = data->superblock.bitmap_blocks * SFS_BLOCK_SIZE;
    data->block_bitmap = kmalloc(data->bitmap_size);
    if (!data->block_bitmap) {
        early_print("Failed to allocate block bitmap\n");
        kfree(data);
        kfree(fs);
        return NULL;
    }
    
    // Read bitmap blocks
    for (uint32_t i = 0; i < data->superblock.bitmap_blocks; i++) {
        if (block_device_read(dev, SFS_BITMAP_START + i, 
                              data->block_bitmap + (i * SFS_BLOCK_SIZE)) != BLOCK_SUCCESS) {
            early_print("Failed to read bitmap block\n");
            kfree(data->block_bitmap);
            kfree(data);
            kfree(fs);
            return NULL;
        }
    }
    
    // Initialize filesystem structure
    data->device = dev;
    fs->type = &sfs_fs_type;
    fs->device = dev;
    fs->private_data = data;
    fs->mount_point = NULL;  // Will be set by VFS
    fs->flags = flags;
    
    // Update mount count in superblock
    data->superblock.mount_count++;
    sfs_write_superblock(dev, &data->superblock);
    
    early_print("SFS mount successful\n");
    return fs;
}

void sfs_unmount(struct file_system *fs)
{
    if (!fs || !fs->private_data) {
        return;
    }
    
    early_print("Unmounting SFS filesystem...\n");
    
    struct sfs_fs_data *data = (struct sfs_fs_data *)fs->private_data;
    
    // Sync filesystem
    block_device_sync(data->device);
    
    // Free bitmap
    if (data->block_bitmap) {
        kfree(data->block_bitmap);
    }
    
    // Free private data
    kfree(data);
    
    // Free filesystem structure
    kfree(fs);
    
    early_print("SFS unmount complete\n");
}

int sfs_read_superblock(struct block_device *dev, struct sfs_superblock *sb)
{
    if (!dev || !sb) {
        return VFS_EINVAL;
    }
    
    return (block_device_read(dev, SFS_SUPERBLOCK_BLOCK, sb) == BLOCK_SUCCESS) ? 
           VFS_SUCCESS : VFS_EIO;
}

int sfs_write_superblock(struct block_device *dev, const struct sfs_superblock *sb)
{
    if (!dev || !sb) {
        return VFS_EINVAL;
    }
    
    return (block_device_write(dev, SFS_SUPERBLOCK_BLOCK, sb) == BLOCK_SUCCESS) ? 
           VFS_SUCCESS : VFS_EIO;
}

int sfs_validate_superblock(const struct sfs_superblock *sb)
{
    if (!sb) {
        return VFS_EINVAL;
    }
    
    if (sb->magic != SFS_MAGIC) {
        early_print("Invalid SFS magic number\n");
        return VFS_ERROR;
    }
    
    if (sb->version != SFS_VERSION) {
        early_print("Unsupported SFS version\n");
        return VFS_ERROR;
    }
    
    if (sb->block_size != SFS_BLOCK_SIZE) {
        early_print("Unsupported block size\n");
        return VFS_ERROR;
    }
    
    return VFS_SUCCESS;
}

// SFS bitmap operations
int sfs_test_bit(const uint8_t *bitmap, uint32_t bit)
{
    uint32_t byte_index = bit / 8;
    uint32_t bit_index = bit % 8;
    return (bitmap[byte_index] >> bit_index) & 1;
}

void sfs_set_bit(uint8_t *bitmap, uint32_t bit)
{
    uint32_t byte_index = bit / 8;
    uint32_t bit_index = bit % 8;
    bitmap[byte_index] |= (1 << bit_index);
}

void sfs_clear_bit(uint8_t *bitmap, uint32_t bit)
{
    uint32_t byte_index = bit / 8;
    uint32_t bit_index = bit % 8;
    bitmap[byte_index] &= ~(1 << bit_index);
}

uint32_t sfs_find_free_bit(const uint8_t *bitmap, uint32_t size)
{
    for (uint32_t i = 0; i < size; i++) {
        if (!sfs_test_bit(bitmap, i)) {
            return i;
        }
    }
    return (uint32_t)-1;  // No free bit found
}

// SFS inode and block allocation functions
uint32_t sfs_alloc_block(struct file_system *fs)
{
    if (!fs || !fs->private_data) {
        return 0;
    }
    
    struct sfs_fs_data *data = (struct sfs_fs_data *)fs->private_data;
    
    // Find free block in bitmap
    uint32_t block_num = sfs_find_free_bit(data->block_bitmap, data->superblock.total_blocks);
    if (block_num == (uint32_t)-1 || block_num >= data->superblock.total_blocks) {
        return 0;  // No free blocks
    }
    
    // Mark block as used
    sfs_set_bit(data->block_bitmap, block_num);
    data->superblock.free_blocks--;

    if (sfs_flush_bitmap_bit(fs, block_num) != VFS_SUCCESS) {
        sfs_clear_bit(data->block_bitmap, block_num);
        data->superblock.free_blocks++;
        return 0;
    }

    sfs_sync_superblock(fs);

    void *zero = kmalloc(SFS_BLOCK_SIZE);
    if (zero) {
        memset(zero, 0, SFS_BLOCK_SIZE);
        sfs_write_block(fs, block_num, zero);
        kfree(zero);
    }
    
    return block_num;
}

void sfs_free_block(struct file_system *fs, uint32_t block_num)
{
    if (!fs || !fs->private_data || block_num == 0) {
        return;
    }
    
    struct sfs_fs_data *data = (struct sfs_fs_data *)fs->private_data;
    
    if (block_num >= data->superblock.total_blocks) {
        return;
    }
    
    // Clear block in bitmap
    sfs_clear_bit(data->block_bitmap, block_num);
    data->superblock.free_blocks++;

    sfs_flush_bitmap_bit(fs, block_num);
    sfs_sync_superblock(fs);
}

int sfs_read_block(struct file_system *fs, uint32_t block_num, void *buffer)
{
    if (!fs || !fs->private_data || !buffer) {
        return VFS_EINVAL;
    }
    
    struct sfs_fs_data *data = (struct sfs_fs_data *)fs->private_data;
    
    if (block_num >= data->superblock.total_blocks) {
        return VFS_EINVAL;
    }
    
    return (block_device_read(data->device, block_num, buffer) == BLOCK_SUCCESS) ?
           VFS_SUCCESS : VFS_EIO;
}

int sfs_write_block(struct file_system *fs, uint32_t block_num, const void *buffer)
{
    if (!fs || !fs->private_data || !buffer) {
        return VFS_EINVAL;
    }
    
    struct sfs_fs_data *data = (struct sfs_fs_data *)fs->private_data;
    
    if (block_num >= data->superblock.total_blocks) {
        return VFS_EINVAL;
    }
    
    return (block_device_write(data->device, block_num, buffer) == BLOCK_SUCCESS) ?
           VFS_SUCCESS : VFS_EIO;
}

static int sfs_sync_superblock(struct file_system *fs)
{
    if (!fs || !fs->private_data) {
        return VFS_EINVAL;
    }

    struct sfs_fs_data *data = (struct sfs_fs_data *)fs->private_data;
    return sfs_write_superblock(data->device, &data->superblock);
}

static int sfs_flush_bitmap_bit(struct file_system *fs, uint32_t bit_index)
{
    if (!fs || !fs->private_data) {
        return VFS_EINVAL;
    }

    struct sfs_fs_data *data = (struct sfs_fs_data *)fs->private_data;
    if (!data->block_bitmap) {
        return VFS_EINVAL;
    }

    uint32_t bits_per_block = SFS_BLOCK_SIZE * 8;
    uint32_t bitmap_block = bit_index / bits_per_block;
    if (bitmap_block >= data->superblock.bitmap_blocks) {
        return VFS_EINVAL;
    }

    uint8_t *bitmap_ptr = data->block_bitmap + (bitmap_block * SFS_BLOCK_SIZE);
    uint32_t block_num = SFS_BITMAP_START + bitmap_block;

    return (block_device_write(data->device, block_num, bitmap_ptr) == BLOCK_SUCCESS) ?
           VFS_SUCCESS : VFS_EIO;
}

static int sfs_inode_location(struct sfs_fs_data *data, uint32_t inode_num,
                              uint32_t *block_num_out, uint32_t *offset_out)
{
    if (!data || inode_num == 0) {
        return VFS_EINVAL;
    }

    uint32_t index = inode_num - 1;
    uint32_t block_index = index / SFS_INODES_PER_BLOCK;
    if (block_index >= data->superblock.inode_blocks) {
        return VFS_EINVAL;
    }

    if (block_num_out) {
        *block_num_out = (data->superblock.first_data_block - data->superblock.inode_blocks) + block_index;
    }

    if (offset_out) {
        *offset_out = index % SFS_INODES_PER_BLOCK;
    }

    return VFS_SUCCESS;
}

static int sfs_read_inode_raw(struct file_system *fs, uint32_t inode_num, struct sfs_inode *out)
{
    if (!fs || !out || !fs->private_data) {
        return VFS_EINVAL;
    }

    struct sfs_fs_data *data = (struct sfs_fs_data *)fs->private_data;
    uint32_t block_num = 0;
    uint32_t offset = 0;

    if (sfs_inode_location(data, inode_num, &block_num, &offset) != VFS_SUCCESS) {
        return VFS_EINVAL;
    }

    void *buffer = kmalloc(SFS_BLOCK_SIZE);
    if (!buffer) {
        return VFS_ENOMEM;
    }

    int result = sfs_read_block(fs, block_num, buffer);
    if (result != VFS_SUCCESS) {
        kfree(buffer);
        return result;
    }

    struct sfs_inode *inodes = (struct sfs_inode *)buffer;
    memcpy(out, &inodes[offset], sizeof(struct sfs_inode));

    kfree(buffer);
    return VFS_SUCCESS;
}

static int sfs_write_inode_raw(struct file_system *fs, uint32_t inode_num, const struct sfs_inode *in)
{
    if (!fs || !in || !fs->private_data) {
        return VFS_EINVAL;
    }

    struct sfs_fs_data *data = (struct sfs_fs_data *)fs->private_data;
    uint32_t block_num = 0;
    uint32_t offset = 0;

    if (sfs_inode_location(data, inode_num, &block_num, &offset) != VFS_SUCCESS) {
        return VFS_EINVAL;
    }

    void *buffer = kmalloc(SFS_BLOCK_SIZE);
    if (!buffer) {
        return VFS_ENOMEM;
    }

    int result = sfs_read_block(fs, block_num, buffer);
    if (result != VFS_SUCCESS) {
        kfree(buffer);
        return result;
    }

    struct sfs_inode *inodes = (struct sfs_inode *)buffer;
    memcpy(&inodes[offset], in, sizeof(struct sfs_inode));

    result = sfs_write_block(fs, block_num, buffer);
    kfree(buffer);
    return result;
}

static uint32_t sfs_mode_to_vfs(uint32_t sfs_mode)
{
    uint32_t mode = 0;

    if (sfs_mode & SFS_TYPE_DIRECTORY) {
        mode |= VFS_FILE_DIRECTORY;
    } else if (sfs_mode & SFS_TYPE_SYMLINK) {
        mode |= VFS_FILE_SYMLINK;
    } else {
        mode |= VFS_FILE_REGULAR;
    }

    mode |= (sfs_mode & 0x0FFF);  // Preserve basic permission bits
    return mode;
}

static struct inode *sfs_allocate_vfs_inode(struct file_system *fs, uint32_t inode_num,
                                            const struct sfs_inode *disk_inode)
{
    struct inode *inode = kmalloc(sizeof(struct inode));
    if (!inode) {
        return NULL;
    }

    struct sfs_inode_data *inode_data = kmalloc(sizeof(struct sfs_inode_data));
    if (!inode_data) {
        kfree(inode);
        return NULL;
    }

    // Initialize with memset to ensure proper memory initialization
    memset(inode, 0, sizeof(struct inode));
    memset(inode_data, 0, sizeof(struct sfs_inode_data));

    // Full memory barrier to prevent compiler reordering
    __asm__ volatile("dmb ish" ::: "memory");

    // Set inode fields
    inode->ino = inode_num;
    inode->mode = sfs_mode_to_vfs(disk_inode->mode);
    inode->size = disk_inode->size;
    inode->blocks = disk_inode->blocks;
    inode->created_time = disk_inode->created_time;
    inode->modified_time = disk_inode->modified_time;
    inode->accessed_time = disk_inode->accessed_time;
    inode->fs = fs;
    inode->private_data = inode_data;
    inode->ref_count = 1;

    // Copy inode data field-by-field to prevent SIMD generation
    __asm__ volatile("dmb ish" ::: "memory");
    inode_data->disk_inode.mode = disk_inode->mode;
    __asm__ volatile("dmb ish" ::: "memory");
    inode_data->disk_inode.size = disk_inode->size;
    __asm__ volatile("dmb ish" ::: "memory");
    inode_data->disk_inode.blocks = disk_inode->blocks;
    __asm__ volatile("dmb ish" ::: "memory");
    for (int i = 0; i < SFS_DIRECT_BLOCKS; i++) {
        inode_data->disk_inode.direct[i] = disk_inode->direct[i];
    }
    __asm__ volatile("dmb ish" ::: "memory");
    inode_data->disk_inode.indirect = disk_inode->indirect;
    __asm__ volatile("dmb ish" ::: "memory");
    inode_data->disk_inode.created_time = disk_inode->created_time;
    __asm__ volatile("dmb ish" ::: "memory");
    inode_data->disk_inode.modified_time = disk_inode->modified_time;
    __asm__ volatile("dmb ish" ::: "memory");
    inode_data->disk_inode.accessed_time = disk_inode->accessed_time;
    __asm__ volatile("dmb ish" ::: "memory");
    inode_data->disk_inode.links = disk_inode->links;
    __asm__ volatile("dmb ish" ::: "memory");
    inode_data->disk_inode.flags = disk_inode->flags;
    __asm__ volatile("dmb ish" ::: "memory");
    // Copy reserved field byte-by-byte
    for (size_t i = 0; i < sizeof(disk_inode->reserved); i++) {
        inode_data->disk_inode.reserved[i] = disk_inode->reserved[i];
    }
    __asm__ volatile("dmb ish" ::: "memory");
    inode_data->inode_num = inode_num;
    inode_data->dirty = 0;
    __asm__ volatile("dmb ish" ::: "memory");

    // Final barrier before return
    __asm__ volatile("dmb ish" ::: "memory");

    return inode;
}

static void sfs_release_vfs_inode(struct inode *inode)
{
    if (!inode) {
        return;
    }

    if (inode->private_data) {
        kfree(inode->private_data);
        inode->private_data = NULL;
    }

    kfree(inode);
}

static int sfs_extract_parent(struct file_system *fs, const char *path,
                              struct inode **parent_out, char *name_out)
{
    if (!fs || !path || !parent_out || !name_out) {
        return VFS_EINVAL;
    }

    if (strcmp(path, "/") == 0) {
        return VFS_EINVAL;
    }

    char *dir_name = vfs_get_dirname(path);
    if (!dir_name) {
        return VFS_EINVAL;
    }

    struct inode *resolved_parent = sfs_resolve_path(fs, dir_name);

    if (!resolved_parent) {
        kfree(dir_name);
        return VFS_ENOENT;
    }

    const char *base_name = vfs_get_filename(path);
    if (!base_name || base_name[0] == '\0') {
        sfs_put_inode(resolved_parent);
        kfree(dir_name);
        return VFS_EINVAL;
    }

    strncpy(name_out, base_name, SFS_MAX_NAME - 1);
    name_out[SFS_MAX_NAME - 1] = '\0';

    *parent_out = resolved_parent;
    kfree(dir_name);
    return VFS_SUCCESS;
}

struct inode *sfs_get_inode(struct file_system *fs, uint32_t inode_num)
{
    if (!fs) {
        return NULL;
    }

    struct sfs_inode disk_inode;
    if (sfs_read_inode_raw(fs, inode_num, &disk_inode) != VFS_SUCCESS) {
        return NULL;
    }

    if (disk_inode.mode == 0) {
        return NULL;
    }

    return sfs_allocate_vfs_inode(fs, inode_num, &disk_inode);
}

static uint32_t sfs_calculate_mode(uint32_t requested_mode, uint32_t type_flag)
{
    uint32_t mode = type_flag;

    if (requested_mode & 0400) {
        mode |= SFS_PERM_READ;
    }
    if (requested_mode & 0200) {
        mode |= SFS_PERM_WRITE;
    }
    if (requested_mode & 0100) {
        mode |= SFS_PERM_EXEC;
    }

    return mode;
}

struct inode *sfs_alloc_inode(struct file_system *fs, uint32_t mode)
{
    if (!fs || !fs->private_data) {
        return NULL;
    }

    struct sfs_fs_data *data = (struct sfs_fs_data *)fs->private_data;
    uint32_t total_inodes = data->superblock.inode_blocks * SFS_INODES_PER_BLOCK;

    void *buffer = kmalloc(SFS_BLOCK_SIZE);
    if (!buffer) {
        return NULL;
    }

    struct inode *result_inode = NULL;

    for (uint32_t block_index = 0; block_index < data->superblock.inode_blocks; block_index++) {
        uint32_t block_num = (data->superblock.first_data_block - data->superblock.inode_blocks) + block_index;
        if (sfs_read_block(fs, block_num, buffer) != VFS_SUCCESS) {
            continue;
        }

        struct sfs_inode *inodes = (struct sfs_inode *)buffer;
        for (uint32_t entry = 0; entry < SFS_INODES_PER_BLOCK; entry++) {
            uint32_t global_index = block_index * SFS_INODES_PER_BLOCK + entry;
            if (global_index >= total_inodes) {
                break;
            }

            if (inodes[entry].mode != 0) {
                continue;
            }

            uint32_t inode_num = global_index + 1;

            struct sfs_inode new_inode;
            memset(&new_inode, 0, sizeof(new_inode));
            new_inode.mode = mode;
            new_inode.size = 0;
            new_inode.blocks = 0;
            new_inode.links = 1;
            new_inode.created_time = 0;
            new_inode.modified_time = 0;
            new_inode.accessed_time = 0;

            for (int i = 0; i < SFS_DIRECT_BLOCKS; i++) {
                new_inode.direct[i] = 0;
            }
            new_inode.indirect = 0;

            // Add memory barrier to prevent GCC vectorization from causing stack corruption
            __asm__ volatile("dmb ish" ::: "memory");
            inodes[entry] = new_inode;
            __asm__ volatile("dmb ish" ::: "memory");

            if (sfs_write_block(fs, block_num, buffer) != VFS_SUCCESS) {
                goto out;
            }

            if (data->superblock.free_inodes > 0) {
                data->superblock.free_inodes--;
            }
            sfs_sync_superblock(fs);

            result_inode = sfs_allocate_vfs_inode(fs, inode_num, &new_inode);
            goto out;
        }
    }

out:
    kfree(buffer);
    return result_inode;
}

void sfs_free_inode(struct file_system *fs, struct inode *inode)
{
    if (!fs || !inode || !fs->private_data) {
        return;
    }

    struct sfs_fs_data *data = (struct sfs_fs_data *)fs->private_data;

    struct sfs_inode empty_inode;
    memset(&empty_inode, 0, sizeof(empty_inode));

    sfs_write_inode_raw(fs, inode->ino, &empty_inode);

    data->superblock.free_inodes++;
    sfs_sync_superblock(fs);

    sfs_release_vfs_inode(inode);
}

int sfs_sync_inode(struct inode *inode)
{
    if (!inode || !inode->private_data || !inode->fs) {
        return VFS_EINVAL;
    }

    struct sfs_inode_data *inode_data = (struct sfs_inode_data *)inode->private_data;
    if (!inode_data->dirty) {
        return VFS_SUCCESS;
    }

    // Use helper function to prevent GCC SIMD vectorization
    sfs_sync_inode_data_safe(&inode_data->disk_inode, inode);

    int result = sfs_write_inode_raw(inode->fs, inode_data->inode_num, &inode_data->disk_inode);
    if (result == VFS_SUCCESS) {
        inode_data->dirty = 0;
    }

    return result;
}

int sfs_put_inode(struct inode *inode)
{
    if (!inode) {
        return VFS_EINVAL;
    }

    int result = sfs_sync_inode(inode);
    sfs_release_vfs_inode(inode);
    return result;
}

struct inode *sfs_resolve_path(struct file_system *fs, const char *path)
{
    if (!fs || !path || !fs->private_data) {
        return NULL;
    }

    struct sfs_fs_data *data = (struct sfs_fs_data *)fs->private_data;

    if (strcmp(path, "/") == 0) {
        return sfs_get_inode(fs, data->superblock.root_inode);
    }

    struct inode *current = sfs_get_inode(fs, data->superblock.root_inode);
    if (!current) {
        return NULL;
    }

    const char *cursor = path;
    while (*cursor == '/') {
        cursor++;
    }

    char component[SFS_MAX_NAME];

    while (*cursor) {
        size_t len = 0;
        while (cursor[len] && cursor[len] != '/') {
            len++;
        }

        if (len == 0 || len >= SFS_MAX_NAME) {
            sfs_put_inode(current);
            return NULL;
        }

        memcpy(component, cursor, len);
        component[len] = '\0';

        struct inode *next = sfs_dir_lookup(fs, current, component);
        if (!next) {
            sfs_put_inode(current);
            return NULL;
        }

        sfs_put_inode(current);
        current = next;

        cursor += len;
        while (*cursor == '/') {
            cursor++;
        }
    }

    return current;
}

int sfs_add_dirent(struct file_system *fs, struct inode *dir_inode, const char *name, uint32_t inode_num)
{
    if (!fs || !dir_inode || !name || !dir_inode->private_data) {
        return VFS_EINVAL;
    }

    struct sfs_inode_data *dir_data = (struct sfs_inode_data *)dir_inode->private_data;
    struct sfs_inode *disk_inode = &dir_data->disk_inode;

    if ((disk_inode->mode & SFS_TYPE_DIRECTORY) == 0) {
        return VFS_EINVAL;
    }

    void *block_buffer = kmalloc(SFS_BLOCK_SIZE);
    if (!block_buffer) {
        return VFS_ENOMEM;
    }

    int result = VFS_ENOSPC;
    int entries_per_block = SFS_BLOCK_SIZE / sizeof(struct sfs_dirent);

    for (uint32_t block_index = 0; block_index < SFS_DIRECT_BLOCKS; block_index++) {
        uint32_t block_num = disk_inode->direct[block_index];

        if (block_num == 0) {
            block_num = sfs_alloc_block(fs);
            if (block_num == 0) {
                result = VFS_ENOSPC;
                goto out;
            }
            disk_inode->direct[block_index] = block_num;
            memset(block_buffer, 0, SFS_BLOCK_SIZE);
            dir_inode->blocks++;
            disk_inode->blocks = dir_inode->blocks;
        } else {
            if (sfs_read_block(fs, block_num, block_buffer) != VFS_SUCCESS) {
                result = VFS_EIO;
                goto out;
            }
        }

        struct sfs_dirent *entries = (struct sfs_dirent *)block_buffer;
        for (int i = 0; i < entries_per_block; i++) {
            if (entries[i].inode != 0) {
                if (strncmp(entries[i].name, name, SFS_MAX_NAME) == 0) {
                    result = VFS_EEXIST;
                    goto out;
                }
                continue;
            }

            entries[i].inode = inode_num;
            size_t name_len = strlen(name);
            if (name_len >= SFS_MAX_NAME) {
                name_len = SFS_MAX_NAME - 1;
            }
            entries[i].name_len = (uint16_t)name_len;
            entries[i].rec_len = sizeof(struct sfs_dirent);
            strncpy(entries[i].name, name, SFS_MAX_NAME - 1);
            entries[i].name[SFS_MAX_NAME - 1] = '\0';

            if (sfs_write_block(fs, block_num, block_buffer) != VFS_SUCCESS) {
                entries[i].inode = 0;
                result = VFS_EIO;
                goto out;
            }

            disk_inode->size += sizeof(struct sfs_dirent);
            dir_inode->size = disk_inode->size;
            dir_data->dirty = 1;
            sfs_sync_inode(dir_inode);

            result = VFS_SUCCESS;
            goto out;
        }
    }

out:
    kfree(block_buffer);
    return result;
}

int sfs_remove_dirent(struct file_system *fs, struct inode *dir_inode, const char *name)
{
    if (!fs || !dir_inode || !name || !dir_inode->private_data) {
        return VFS_EINVAL;
    }

    struct sfs_inode_data *dir_data = (struct sfs_inode_data *)dir_inode->private_data;
    struct sfs_inode *disk_inode = &dir_data->disk_inode;

    if ((disk_inode->mode & SFS_TYPE_DIRECTORY) == 0) {
        return VFS_EINVAL;
    }

    void *block_buffer = kmalloc(SFS_BLOCK_SIZE);
    if (!block_buffer) {
        return VFS_ENOMEM;
    }

    int result = VFS_ENOENT;
    int entries_per_block = SFS_BLOCK_SIZE / sizeof(struct sfs_dirent);

    for (uint32_t block_index = 0; block_index < SFS_DIRECT_BLOCKS; block_index++) {
        uint32_t block_num = disk_inode->direct[block_index];
        if (block_num == 0) {
            continue;
        }

        if (sfs_read_block(fs, block_num, block_buffer) != VFS_SUCCESS) {
            result = VFS_EIO;
            goto out;
        }

        struct sfs_dirent *entries = (struct sfs_dirent *)block_buffer;
        for (int i = 0; i < entries_per_block; i++) {
            if (entries[i].inode == 0) {
                continue;
            }

            if (strncmp(entries[i].name, name, SFS_MAX_NAME) != 0) {
                continue;
            }

            entries[i].inode = 0;
            entries[i].name[0] = '\0';
            entries[i].name_len = 0;

            if (sfs_write_block(fs, block_num, block_buffer) != VFS_SUCCESS) {
                result = VFS_EIO;
                goto out;
            }

            if (disk_inode->size >= sizeof(struct sfs_dirent)) {
                disk_inode->size -= sizeof(struct sfs_dirent);
            } else {
                disk_inode->size = 0;
            }
            dir_inode->size = disk_inode->size;
            dir_data->dirty = 1;
            sfs_sync_inode(dir_inode);

            result = VFS_SUCCESS;
            goto out;
        }
    }

out:
    kfree(block_buffer);
    return result;
}

int sfs_create_file(struct file_system *fs, const char *path, uint32_t mode)
{
    if (!fs || !path) {
        return VFS_EINVAL;
    }

    char name[SFS_MAX_NAME];
    struct inode *parent = NULL;
    int result = sfs_extract_parent(fs, path, &parent, name);
    if (result != VFS_SUCCESS) {
        return result;
    }

    struct inode *existing = sfs_dir_lookup(fs, parent, name);
    if (existing) {
        sfs_put_inode(existing);
        sfs_put_inode(parent);
        return VFS_EEXIST;
    }

    uint32_t file_mode = sfs_calculate_mode(mode ? mode : 0644, SFS_TYPE_FILE);

    struct inode *file_inode = sfs_alloc_inode(fs, file_mode);
    if (!file_inode) {
        sfs_put_inode(parent);
        return VFS_ENOSPC;
    }

    result = sfs_add_dirent(fs, parent, name, file_inode->ino);
    if (result != VFS_SUCCESS) {
        sfs_free_inode(fs, file_inode);
        sfs_put_inode(parent);
        return result;
    }

    struct sfs_inode_data *file_data = (struct sfs_inode_data *)file_inode->private_data;
    if (file_data) {
        file_data->disk_inode.mode = file_mode;
        file_data->disk_inode.links = 1;
        file_data->disk_inode.size = 0;
        file_data->disk_inode.blocks = 0;
        file_data->dirty = 1;
        sfs_sync_inode(file_inode);
    }

    sfs_put_inode(file_inode);
    sfs_put_inode(parent);
    return VFS_SUCCESS;
}

int sfs_delete_file(struct file_system *fs, const char *path)
{
    if (!fs || !path) {
        return VFS_EINVAL;
    }

    char name[SFS_MAX_NAME];
    struct inode *parent = NULL;
    int result = sfs_extract_parent(fs, path, &parent, name);
    if (result != VFS_SUCCESS) {
        return result;
    }

    struct inode *target = sfs_dir_lookup(fs, parent, name);
    if (!target) {
        sfs_put_inode(parent);
        return VFS_ENOENT;
    }

    struct sfs_inode_data *target_data = (struct sfs_inode_data *)target->private_data;
    if (!target_data || (target_data->disk_inode.mode & SFS_TYPE_DIRECTORY)) {
        sfs_put_inode(target);
        sfs_put_inode(parent);
        return VFS_EPERM;
    }

    // Free data blocks
    for (int i = 0; i < SFS_DIRECT_BLOCKS; i++) {
        uint32_t block_num = target_data->disk_inode.direct[i];
        if (block_num) {
            sfs_free_block(fs, block_num);
            target_data->disk_inode.direct[i] = 0;
        }
    }

    if (target_data->disk_inode.indirect) {
        sfs_free_block(fs, target_data->disk_inode.indirect);
        target_data->disk_inode.indirect = 0;
    }

    result = sfs_remove_dirent(fs, parent, name);
    if (result == VFS_SUCCESS) {
        sfs_free_inode(fs, target);
    } else {
        sfs_put_inode(target);
    }

    sfs_put_inode(parent);
    return result;
}

int sfs_truncate_file(struct file_system *fs, struct inode *inode, size_t new_size)
{
    if (!fs || !inode || !inode->private_data) {
        return VFS_EINVAL;
    }

    if (new_size != 0) {
        return VFS_EPERM;  // Only support truncate to zero for now
    }

    struct sfs_inode_data *inode_data = (struct sfs_inode_data *)inode->private_data;

    for (int i = 0; i < SFS_DIRECT_BLOCKS; i++) {
        uint32_t block_num = inode_data->disk_inode.direct[i];
        if (block_num) {
            sfs_free_block(fs, block_num);
            inode_data->disk_inode.direct[i] = 0;
        }
    }

    if (inode_data->disk_inode.indirect) {
        sfs_free_block(fs, inode_data->disk_inode.indirect);
        inode_data->disk_inode.indirect = 0;
    }

    inode_data->disk_inode.size = 0;
    inode_data->disk_inode.blocks = 0;
    inode->size = 0;
    inode->blocks = 0;
    inode_data->dirty = 1;
    return sfs_sync_inode(inode);
}

int sfs_create_directory(struct file_system *fs, const char *path, uint32_t mode)
{
    return sfs_dir_mkdir(fs, path, (int)mode);
}

int sfs_delete_directory(struct file_system *fs, const char *path)
{
    return sfs_dir_rmdir(fs, path);
}

struct inode *sfs_lookup(struct file_system *fs, struct inode *parent, const char *name)
{
    return sfs_dir_lookup(fs, parent, name);
}

int sfs_is_directory(const struct sfs_inode *inode)
{
    return inode && (inode->mode & SFS_TYPE_DIRECTORY);
}

int sfs_is_file(const struct sfs_inode *inode)
{
    return inode && (inode->mode & SFS_TYPE_FILE);
}

// File operations implementations
static int sfs_file_open(struct file *file, int flags, int mode)
{
    (void)flags; (void)mode;
    
    if (!file || !file->inode) {
        return VFS_EINVAL;
    }
    
    // For now, just return success
    return VFS_SUCCESS;
}

static int sfs_file_close(struct file *file)
{
    if (!file) {
        return VFS_EINVAL;
    }

    if (file->inode && file->inode->fs && file->inode->fs->type == &sfs_fs_type) {
        sfs_sync_inode(file->inode);
    }

    // Sync any dirty data
    if (file->fs && file->fs->device) {
        block_device_sync(file->fs->device);
    }

    return VFS_SUCCESS;
}

static ssize_t sfs_file_read(struct file *file, void *buf, size_t count, off_t offset)
{
    if (!file || !file->inode || !file->fs || !buf) {
        return -1;
    }
    
    struct sfs_inode_data *inode_data = (struct sfs_inode_data *)file->inode->private_data;
    if (!inode_data) {
        return -1;
    }
    
    struct sfs_inode *disk_inode = &inode_data->disk_inode;
    
    // Check bounds
    if (offset >= (off_t)disk_inode->size) {
        return 0;  // EOF
    }
    
    // Calculate actual bytes to read
    size_t bytes_to_read = count;
    if (offset + (off_t)bytes_to_read > (off_t)disk_inode->size) {
        bytes_to_read = disk_inode->size - offset;
    }
    
    // Read from direct blocks
    uint8_t *dest = (uint8_t *)buf;
    size_t bytes_read = 0;
    
    void *block_buffer = kmalloc(SFS_BLOCK_SIZE);
    if (!block_buffer) {
        return -1;
    }
    
    while (bytes_read < bytes_to_read) {
        uint32_t block_index = (offset + bytes_read) / SFS_BLOCK_SIZE;
        uint32_t block_offset = (offset + bytes_read) % SFS_BLOCK_SIZE;
        
        // Only support direct blocks for now
        if (block_index >= SFS_DIRECT_BLOCKS) {
            break;  // Indirect blocks not implemented
        }
        
        uint32_t block_num = disk_inode->direct[block_index];
        if (block_num == 0) {
            break;  // No more blocks allocated
        }
        
        // Read block
        if (sfs_read_block(file->fs, block_num, block_buffer) != VFS_SUCCESS) {
            break;
        }
        
        // Copy data from block
        size_t copy_size = SFS_BLOCK_SIZE - block_offset;
        if (copy_size > bytes_to_read - bytes_read) {
            copy_size = bytes_to_read - bytes_read;
        }
        
        memcpy(dest + bytes_read, (uint8_t *)block_buffer + block_offset, copy_size);
        bytes_read += copy_size;
    }
    
    kfree(block_buffer);
    return bytes_read;
}

static ssize_t sfs_file_write(struct file *file, const void *buf, size_t count, off_t offset)
{
    if (!file || !file->inode || !file->fs || !buf) {
        return -1;
    }
    
    struct sfs_inode_data *inode_data = (struct sfs_inode_data *)file->inode->private_data;
    if (!inode_data) {
        return -1;
    }
    
    struct sfs_inode *disk_inode = &inode_data->disk_inode;
    
    // Write to direct blocks
    const uint8_t *src = (const uint8_t *)buf;
    size_t bytes_written = 0;
    
    void *block_buffer = kmalloc(SFS_BLOCK_SIZE);
    if (!block_buffer) {
        return -1;
    }
    
    while (bytes_written < count) {
        uint32_t block_index = (offset + bytes_written) / SFS_BLOCK_SIZE;
        uint32_t block_offset = (offset + bytes_written) % SFS_BLOCK_SIZE;
        
        // Only support direct blocks for now
        if (block_index >= SFS_DIRECT_BLOCKS) {
            break;  // Indirect blocks not implemented
        }
        
        uint32_t block_num = disk_inode->direct[block_index];
        
        // Allocate block if needed
        if (block_num == 0) {
            block_num = sfs_alloc_block(file->fs);
            if (block_num == 0) {
                break;  // Out of space
            }
            disk_inode->direct[block_index] = block_num;
            disk_inode->blocks++;
            file->inode->blocks = disk_inode->blocks;
            inode_data->dirty = 1;
        }
        
        // Read existing block if we're doing a partial write
        if (block_offset != 0 || (count - bytes_written) < SFS_BLOCK_SIZE) {
            if (sfs_read_block(file->fs, block_num, block_buffer) != VFS_SUCCESS) {
                memset(block_buffer, 0, SFS_BLOCK_SIZE);
            }
        }
        
        // Copy data to block
        size_t copy_size = SFS_BLOCK_SIZE - block_offset;
        if (copy_size > count - bytes_written) {
            copy_size = count - bytes_written;
        }
        
        memcpy((uint8_t *)block_buffer + block_offset, src + bytes_written, copy_size);
        
        // Write block
        if (sfs_write_block(file->fs, block_num, block_buffer) != VFS_SUCCESS) {
            break;
        }
        
        bytes_written += copy_size;
    }
    
    kfree(block_buffer);
    
    // Update file size if needed
    if (offset + (off_t)bytes_written > (off_t)disk_inode->size) {
        disk_inode->size = offset + bytes_written;
        file->inode->size = disk_inode->size;
        inode_data->dirty = 1;
    }
    
    return bytes_written;
}

static off_t sfs_file_seek(struct file *file, off_t offset, int whence)
{
    if (!file) {
        return -1;
    }
    
    off_t new_pos = 0;
    
    switch (whence) {
        case VFS_SEEK_SET:
            new_pos = offset;
            break;
        case VFS_SEEK_CUR:
            new_pos = file->position + offset;
            break;
        case VFS_SEEK_END:
            new_pos = file->inode->size + offset;
            break;
        default:
            return -1;
    }
    
    if (new_pos < 0) {
        return -1;
    }
    
    file->position = new_pos;
    return new_pos;
}

static int sfs_file_sync(struct file *file)
{
    if (!file || !file->fs) {
        return VFS_EINVAL;
    }

    struct sfs_fs_data *data = (struct sfs_fs_data *)file->fs->private_data;
    if (!data) {
        return VFS_EINVAL;
    }

    if (file->inode) {
        sfs_sync_inode(file->inode);
    }

    // Sync block device
    return (block_device_sync(data->device) == BLOCK_SUCCESS) ? VFS_SUCCESS : VFS_EIO;
}

// Directory operations implementations
static int sfs_dir_readdir(struct file *dir, void *buffer, size_t buffer_size, off_t *offset)
{
    if (!dir || !dir->inode || !dir->fs || !buffer || !offset) {
        return -1;
    }
    
    struct sfs_inode_data *inode_data = (struct sfs_inode_data *)dir->inode->private_data;
    if (!inode_data) {
        return -1;
    }
    
    struct sfs_inode *disk_inode = &inode_data->disk_inode;
    
    // Check if it's a directory
    if ((disk_inode->mode & SFS_TYPE_DIRECTORY) == 0) {
        return -1;
    }
    
    struct dirent *entries = (struct dirent *)buffer;
    int max_entries = buffer_size / sizeof(struct dirent);
    int count = 0;
    
    // Read directory entries from disk
    void *block_buffer = kmalloc(SFS_BLOCK_SIZE);
    if (!block_buffer) {
        return -1;
    }
    
    int current_offset = 0;
    
    // Scan through directory blocks
    for (uint32_t block_idx = 0; block_idx < SFS_DIRECT_BLOCKS && count < max_entries; block_idx++) {
        uint32_t block_num = disk_inode->direct[block_idx];
        if (block_num == 0) {
            break;  // No more blocks
        }
        
        // Read directory block
        if (sfs_read_block(dir->fs, block_num, block_buffer) != VFS_SUCCESS) {
            break;
        }
        
        // Parse directory entries in this block
        struct sfs_dirent *sfs_entries = (struct sfs_dirent *)block_buffer;
        int entries_per_block = SFS_BLOCK_SIZE / sizeof(struct sfs_dirent);
        
        for (int i = 0; i < entries_per_block && count < max_entries; i++) {
            if (sfs_entries[i].inode == 0) {
                continue;  // Empty entry
            }

            // Skip to offset
            if (current_offset < *offset) {
                current_offset++;
                continue;
            }

            struct inode *child = sfs_get_inode(dir->fs, sfs_entries[i].inode);
            if (!child) {
                current_offset++;
                continue;
            }

            entries[count].ino = sfs_entries[i].inode;
            entries[count].name_len = sfs_entries[i].name_len;
            strncpy(entries[count].name, sfs_entries[i].name, VFS_MAX_NAME - 1);
            entries[count].name[VFS_MAX_NAME - 1] = '\0';

            if (child->mode & VFS_FILE_DIRECTORY) {
                entries[count].type = VFS_FILE_DIRECTORY;
            } else if (child->mode & VFS_FILE_SYMLINK) {
                entries[count].type = VFS_FILE_SYMLINK;
            } else {
                entries[count].type = VFS_FILE_REGULAR;
            }

            sfs_put_inode(child);

            count++;
            (*offset)++;
            current_offset++;
        }
    }
    
    kfree(block_buffer);
    return count;
}

static int sfs_dir_mkdir(struct file_system *fs, const char *path, int mode)
{
    if (!fs || !path) {
        return VFS_EINVAL;
    }

    if (strcmp(path, "/") == 0) {
        return VFS_EEXIST;
    }

    char name[SFS_MAX_NAME];
    struct inode *parent = NULL;
    int result = sfs_extract_parent(fs, path, &parent, name);
    if (result != VFS_SUCCESS) {
        return result;
    }

    struct inode *existing = sfs_dir_lookup(fs, parent, name);
    if (existing) {
        sfs_put_inode(existing);
        sfs_put_inode(parent);
        return VFS_EEXIST;
    }

    uint32_t dir_mode = sfs_calculate_mode(mode ? mode : 0755, SFS_TYPE_DIRECTORY);
    dir_mode |= SFS_PERM_EXEC;  // Ensure directories are traversable

    struct inode *dir_inode = sfs_alloc_inode(fs, dir_mode);
    if (!dir_inode) {
        sfs_put_inode(parent);
        return VFS_ENOSPC;
    }

    result = sfs_add_dirent(fs, parent, name, dir_inode->ino);
    if (result != VFS_SUCCESS) {
        sfs_free_inode(fs, dir_inode);
        sfs_put_inode(parent);
        return result;
    }

    struct sfs_inode_data *dir_data = (struct sfs_inode_data *)dir_inode->private_data;
    if (dir_data) {
        dir_data->disk_inode.mode = dir_mode;
        dir_data->disk_inode.links = 1;
        dir_data->dirty = 1;
        sfs_sync_inode(dir_inode);
    }

    sfs_put_inode(dir_inode);
    sfs_put_inode(parent);
    return VFS_SUCCESS;
}

static int sfs_dir_rmdir(struct file_system *fs, const char *path)
{
    if (!fs || !path) {
        return VFS_EINVAL;
    }

    if (strcmp(path, "/") == 0) {
        return VFS_EPERM;
    }

    char name[SFS_MAX_NAME];
    struct inode *parent = NULL;
    int result = sfs_extract_parent(fs, path, &parent, name);
    if (result != VFS_SUCCESS) {
        return result;
    }

    struct inode *target = sfs_dir_lookup(fs, parent, name);
    if (!target) {
        sfs_put_inode(parent);
        return VFS_ENOENT;
    }

    struct sfs_inode_data *target_data = (struct sfs_inode_data *)target->private_data;
    if (!target_data || (target_data->disk_inode.mode & SFS_TYPE_DIRECTORY) == 0) {
        sfs_put_inode(target);
        sfs_put_inode(parent);
        return VFS_EPERM;
    }

    void *block_buffer = kmalloc(SFS_BLOCK_SIZE);
    if (!block_buffer) {
        sfs_put_inode(target);
        sfs_put_inode(parent);
        return VFS_ENOMEM;
    }

    int entries_per_block = SFS_BLOCK_SIZE / sizeof(struct sfs_dirent);
    for (uint32_t block_index = 0; block_index < SFS_DIRECT_BLOCKS; block_index++) {
        uint32_t block_num = target_data->disk_inode.direct[block_index];
        if (block_num == 0) {
            continue;
        }

        if (sfs_read_block(fs, block_num, block_buffer) != VFS_SUCCESS) {
            kfree(block_buffer);
            sfs_put_inode(target);
            sfs_put_inode(parent);
            return VFS_EIO;
        }

        struct sfs_dirent *entries = (struct sfs_dirent *)block_buffer;
        for (int i = 0; i < entries_per_block; i++) {
            if (entries[i].inode != 0) {
                kfree(block_buffer);
                sfs_put_inode(target);
                sfs_put_inode(parent);
                return VFS_EPERM;  // Directory not empty
            }
        }
    }

    kfree(block_buffer);

    // Free any allocated blocks (should be zero for empty directory but safe)
    for (int i = 0; i < SFS_DIRECT_BLOCKS; i++) {
        uint32_t block_num = target_data->disk_inode.direct[i];
        if (block_num) {
            sfs_free_block(fs, block_num);
            target_data->disk_inode.direct[i] = 0;
        }
    }

    if (target_data->disk_inode.indirect) {
        sfs_free_block(fs, target_data->disk_inode.indirect);
        target_data->disk_inode.indirect = 0;
    }

    result = sfs_remove_dirent(fs, parent, name);
    if (result == VFS_SUCCESS) {
        sfs_free_inode(fs, target);
    } else {
        sfs_put_inode(target);
    }

    sfs_put_inode(parent);
    return result;
}

static struct inode *sfs_dir_lookup(struct file_system *fs, struct inode *parent, const char *name)
{
    if (!fs || !parent || !name) {
        return NULL;
    }

    struct sfs_inode_data *parent_data = (struct sfs_inode_data *)parent->private_data;
    if (!parent_data) {
        return NULL;
    }

    struct sfs_inode *parent_inode = &parent_data->disk_inode;

    // Check if parent is a directory
    if ((parent_inode->mode & SFS_TYPE_DIRECTORY) == 0) {
        return NULL;
    }

    // Search for name in directory entries
    void *block_buffer = kmalloc(SFS_BLOCK_SIZE);
    if (!block_buffer) {
        return NULL;
    }

    // Memory barrier after allocation
    __asm__ volatile("dmb ish" ::: "memory");

    struct inode *found_inode = NULL;
    
    // Scan through directory blocks
    for (uint32_t block_idx = 0; block_idx < SFS_DIRECT_BLOCKS; block_idx++) {
        uint32_t block_num = parent_inode->direct[block_idx];
        if (block_num == 0) {
            break;  // No more blocks
        }
        
        // Read directory block
        if (sfs_read_block(fs, block_num, (void*)block_buffer) != VFS_SUCCESS) {
            break;
        }
        
        // Parse directory entries in this block
        struct sfs_dirent *entries = (struct sfs_dirent *)block_buffer;
        int entries_per_block = SFS_BLOCK_SIZE / sizeof(struct sfs_dirent);
        
        for (int i = 0; i < entries_per_block; i++) {
            if (entries[i].inode == 0) {
                continue;  // Empty entry
            }

            if (strncmp(entries[i].name, name, SFS_MAX_NAME) == 0) {
                found_inode = sfs_get_inode(fs, entries[i].inode);
                // Memory barrier after getting inode
                __asm__ volatile("dmb ish" ::: "memory");
                break;
            }
        }

        if (found_inode) {
            break;
        }
    }

    // Memory barrier before cleanup and return
    __asm__ volatile("dmb ish" ::: "memory");

    kfree(block_buffer);
    return found_inode;
}

// Debugging functions
void sfs_dump_superblock(const struct sfs_superblock *sb)
{
    if (!sb) {
        early_print("Superblock: NULL\n");
        return;
    }
    
    early_print("SFS Superblock:\n");
    early_print("  Magic: 0x");
    
    // Print hex (simplified)
    uint32_t magic = sb->magic;
    char hex_chars[] = "0123456789ABCDEF";
    char hex_str[9];
    for (int i = 7; i >= 0; i--) {
        hex_str[7-i] = hex_chars[(magic >> (i*4)) & 0xF];
    }
    hex_str[8] = '\0';
    early_print(hex_str);
    early_print("\n");
    
    early_print("  Version: ");
    char num_str[16];
    int val = sb->version;
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
        for (int i = temp_pos - 1; i >= 0; i--) {
            num_str[pos++] = temp[i];
        }
    }
    num_str[pos] = '\0';
    early_print(num_str);
    early_print("\n");
    
    early_print("  Total blocks: ");
    val = sb->total_blocks;
    pos = 0;
    if (val == 0) {
        num_str[pos++] = '0';
    } else {
        char temp[16];
        int temp_pos = 0;
        while (val > 0) {
            temp[temp_pos++] = '0' + (val % 10);
            val /= 10;
        }
        for (int i = temp_pos - 1; i >= 0; i--) {
            num_str[pos++] = temp[i];
        }
    }
    num_str[pos] = '\0';
    early_print(num_str);
    early_print("\n");
    
    early_print("  Free blocks: ");
    val = sb->free_blocks;
    pos = 0;
    if (val == 0) {
        num_str[pos++] = '0';
    } else {
        char temp[16];
        int temp_pos = 0;
        while (val > 0) {
            temp[temp_pos++] = '0' + (val % 10);
            val /= 10;
        }
        for (int i = temp_pos - 1; i >= 0; i--) {
            num_str[pos++] = temp[i];
        }
    }
    num_str[pos] = '\0';
    early_print(num_str);
    early_print("\n");
    
    early_print("  Label: ");
    early_print(sb->label);
    early_print("\n");
}

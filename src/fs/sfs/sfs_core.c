/*
 * MiniOS Simple File System (SFS) Core
 * Basic file system implementation for educational purposes
 */

#include "sfs.h"
#include "memory.h"
#include "kernel.h"

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
    
    // Initialize block bitmap (all blocks free initially)
    void *bitmap_block = kmalloc(SFS_BLOCK_SIZE);
    if (!bitmap_block) {
        early_print("Failed to allocate bitmap block\n");
        return VFS_ENOMEM;
    }
    
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
    
    memset(inode_block, 0, SFS_BLOCK_SIZE);
    
    // Create root directory inode
    struct sfs_inode *root_inode = (struct sfs_inode *)inode_block;
    root_inode->mode = SFS_TYPE_DIRECTORY | SFS_PERM_READ | SFS_PERM_WRITE | SFS_PERM_EXEC;
    root_inode->size = 0;
    root_inode->blocks = 0;
    root_inode->created_time = 0;
    root_inode->modified_time = 0;
    root_inode->accessed_time = 0;
    root_inode->links = 1;
    root_inode->flags = 0;
    
    // Initialize direct blocks to 0 (no blocks allocated yet)
    for (int i = 0; i < SFS_DIRECT_BLOCKS; i++) {
        root_inode->direct[i] = 0;
    }
    root_inode->indirect = 0;
    
    // Write first inode block (contains root inode)
    if (block_device_write(dev, sb.first_data_block - inode_blocks, inode_block) != BLOCK_SUCCESS) {
        early_print("Failed to write root inode\n");
        kfree(inode_block);
        return VFS_ERROR;
    }
    
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

// File operations implementations (placeholders)
static int sfs_file_open(struct file *file, int flags, int mode)
{
    (void)file; (void)flags; (void)mode;
    return VFS_SUCCESS;
}

static int sfs_file_close(struct file *file)
{
    (void)file;
    return VFS_SUCCESS;
}

static ssize_t sfs_file_read(struct file *file, void *buf, size_t count, off_t offset)
{
    (void)file; (void)buf; (void)count; (void)offset;
    return 0;  // Placeholder: read 0 bytes
}

static ssize_t sfs_file_write(struct file *file, const void *buf, size_t count, off_t offset)
{
    (void)file; (void)buf; (void)count; (void)offset;
    return 0;  // Placeholder: write 0 bytes
}

static off_t sfs_file_seek(struct file *file, off_t offset, int whence)
{
    (void)file; (void)whence;
    return offset;  // Placeholder
}

static int sfs_file_sync(struct file *file)
{
    (void)file;
    return VFS_SUCCESS;
}

// Directory operations implementations (placeholders)
static int sfs_dir_readdir(struct file *dir, void *buffer, size_t buffer_size, off_t *offset)
{
    (void)dir; (void)buffer; (void)buffer_size; (void)offset;
    return 0;  // Placeholder: no directory entries
}

static int sfs_dir_mkdir(struct file_system *fs, const char *path, int mode)
{
    (void)fs; (void)path; (void)mode;
    return VFS_SUCCESS;  // Placeholder
}

static int sfs_dir_rmdir(struct file_system *fs, const char *path)
{
    (void)fs; (void)path;
    return VFS_SUCCESS;  // Placeholder
}

static struct inode *sfs_dir_lookup(struct file_system *fs, struct inode *parent, const char *name)
{
    (void)fs; (void)parent; (void)name;
    return NULL;  // Placeholder: file not found
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
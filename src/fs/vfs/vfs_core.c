/*
 * MiniOS Virtual File System Core
 * Unified interface for all file systems
 */

#include "vfs.h"
#include "block_device.h"
#include "memory.h"
#include "kernel.h"

// Maximum number of registered file system types
#define MAX_FS_TYPES        16

// Maximum number of mount points
#define MAX_MOUNTS          16

// VFS state
static int vfs_initialized = 0;
static struct file_system_type *fs_types[MAX_FS_TYPES];
static struct vfs_mount *mounts[MAX_MOUNTS];
static struct vfs_mount *root_mount = NULL;

// VFS statistics
static struct {
    uint32_t fs_types_registered;
    uint32_t mounts_active;
    uint64_t files_opened;
    uint64_t files_closed;
} vfs_stats = {0};

int vfs_init(void)
{
    if (vfs_initialized) {
        return VFS_SUCCESS;
    }
    
    early_print("Initializing Virtual File System...\n");
    
    // Initialize file system types array
    for (int i = 0; i < MAX_FS_TYPES; i++) {
        fs_types[i] = NULL;
    }
    
    // Initialize mount points array
    for (int i = 0; i < MAX_MOUNTS; i++) {
        mounts[i] = NULL;
    }
    
    root_mount = NULL;
    vfs_initialized = 1;
    
    // Reset statistics
    vfs_stats.fs_types_registered = 0;
    vfs_stats.mounts_active = 0;
    vfs_stats.files_opened = 0;
    vfs_stats.files_closed = 0;
    
    early_print("VFS initialized\n");
    return VFS_SUCCESS;
}

int vfs_shutdown(void)
{
    if (!vfs_initialized) {
        return VFS_SUCCESS;
    }
    
    early_print("Shutting down VFS...\n");
    
    // Unmount all file systems
    for (int i = 0; i < MAX_MOUNTS; i++) {
        if (mounts[i]) {
            vfs_unmount(mounts[i]->mountpoint);
        }
    }
    
    // Clear file system types
    for (int i = 0; i < MAX_FS_TYPES; i++) {
        fs_types[i] = NULL;
    }
    
    vfs_initialized = 0;
    root_mount = NULL;
    
    early_print("VFS shutdown complete\n");
    return VFS_SUCCESS;
}

int vfs_register_filesystem(struct file_system_type *fs_type)
{
    if (!vfs_initialized || !fs_type || !fs_type->name) {
        return VFS_EINVAL;
    }
    
    // Find empty slot
    for (int i = 0; i < MAX_FS_TYPES; i++) {
        if (fs_types[i] == NULL) {
            fs_types[i] = fs_type;
            vfs_stats.fs_types_registered++;
            
            early_print("Registered filesystem type: ");
            early_print(fs_type->name);
            early_print("\n");
            
            return VFS_SUCCESS;
        }
    }
    
    early_print("Failed to register filesystem: too many types\n");
    return VFS_ENOMEM;
}

int vfs_unregister_filesystem(const char *name)
{
    if (!vfs_initialized || !name) {
        return VFS_EINVAL;
    }
    
    // Find and remove filesystem type
    for (int i = 0; i < MAX_FS_TYPES; i++) {
        if (fs_types[i] && strcmp(fs_types[i]->name, name) == 0) {
            fs_types[i] = NULL;
            if (vfs_stats.fs_types_registered > 0) {
                vfs_stats.fs_types_registered--;
            }
            return VFS_SUCCESS;
        }
    }
    
    return VFS_ENOENT;
}

static struct file_system_type *vfs_find_filesystem_type(const char *name)
{
    if (!name) {
        return NULL;
    }
    
    for (int i = 0; i < MAX_FS_TYPES; i++) {
        if (fs_types[i] && strcmp(fs_types[i]->name, name) == 0) {
            return fs_types[i];
        }
    }
    
    return NULL;
}

int vfs_mount(const char *device, const char *mountpoint, const char *fstype, unsigned long flags)
{
    if (!vfs_initialized || !device || !mountpoint || !fstype) {
        return VFS_EINVAL;
    }
    
    early_print("Mounting ");
    early_print(device);
    early_print(" at ");
    early_print(mountpoint);
    early_print(" (");
    early_print(fstype);
    early_print(")\n");
    
    // Find filesystem type
    struct file_system_type *fs_type = vfs_find_filesystem_type(fstype);
    if (!fs_type) {
        early_print("Unknown filesystem type: ");
        early_print(fstype);
        early_print("\n");
        return VFS_ENOENT;
    }
    
    // Find block device
    struct block_device *block_dev = block_device_find(device);
    if (!block_dev) {
        early_print("Block device not found: ");
        early_print(device);
        early_print("\n");
        return VFS_ENOENT;
    }
    
    // Create file system instance
    struct file_system *fs = fs_type->mount(block_dev, flags);
    if (!fs) {
        early_print("Failed to mount filesystem\n");
        return VFS_ERROR;
    }
    
    // Find empty mount slot
    int mount_slot = -1;
    for (int i = 0; i < MAX_MOUNTS; i++) {
        if (mounts[i] == NULL) {
            mount_slot = i;
            break;
        }
    }
    
    if (mount_slot == -1) {
        early_print("Too many mounts\n");
        if (fs_type->unmount) {
            fs_type->unmount(fs);
        }
        return VFS_ENOMEM;
    }
    
    // Create mount point
    struct vfs_mount *mount = kmalloc(sizeof(struct vfs_mount));
    if (!mount) {
        early_print("Failed to allocate mount point\n");
        if (fs_type->unmount) {
            fs_type->unmount(fs);
        }
        return VFS_ENOMEM;
    }
    
    // Initialize mount point
    strncpy(mount->mountpoint, mountpoint, sizeof(mount->mountpoint) - 1);
    mount->mountpoint[sizeof(mount->mountpoint) - 1] = '\0';
    mount->fs = fs;
    mount->flags = flags;
    mount->next = NULL;
    
    // Link filesystem to mount point
    fs->mount_point = mount;
    
    // Add to mounts array
    mounts[mount_slot] = mount;
    vfs_stats.mounts_active++;
    
    // Set as root mount if mounting at "/"
    if (strcmp(mountpoint, "/") == 0) {
        root_mount = mount;
        early_print("Root filesystem mounted\n");
    }
    
    early_print("Mount successful\n");
    return VFS_SUCCESS;
}

int vfs_unmount(const char *mountpoint)
{
    if (!vfs_initialized || !mountpoint) {
        return VFS_EINVAL;
    }
    
    early_print("Unmounting ");
    early_print(mountpoint);
    early_print("\n");
    
    // Find mount point
    for (int i = 0; i < MAX_MOUNTS; i++) {
        if (mounts[i] && strcmp(mounts[i]->mountpoint, mountpoint) == 0) {
            struct vfs_mount *mount = mounts[i];
            struct file_system *fs = mount->fs;
            
            // Unmount filesystem
            if (fs->type->unmount) {
                fs->type->unmount(fs);
            }
            
            // Clear root mount if unmounting root
            if (mount == root_mount) {
                root_mount = NULL;
                early_print("Root filesystem unmounted\n");
            }
            
            // Remove from mounts array
            mounts[i] = NULL;
            if (vfs_stats.mounts_active > 0) {
                vfs_stats.mounts_active--;
            }
            
            // Free mount structure
            kfree(mount);
            
            early_print("Unmount successful\n");
            return VFS_SUCCESS;
        }
    }
    
    early_print("Mount point not found\n");
    return VFS_ENOENT;
}

struct file_system *vfs_get_filesystem(const char *path)
{
    if (!vfs_initialized || !path) {
        return NULL;
    }
    
    // For now, simple implementation - just return root filesystem
    // In a full implementation, this would walk the mount table
    // and find the longest matching mount point
    
    if (root_mount) {
        return root_mount->fs;
    }
    
    return NULL;
}

struct file_system *vfs_find_mount(const char *path)
{
    return vfs_get_filesystem(path);
}

char *vfs_resolve_path(const char *path)
{
    if (!path) {
        return NULL;
    }
    
    // Simple implementation: return a copy of the path
    // In a full implementation, this would resolve relative paths,
    // symbolic links, etc.
    
    size_t path_len = strlen(path);
    char *resolved = kmalloc(path_len + 1);
    if (resolved) {
        strcpy(resolved, path);
    }
    
    return resolved;
}

int vfs_is_absolute_path(const char *path)
{
    return path && path[0] == '/';
}

char *vfs_get_filename(const char *path)
{
    if (!path) {
        return NULL;
    }
    
    // Find last '/' character
    const char *filename = strrchr(path, '/');
    if (filename) {
        return (char *)(filename + 1);  // Skip the '/'
    }
    
    return (char *)path;  // No '/' found, entire path is filename
}

char *vfs_get_dirname(const char *path)
{
    if (!path) {
        return NULL;
    }
    
    size_t path_len = strlen(path);
    if (path_len == 0) {
        return NULL;
    }
    
    // Find last '/' character
    const char *last_slash = strrchr(path, '/');
    if (!last_slash) {
        // No '/' found, return current directory
        char *dirname = kmalloc(2);
        if (dirname) {
            strcpy(dirname, ".");
        }
        return dirname;
    }
    
    // Calculate directory length
    size_t dir_len = last_slash - path;
    if (dir_len == 0) {
        // Root directory
        char *dirname = kmalloc(2);
        if (dirname) {
            strcpy(dirname, "/");
        }
        return dirname;
    }
    
    // Allocate and copy directory part
    char *dirname = kmalloc(dir_len + 1);
    if (dirname) {
        strncpy(dirname, path, dir_len);
        dirname[dir_len] = '\0';
    }
    
    return dirname;
}

// VFS debugging and statistics
void vfs_dump_info(void)
{
    if (!vfs_initialized) {
        early_print("VFS not initialized\n");
        return;
    }
    
    early_print("VFS Information:\n");
    early_print("  Filesystem types: ");
    // Simple number to string conversion
    char num_str[16];
    int val = vfs_stats.fs_types_registered;
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
    
    early_print("  Active mounts: ");
    val = vfs_stats.mounts_active;
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
    
    // List registered filesystem types
    early_print("  Registered filesystems:\n");
    for (int i = 0; i < MAX_FS_TYPES; i++) {
        if (fs_types[i]) {
            early_print("    ");
            early_print(fs_types[i]->name);
            early_print("\n");
        }
    }
    
    // List active mounts
    early_print("  Mount points:\n");
    for (int i = 0; i < MAX_MOUNTS; i++) {
        if (mounts[i]) {
            early_print("    ");
            early_print(mounts[i]->mountpoint);
            early_print(" (");
            early_print(mounts[i]->fs->type->name);
            early_print(")\n");
        }
    }
}

// File operations implementations
int vfs_open(const char *path, int flags, int mode)
{
    if (!vfs_initialized || !path) {
        return -1;
    }
    
    (void)flags;  // Suppress unused parameter warning
    (void)mode;   // Suppress unused parameter warning
    
    // Get filesystem for path
    struct file_system *fs = vfs_get_filesystem(path);
    if (!fs) {
        return VFS_ENOENT;
    }
    
    // For now, return a dummy positive FD to indicate success
    // Real implementation would create file structure and return proper FD
    return 3;  // Return dummy FD (0, 1, 2 are stdin/stdout/stderr)
}

ssize_t vfs_read(int fd, void *buf, size_t count)
{
    (void)fd; (void)buf; (void)count;
    return 0;  // Placeholder: read 0 bytes
}

ssize_t vfs_write(int fd, const void *buf, size_t count)
{
    (void)fd; (void)buf; (void)count;
    return count;  // Placeholder: pretend all bytes written
}

int vfs_close(int fd)
{
    (void)fd;
    return VFS_SUCCESS;
}

off_t vfs_seek(int fd, off_t offset, int whence)
{
    (void)fd; (void)whence;
    return offset;
}

int vfs_sync(int fd)
{
    (void)fd;
    return VFS_SUCCESS;
}

int vfs_mkdir(const char *path, int mode)
{
    if (!vfs_initialized || !path) {
        return VFS_EINVAL;
    }
    
    // Get filesystem for path
    struct file_system *fs = vfs_get_filesystem(path);
    if (!fs) {
        return VFS_ENOENT;
    }
    
    // Call filesystem-specific mkdir
    if (fs->type->dir_ops && fs->type->dir_ops->mkdir) {
        return fs->type->dir_ops->mkdir(fs, path, mode);
    }
    
    return VFS_SUCCESS;
}

int vfs_rmdir(const char *path)
{
    if (!vfs_initialized || !path) {
        return VFS_EINVAL;
    }
    
    // Get filesystem for path
    struct file_system *fs = vfs_get_filesystem(path);
    if (!fs) {
        return VFS_ENOENT;
    }
    
    // Call filesystem-specific rmdir
    if (fs->type->dir_ops && fs->type->dir_ops->rmdir) {
        return fs->type->dir_ops->rmdir(fs, path);
    }
    
    return VFS_SUCCESS;
}

int vfs_readdir(int fd, struct dirent *entries, size_t count)
{
    (void)fd; (void)entries; (void)count;
    return 0;  // Placeholder: no directory entries
}

int vfs_unlink(const char *path)
{
    (void)path;
    return VFS_SUCCESS;
}

int vfs_rename(const char *oldpath, const char *newpath)
{
    (void)oldpath; (void)newpath;
    return VFS_SUCCESS;
}

int vfs_stat(const char *path, struct inode *stat_buf)
{
    if (!vfs_initialized || !path || !stat_buf) {
        return VFS_EINVAL;
    }
    
    // Get filesystem for path
    struct file_system *fs = vfs_get_filesystem(path);
    if (!fs) {
        return VFS_ENOENT;
    }
    
    // For now, create a dummy stat structure
    memset(stat_buf, 0, sizeof(struct inode));
    stat_buf->mode = VFS_FILE_DIRECTORY | 0755;  // Assume directory for now
    stat_buf->size = 0;
    
    return VFS_SUCCESS;
}

struct inode *vfs_lookup(const char *path)
{
    (void)path;
    return NULL;
}
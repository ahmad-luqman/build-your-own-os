/*
 * MiniOS Virtual File System Core
 * Unified interface for all file systems
 */

#include "vfs.h"
#include "block_device.h"
#include "memory.h"
#include "kernel.h"
#include "ramfs.h"

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

#define VFS_FD_BASE          3
#define VFS_MAX_OPEN_FILES   32

static struct file *vfs_open_files[VFS_MAX_OPEN_FILES];

static struct file *vfs_get_open_file(int fd)
{
    if (fd < VFS_FD_BASE || fd >= VFS_MAX_OPEN_FILES) {
        return NULL;
    }
    return vfs_open_files[fd];
}

static int vfs_allocate_fd(void)
{
    for (int fd = VFS_FD_BASE; fd < VFS_MAX_OPEN_FILES; fd++) {
        if (!vfs_open_files[fd]) {
            return fd;
        }
    }
    return -1;
}

static void vfs_release_fd(int fd)
{
    if (fd >= VFS_FD_BASE && fd < VFS_MAX_OPEN_FILES) {
        vfs_open_files[fd] = NULL;
    }
}

static struct ramfs_node *vfs_ramfs_resolve(struct file_system *fs, const char *path)
{
    if (!fs || !path || fs->type != &ramfs_fs_type) {
        return NULL;
    }

    struct ramfs_fs_data *fs_data = (struct ramfs_fs_data *)fs->private_data;
    if (!fs_data) {
        return NULL;
    }

    if (strcmp(path, "/") == 0) {
        return fs_data->root;
    }

    return ramfs_resolve_path(fs_data, path);
}

static struct inode *vfs_create_inode_from_ramfs(struct file_system *fs, struct ramfs_node *node)
{
    if (!fs || !node) {
        return NULL;
    }

    struct inode *inode = kmalloc(sizeof(struct inode));
    if (!inode) {
        return NULL;
    }

    memset(inode, 0, sizeof(struct inode));
    inode->ino = node->ino;
    inode->mode = node->mode;
    inode->size = node->size;
    inode->created_time = node->created_time;
    inode->modified_time = node->modified_time;
    inode->accessed_time = node->accessed_time;
    inode->fs = fs;
    inode->private_data = node;
    inode->ref_count = 1;
    return inode;
}

static void vfs_ramfs_truncate_node(struct ramfs_node *node)
{
    if (!node || (node->mode & VFS_FILE_DIRECTORY)) {
        return;
    }

    if (node->data) {
        kfree(node->data);
        node->data = NULL;
    }
    node->size = 0;
    node->modified_time++;
}

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

    for (int i = 0; i < VFS_MAX_OPEN_FILES; i++) {
        vfs_open_files[i] = NULL;
    }

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
    if (!vfs_initialized || !mountpoint || !fstype) {
        return VFS_EINVAL;
    }
    
    early_print("Mounting ");
    early_print(device ? device : "none");
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
    
    // Find block device (optional for virtual filesystems like ramfs)
    struct block_device *block_dev = NULL;
    if (device && strcmp(device, "none") != 0) {
        block_dev = block_device_find(device);
        if (!block_dev) {
            early_print("Block device not found: ");
            early_print(device);
            early_print("\n");
            return VFS_ENOENT;
        }
    } else {
        early_print("Mounting virtual filesystem (no block device)\n");
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
    memset(mount, 0, sizeof(struct vfs_mount));  // Use memset for safety
    
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
        return VFS_EINVAL;
    }

    struct file_system *fs = vfs_get_filesystem(path);
    if (!fs) {
        return VFS_ENOENT;
    }

    if (fs->type != &ramfs_fs_type) {
        return VFS_EINVAL;
    }

    struct ramfs_node *node = vfs_ramfs_resolve(fs, path);

    if (!node) {
        if (!(flags & VFS_O_CREAT)) {
            return VFS_ENOENT;
        }

        if (ramfs_create_file(fs, path, mode ? (uint32_t)mode : 0644) != VFS_SUCCESS) {
            return VFS_ERROR;
        }

        node = vfs_ramfs_resolve(fs, path);
        if (!node) {
            return VFS_ENOENT;
        }
    }

    if ((node->mode & VFS_FILE_DIRECTORY) && (flags & (VFS_O_WRONLY | VFS_O_RDWR))) {
        return VFS_EINVAL;
    }

    if ((flags & VFS_O_TRUNC) && !(node->mode & VFS_FILE_DIRECTORY)) {
        vfs_ramfs_truncate_node(node);
    }

    struct inode *inode = vfs_create_inode_from_ramfs(fs, node);
    if (!inode) {
        return VFS_ENOMEM;
    }

    struct file *file = kmalloc(sizeof(struct file));
    if (!file) {
        kfree(inode);
        return VFS_ENOMEM;
    }

    memset(file, 0, sizeof(struct file));
    file->inode = inode;
    file->fs = fs;
    file->position = (flags & VFS_O_APPEND) ? inode->size : 0;
    file->flags = flags;
    file->mode = mode;
    file->ref_count = 1;
    file->ops = fs->type->file_ops;

    if (file->ops && file->ops->open) {
        int open_result = file->ops->open(file, flags, mode);
        if (open_result != VFS_SUCCESS) {
            kfree(inode);
            kfree(file);
            return open_result;
        }
    }

    int fd = vfs_allocate_fd();
    if (fd < 0) {
        if (file->ops && file->ops->close) {
            file->ops->close(file);
        }
        kfree(inode);
        kfree(file);
        return VFS_ENOSPC;
    }

    vfs_open_files[fd] = file;
    vfs_stats.files_opened++;
    return fd;
}

ssize_t vfs_read(int fd, void *buf, size_t count)
{
    struct file *file = vfs_get_open_file(fd);
    if (!file || !buf || count == 0) {
        return VFS_EINVAL;
    }

    if (!file->ops || !file->ops->read) {
        return VFS_EINVAL;
    }

    ssize_t result = file->ops->read(file, buf, count, file->position);
    if (result > 0) {
        file->position += result;
    }
    return result;
}

ssize_t vfs_write(int fd, const void *buf, size_t count)
{
    struct file *file = vfs_get_open_file(fd);
    if (!file || !buf || count == 0) {
        return VFS_EINVAL;
    }

    if (!file->ops || !file->ops->write) {
        return VFS_EINVAL;
    }

    ssize_t result = file->ops->write(file, buf, count, file->position);
    if (result > 0) {
        file->position += result;
    }
    return result;
}

int vfs_close(int fd)
{
    struct file *file = vfs_get_open_file(fd);
    if (!file) {
        return VFS_EINVAL;
    }

    if (file->ops && file->ops->close) {
        file->ops->close(file);
    }

    if (file->inode) {
        kfree(file->inode);
    }

    kfree(file);
    vfs_release_fd(fd);
    vfs_stats.files_closed++;
    return VFS_SUCCESS;
}

off_t vfs_seek(int fd, off_t offset, int whence)
{
    struct file *file = vfs_get_open_file(fd);
    if (!file) {
        return VFS_EINVAL;
    }

    if (file->ops && file->ops->seek) {
        return file->ops->seek(file, offset, whence);
    }

    switch (whence) {
        case VFS_SEEK_SET:
            file->position = offset;
            break;
        case VFS_SEEK_CUR:
            file->position += offset;
            break;
        case VFS_SEEK_END:
            file->position = file->inode ? (off_t)file->inode->size + offset : offset;
            break;
        default:
            return VFS_EINVAL;
    }

    if (file->position < 0) {
        file->position = 0;
    }

    return file->position;
}

int vfs_sync(int fd)
{
    struct file *file = vfs_get_open_file(fd);
    if (!file) {
        return VFS_EINVAL;
    }

    if (file->ops && file->ops->sync) {
        return file->ops->sync(file);
    }

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
    struct file *dir = vfs_get_open_file(fd);
    if (!dir || !entries || count == 0) {
        return VFS_EINVAL;
    }

    if (!dir->fs || dir->fs->type != &ramfs_fs_type) {
        return VFS_EINVAL;
    }

    if (!dir->fs->type->dir_ops || !dir->fs->type->dir_ops->readdir) {
        return VFS_EINVAL;
    }

    if (!dir->inode || !(dir->inode->mode & VFS_FILE_DIRECTORY)) {
        return VFS_EINVAL;
    }

    off_t offset = dir->position;
    int result = dir->fs->type->dir_ops->readdir(dir, entries, count * sizeof(struct dirent), &offset);
    if (result >= 0) {
        dir->position = offset;
    }
    return result;
}

int vfs_unlink(const char *path)
{
    if (!vfs_initialized || !path) {
        return VFS_EINVAL;
    }

    struct file_system *fs = vfs_get_filesystem(path);
    if (!fs || fs->type != &ramfs_fs_type) {
        return VFS_EINVAL;
    }

    struct ramfs_node *node = vfs_ramfs_resolve(fs, path);
    if (!node) {
        return VFS_ENOENT;
    }

    if (node->mode & VFS_FILE_DIRECTORY) {
        return VFS_EINVAL;
    }

    struct ramfs_node *parent = node->parent;
    if (!parent) {
        return VFS_EPERM;
    }

    return ramfs_remove_child(parent, node->name);
}

int vfs_rename(const char *oldpath, const char *newpath)
{
    if (!vfs_initialized || !oldpath || !newpath) {
        return VFS_EINVAL;
    }

    struct file_system *old_fs = vfs_get_filesystem(oldpath);
    struct file_system *new_fs = vfs_get_filesystem(newpath);

    if (!old_fs || old_fs != new_fs || old_fs->type != &ramfs_fs_type) {
        return VFS_EINVAL;
    }

    struct ramfs_node *node = vfs_ramfs_resolve(old_fs, oldpath);
    if (!node) {
        return VFS_ENOENT;
    }

    if (node->mode & VFS_FILE_DIRECTORY) {
        return VFS_EINVAL;
    }

    int src_fd = vfs_open(oldpath, VFS_O_RDONLY, 0);
    if (src_fd < 0) {
        return src_fd;
    }

    int dst_fd = vfs_open(newpath, VFS_O_WRONLY | VFS_O_CREAT | VFS_O_TRUNC, 0644);
    if (dst_fd < 0) {
        vfs_close(src_fd);
        return dst_fd;
    }

    char buffer[512];
    ssize_t bytes_read;
    int result = VFS_SUCCESS;

    while ((bytes_read = vfs_read(src_fd, buffer, sizeof(buffer))) > 0) {
        ssize_t bytes_written = vfs_write(dst_fd, buffer, (size_t)bytes_read);
        if (bytes_written != bytes_read) {
            result = VFS_ERROR;
            break;
        }
    }

    vfs_close(src_fd);
    vfs_close(dst_fd);

    if (result == VFS_SUCCESS) {
        return vfs_unlink(oldpath);
    }

    vfs_unlink(newpath);
    return result;
}

int vfs_stat(const char *path, struct inode *stat_buf)
{
    if (!vfs_initialized || !path || !stat_buf) {
        return VFS_EINVAL;
    }

    struct file_system *fs = vfs_get_filesystem(path);
    if (!fs || fs->type != &ramfs_fs_type) {
        return VFS_EINVAL;
    }

    struct ramfs_node *node = vfs_ramfs_resolve(fs, path);
    if (!node) {
        return VFS_ENOENT;
    }

    memset(stat_buf, 0, sizeof(struct inode));
    stat_buf->ino = node->ino;
    stat_buf->mode = node->mode;
    stat_buf->size = node->size;
    stat_buf->created_time = node->created_time;
    stat_buf->modified_time = node->modified_time;
    stat_buf->accessed_time = node->accessed_time;
    stat_buf->fs = fs;
    stat_buf->private_data = node;
    stat_buf->ref_count = 1;

    return VFS_SUCCESS;
}

struct inode *vfs_lookup(const char *path)
{
    if (!vfs_initialized || !path) {
        return NULL;
    }

    struct file_system *fs = vfs_get_filesystem(path);
    if (!fs || fs->type != &ramfs_fs_type) {
        return NULL;
    }

    struct ramfs_node *node = vfs_ramfs_resolve(fs, path);
    if (!node) {
        return NULL;
    }

    return vfs_create_inode_from_ramfs(fs, node);
}

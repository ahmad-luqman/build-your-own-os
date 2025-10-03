/*
 * MiniOS RAM File System (RAMFS) Core
 * In-memory file system implementation
 */

#include "ramfs.h"
#include "memory.h"
#include "kernel.h"
#include "fd.h"

// Forward declarations for operations
static int ramfs_file_open(struct file *file, int flags, int mode);
static int ramfs_file_close(struct file *file);
static ssize_t ramfs_file_read(struct file *file, void *buf, size_t count, off_t offset);
static ssize_t ramfs_file_write(struct file *file, const void *buf, size_t count, off_t offset);
static off_t ramfs_file_seek(struct file *file, off_t offset, int whence);
static int ramfs_file_sync(struct file *file);

static int ramfs_dir_readdir(struct file *dir, void *buffer, size_t buffer_size, off_t *offset);
static int ramfs_dir_mkdir(struct file_system *fs, const char *path, int mode);
static int ramfs_dir_rmdir(struct file_system *fs, const char *path);
static struct inode *ramfs_dir_lookup(struct file_system *fs, struct inode *parent, const char *name);

// RAMFS operations structures
static struct file_operations ramfs_file_ops = {
    .open = ramfs_file_open,
    .close = ramfs_file_close,
    .read = ramfs_file_read,
    .write = ramfs_file_write,
    .seek = ramfs_file_seek,
    .sync = ramfs_file_sync,
    .ioctl = NULL
};

static struct directory_operations ramfs_dir_ops = {
    .readdir = ramfs_dir_readdir,
    .mkdir = ramfs_dir_mkdir,
    .rmdir = ramfs_dir_rmdir,
    .lookup = ramfs_dir_lookup
};

// RAMFS file system type
struct file_system_type ramfs_fs_type = {
    .name = "ramfs",
    .mount = ramfs_mount,
    .unmount = ramfs_unmount,
    .format = ramfs_format,
    .file_ops = &ramfs_file_ops,
    .dir_ops = &ramfs_dir_ops
};

int ramfs_init(void)
{
    early_print("Initializing RAM File System (RAMFS)...\n");
    
    // Register RAMFS with VFS
    if (vfs_register_filesystem(&ramfs_fs_type) != VFS_SUCCESS) {
        early_print("Failed to register RAMFS filesystem\n");
        return VFS_ERROR;
    }
    
    early_print("RAMFS initialized\n");
    return VFS_SUCCESS;
}

struct ramfs_node *ramfs_create_node(struct ramfs_fs_data *fs_data, const char *name, uint32_t mode)
{
    if (!fs_data || !name) {
        return NULL;
    }
    
    struct ramfs_node *node = kmalloc(sizeof(struct ramfs_node));
    if (!node) {
        return NULL;
    }
    
    memset(node, 0, sizeof(struct ramfs_node));
    strncpy(node->name, name, RAMFS_MAX_NAME);
    node->name[RAMFS_MAX_NAME] = '\0';
    node->mode = mode;
    node->size = 0;
    node->ino = fs_data->next_ino++;
    node->data = NULL;
    node->parent = NULL;
    node->children = NULL;
    node->next = NULL;
    node->created_time = 0;
    node->modified_time = 0;
    node->accessed_time = 0;
    
    fs_data->file_count++;
    
    return node;
}

void ramfs_destroy_node(struct ramfs_node *node)
{
    if (!node) {
        return;
    }
    
    // Free file data if any
    if (node->data) {
        kfree(node->data);
        node->data = NULL;
    }
    
    // Recursively destroy children
    struct ramfs_node *child = node->children;
    while (child) {
        struct ramfs_node *next = child->next;
        ramfs_destroy_node(child);
        child = next;
    }
    
    kfree(node);
}

struct ramfs_node *ramfs_find_node(struct ramfs_node *parent, const char *name)
{
    if (!parent || !name) {
        return NULL;
    }
    
    // Check if this is a directory
    if ((parent->mode & VFS_FILE_DIRECTORY) == 0) {
        return NULL;
    }
    
    // Search children
    struct ramfs_node *child = parent->children;
    while (child) {
        if (strcmp(child->name, name) == 0) {
            return child;
        }
        child = child->next;
    }
    
    return NULL;
}

struct ramfs_node *ramfs_resolve_path(struct ramfs_fs_data *fs_data, const char *path)
{
    if (!fs_data || !path) {
        return NULL;
    }

    // Handle root directory
    if (strcmp(path, "/") == 0) {
        return fs_data->root;
    }
    
    // Skip leading slash
    if (path[0] == '/') {
        path++;
    }
    
    // Start from root
    struct ramfs_node *current = fs_data->root;
    
    // Parse path components
    char component[RAMFS_MAX_NAME + 1];
    int comp_idx = 0;
    
    while (*path) {
        if (*path == '/') {
            // End of component
            if (comp_idx > 0) {
                component[comp_idx] = '\0';
                current = ramfs_find_node(current, component);
                if (!current) {
                    return NULL;
                }
                comp_idx = 0;
            }
            path++;
        } else {
            // Add character to component
            if (comp_idx < RAMFS_MAX_NAME) {
                component[comp_idx++] = *path;
            }
            path++;
        }
    }
    
    // Handle final component
    if (comp_idx > 0) {
        component[comp_idx] = '\0';
        current = ramfs_find_node(current, component);
    }

    return current;
}

int ramfs_add_child(struct ramfs_node *parent, struct ramfs_node *child)
{
    if (!parent || !child) {
        return VFS_EINVAL;
    }
    
    // Check if parent is a directory
    if ((parent->mode & VFS_FILE_DIRECTORY) == 0) {
        return VFS_EINVAL;
    }
    
    // Set parent
    child->parent = parent;
    
    // Add to children list
    child->next = parent->children;
    parent->children = child;
    
    return VFS_SUCCESS;
}

int ramfs_remove_child(struct ramfs_node *parent, const char *name)
{
    if (!parent || !name) {
        return VFS_EINVAL;
    }
    
    struct ramfs_node *prev = NULL;
    struct ramfs_node *child = parent->children;
    
    while (child) {
        if (strcmp(child->name, name) == 0) {
            // Remove from list
            if (prev) {
                prev->next = child->next;
            } else {
                parent->children = child->next;
            }
            
            // Destroy node
            ramfs_destroy_node(child);
            return VFS_SUCCESS;
        }
        
        prev = child;
        child = child->next;
    }
    
    return VFS_ENOENT;
}

int ramfs_format(struct block_device *dev)
{
    (void)dev;  // RAMFS doesn't need a block device
    early_print("RAMFS format: No-op (RAM filesystem doesn't need formatting)\n");
    return VFS_SUCCESS;
}

struct file_system *ramfs_mount(struct block_device *dev, unsigned long flags)
{
    (void)flags;
    (void)dev;  // RAMFS doesn't need a block device
    
    early_print("RAMFS: Starting mount (using kmalloc)...\n");
    
    // Allocate structures using kmalloc (which works!)
    struct file_system *fs = kmalloc(sizeof(struct file_system));
    if (!fs) {
        early_print("RAMFS: Failed to allocate filesystem\n");
        return NULL;
    }
    memset(fs, 0, sizeof(struct file_system));
    early_print("RAMFS: Filesystem allocated\n");
    
    struct ramfs_fs_data *fs_data = kmalloc(sizeof(struct ramfs_fs_data));
    if (!fs_data) {
        early_print("RAMFS: Failed to allocate fs_data\n");
        kfree(fs);
        return NULL;
    }
    memset(fs_data, 0, sizeof(struct ramfs_fs_data));
    early_print("RAMFS: FS data allocated\n");
    
    struct ramfs_node *root = kmalloc(sizeof(struct ramfs_node));
    if (!root) {
        early_print("RAMFS: Failed to allocate root\n");
        kfree(fs_data);
        kfree(fs);
        return NULL;
    }
    memset(root, 0, sizeof(struct ramfs_node));
    early_print("RAMFS: Root node allocated\n");
    
    early_print("RAMFS: Setting up root node...\n");
    // Setup root node - do it very carefully
    char *name_ptr = root->name;
    name_ptr[0] = '/';
    name_ptr[1] = '\0';
    early_print("RAMFS: Root name set\n");
    
    root->mode = VFS_FILE_DIRECTORY | 0755;
    root->size = 0;
    root->ino = 1;
    root->data = NULL;
    root->parent = NULL;
    root->children = NULL;
    root->next = NULL;
    early_print("RAMFS: Root node setup complete\n");
    
    early_print("RAMFS: Setting up filesystem data...\n");
    // Setup filesystem data
    fs_data->root = root;
    fs_data->next_ino = 2;
    fs_data->file_count = 1;
    fs_data->used_memory = 0;
    
    early_print("RAMFS: Setting up filesystem structure...\n");
    // Setup filesystem structure
    fs->type = &ramfs_fs_type;
    fs->device = dev;
    fs->private_data = fs_data;
    fs->mount_point = NULL;
    fs->flags = 0;
    
    early_print("RAMFS: Mount successful!\n");
    return fs;
}

void ramfs_unmount(struct file_system *fs)
{
    if (!fs) {
        return;
    }
    
    early_print("Unmounting RAMFS...\n");
    
    struct ramfs_fs_data *fs_data = (struct ramfs_fs_data *)fs->private_data;
    if (fs_data) {
        // Destroy all nodes starting from root
        if (fs_data->root) {
            ramfs_destroy_node(fs_data->root);
        }
        kfree(fs_data);
    }
    
    kfree(fs);
    early_print("RAMFS unmounted\n");
}

int ramfs_create_directory(struct file_system *fs, const char *path, uint32_t mode)
{
    if (!fs || !path) {
        return VFS_EINVAL;
    }

    struct ramfs_fs_data *fs_data = (struct ramfs_fs_data *)fs->private_data;
    if (!fs_data) {
        return VFS_ERROR;
    }

    // Find parent directory
    char *parent_path = vfs_get_dirname(path);
    if (!parent_path) {
        return VFS_ENOMEM;
    }

    struct ramfs_node *parent = ramfs_resolve_path(fs_data, parent_path);
    kfree(parent_path);

    if (!parent) {
        return VFS_ENOENT;
    }

    // Get directory name
    char *dir_name = vfs_get_filename(path);
    if (!dir_name) {
        return VFS_EINVAL;
    }

    // Check if already exists
    if (ramfs_find_node(parent, dir_name)) {
        return VFS_EEXIST;
    }

    // Create new directory node
    struct ramfs_node *dir_node = ramfs_create_node(fs_data, dir_name, VFS_FILE_DIRECTORY | mode);
    if (!dir_node) {
        return VFS_ENOMEM;
    }

    // Add to parent
    int result = ramfs_add_child(parent, dir_node);
    if (result != VFS_SUCCESS) {
        ramfs_destroy_node(dir_node);
        return result;
    }

    return VFS_SUCCESS;
}

int ramfs_create_file(struct file_system *fs, const char *path, uint32_t mode)
{
    if (!fs || !path) {
        return VFS_EINVAL;
    }

    struct ramfs_fs_data *fs_data = (struct ramfs_fs_data *)fs->private_data;
    if (!fs_data) {
        return VFS_ERROR;
    }

    char temp_path[VFS_MAX_PATH];
    strncpy(temp_path, path, sizeof(temp_path));
    temp_path[sizeof(temp_path) - 1] = '\0';

    struct ramfs_node *parent = NULL;
    const char *file_name = NULL;

    char *last_slash = strrchr(temp_path, '/');
    if (!last_slash) {
        parent = fs_data->root;
        file_name = temp_path;
    } else {
        if (last_slash == temp_path) {
            parent = fs_data->root;
        } else {
            *last_slash = '\0';
            parent = ramfs_resolve_path(fs_data, temp_path);
        }
        file_name = last_slash + 1;
    }

    if (!parent || !file_name || file_name[0] == '\0') {
        return VFS_EINVAL;
    }

    if (ramfs_find_node(parent, file_name)) {
        return VFS_EEXIST;
    }

    struct ramfs_node *file_node = ramfs_create_node(fs_data, file_name, VFS_FILE_REGULAR | mode);
    if (!file_node) {
        return VFS_ENOMEM;
    }

    int result = ramfs_add_child(parent, file_node);
    if (result != VFS_SUCCESS) {
        ramfs_destroy_node(file_node);
        return result;
    }

    return VFS_SUCCESS;
}

// File operations implementations
static int ramfs_file_open(struct file *file, int flags, int mode)
{
    (void)flags; (void)mode;
    
    if (!file || !file->inode) {
        return VFS_EINVAL;
    }
    
    // Update access time
    struct ramfs_node *node = (struct ramfs_node *)file->inode->private_data;
    if (node) {
        node->accessed_time++;
    }
    
    return VFS_SUCCESS;
}

static int ramfs_file_close(struct file *file)
{
    (void)file;
    return VFS_SUCCESS;
}

static ssize_t ramfs_file_read(struct file *file, void *buf, size_t count, off_t offset)
{
    if (!file || !file->inode || !buf) {
        return -1;
    }
    
    struct ramfs_node *node = (struct ramfs_node *)file->inode->private_data;
    if (!node || !node->data) {
        return 0;  // Empty file
    }
    
    // Check bounds
    if (offset >= (off_t)node->size) {
        return 0;  // EOF
    }
    
    // Calculate actual bytes to read
    size_t bytes_to_read = count;
    if (offset + (off_t)bytes_to_read > (off_t)node->size) {
        bytes_to_read = node->size - offset;
    }
    
    // Copy data
    memcpy(buf, (char *)node->data + offset, bytes_to_read);
    
    // Update access time
    node->accessed_time++;
    
    return bytes_to_read;
}

static ssize_t ramfs_file_write(struct file *file, const void *buf, size_t count, off_t offset)
{
    if (!file || !file->inode || !buf) {
        return -1;
    }
    
    struct ramfs_node *node = (struct ramfs_node *)file->inode->private_data;
    if (!node) {
        return -1;
    }
    
    // Calculate new size needed
    size_t new_size = offset + count;
    if (new_size > RAMFS_MAX_FILE_SIZE) {
        return -1;  // File too large
    }
    
    // Allocate or reallocate data buffer
    if (!node->data) {
        node->data = kmalloc(new_size);
        if (!node->data) {
            return -1;
        }
        memset(node->data, 0, new_size);
    } else if (new_size > node->size) {
        void *new_data = kmalloc(new_size);
        if (!new_data) {
            return -1;
        }
        memcpy(new_data, node->data, node->size);
        memset((char *)new_data + node->size, 0, new_size - node->size);
        kfree(node->data);
        node->data = new_data;
    }
    
    // Write data
    memcpy((char *)node->data + offset, buf, count);
    
    // Update size if needed
    if (new_size > node->size) {
        node->size = new_size;
        file->inode->size = new_size;
    }
    
    // Update modification time
    node->modified_time++;
    
    return count;
}

static off_t ramfs_file_seek(struct file *file, off_t offset, int whence)
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

static int ramfs_file_sync(struct file *file)
{
    (void)file;
    return VFS_SUCCESS;  // Nothing to sync in RAM
}

// Directory operations implementations
static int ramfs_dir_readdir(struct file *dir, void *buffer, size_t buffer_size, off_t *offset)
{
    if (!dir || !dir->inode || !buffer || !offset) {
        return -1;
    }
    
    struct ramfs_node *dir_node = (struct ramfs_node *)dir->inode->private_data;
    if (!dir_node) {
        return -1;
    }
    
    // Check if it's a directory
    if ((dir_node->mode & VFS_FILE_DIRECTORY) == 0) {
        return -1;
    }
    
    struct dirent *entries = (struct dirent *)buffer;
    int count = 0;
    int max_entries = buffer_size / sizeof(struct dirent);
    
    // Skip to offset
    struct ramfs_node *child = dir_node->children;
    int current_offset = 0;
    
    while (child && current_offset < *offset) {
        child = child->next;
        current_offset++;
    }
    
    // Fill entries
    while (child && count < max_entries) {
        entries[count].ino = child->ino;
        entries[count].type = child->mode & 0xF000;
        entries[count].name_len = strlen(child->name);
        strncpy(entries[count].name, child->name, VFS_MAX_NAME);
        entries[count].name[VFS_MAX_NAME - 1] = '\0';
        
        count++;
        child = child->next;
        (*offset)++;
    }
    
    return count;
}

static int ramfs_dir_mkdir(struct file_system *fs, const char *path, int mode)
{
    return ramfs_create_directory(fs, path, mode);
}

static int ramfs_dir_rmdir(struct file_system *fs, const char *path)
{
    if (!fs || !path) {
        return VFS_EINVAL;
    }
    
    struct ramfs_fs_data *fs_data = (struct ramfs_fs_data *)fs->private_data;
    if (!fs_data) {
        return VFS_ERROR;
    }
    
    // Find the directory node
    struct ramfs_node *node = ramfs_resolve_path(fs_data, path);
    if (!node) {
        return VFS_ENOENT;
    }
    
    // Check if it's a directory
    if ((node->mode & VFS_FILE_DIRECTORY) == 0) {
        return VFS_EINVAL;
    }
    
    // Check if directory is empty
    if (node->children != NULL) {
        return VFS_EINVAL;  // Directory not empty
    }
    
    // Remove from parent
    if (node->parent) {
        return ramfs_remove_child(node->parent, node->name);
    }
    
    return VFS_EPERM;  // Can't delete root
}

static struct inode *ramfs_dir_lookup(struct file_system *fs, struct inode *parent, const char *name)
{
    if (!fs || !parent || !name) {
        return NULL;
    }
    
    struct ramfs_node *parent_node = (struct ramfs_node *)parent->private_data;
    if (!parent_node) {
        return NULL;
    }
    
    struct ramfs_node *child = ramfs_find_node(parent_node, name);
    if (!child) {
        return NULL;
    }
    
    // Create VFS inode
    struct inode *inode = kmalloc(sizeof(struct inode));
    if (!inode) {
        return NULL;
    }
    
    inode->ino = child->ino;
    inode->mode = child->mode;
    inode->size = child->size;
    inode->blocks = 0;
    inode->created_time = child->created_time;
    inode->modified_time = child->modified_time;
    inode->accessed_time = child->accessed_time;
    inode->fs = fs;
    inode->private_data = child;
    inode->ref_count = 1;
    
    return inode;
}

int ramfs_populate_initial_files(struct file_system *fs)
{
    if (!fs) {
        return VFS_EINVAL;
    }
    
    early_print("Populating RAMFS with initial files...\n");
    
    // Create some directories
    ramfs_create_directory(fs, "/bin", 0755);
    ramfs_create_directory(fs, "/etc", 0755);
    ramfs_create_directory(fs, "/tmp", 0777);
    ramfs_create_directory(fs, "/home", 0755);
    ramfs_create_directory(fs, "/dev", 0755);
    
    // Create a welcome file
    ramfs_create_file(fs, "/welcome.txt", 0644);
    
    // Write content to welcome file
    struct ramfs_fs_data *fs_data = (struct ramfs_fs_data *)fs->private_data;
    struct ramfs_node *welcome = ramfs_resolve_path(fs_data, "/welcome.txt");
    if (welcome) {
        const char *content = "Welcome to MiniOS!\n\nThis is a fully functional RAM disk file system.\nTry these commands:\n  ls\n  cat welcome.txt\n  mkdir test\n  cd test\n  pwd\n\nEnjoy exploring!\n";
        size_t len = strlen(content);
        welcome->data = kmalloc(len + 1);
        if (welcome->data) {
            memcpy(welcome->data, content, len);
            ((char *)welcome->data)[len] = '\0';
            welcome->size = len;
        }
    }

    early_print("Initial file structure created\n");
    return VFS_SUCCESS;
}

void ramfs_dump_tree(struct ramfs_node *node, int depth)
{
    if (!node) {
        return;
    }
    
    // Print indentation
    for (int i = 0; i < depth; i++) {
        early_print("  ");
    }
    
    // Print node name
    early_print(node->name);
    if (node->mode & VFS_FILE_DIRECTORY) {
        early_print("/");
    }
    early_print("\n");
    
    // Print children
    struct ramfs_node *child = node->children;
    while (child) {
        ramfs_dump_tree(child, depth + 1);
        child = child->next;
    }
}

void ramfs_dump_filesystem_info(struct file_system *fs)
{
    if (!fs) {
        early_print("RAMFS: NULL filesystem\n");
        return;
    }
    
    struct ramfs_fs_data *fs_data = (struct ramfs_fs_data *)fs->private_data;
    if (!fs_data) {
        early_print("RAMFS: NULL filesystem data\n");
        return;
    }
    
    early_print("RAMFS Filesystem Information:\n");
    early_print("  Total files/directories: ");
    char num_str[16];
    int val = fs_data->file_count;
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
    
    early_print("  Directory structure:\n");
    ramfs_dump_tree(fs_data->root, 1);
}

# MiniOS File System Architecture Guide

## Overview

This document explains the architecture of MiniOS file systems and how to build new features on top of them.

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [VFS Layer (Virtual File System)](#vfs-layer)
3. [File System Implementations](#file-system-implementations)
4. [Building New Features](#building-new-features)
5. [Adding a New File System](#adding-a-new-file-system)
6. [Examples and Use Cases](#examples-and-use-cases)

---

## Architecture Overview

### Layered Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         User Space                              │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐      │
│  │  Shell   │  │   Apps   │  │  Tools   │  │ Scripts  │      │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘      │
└───────┼─────────────┼─────────────┼─────────────┼─────────────┘
        │             │             │             │
┌───────┼─────────────┼─────────────┼─────────────┼─────────────┐
│       │             │             │             │              │
│  ┌────▼─────────────▼─────────────▼─────────────▼───────────┐ │
│  │          System Call Interface (syscall.c)              │ │
│  │  open(), read(), write(), close(), seek(), etc.         │ │
│  └────┬────────────────────────────────────────────────────┘ │
│       │                                                        │
│  ┌────▼────────────────────────────────────────────────────┐ │
│  │      File Descriptor Layer (fd_table.c)                │ │
│  │  - File descriptor management                          │ │
│  │  - stdin(0), stdout(1), stderr(2), user files(3+)     │ │
│  │  - Reference counting                                  │ │
│  └────┬────────────────────────────────────────────────────┘ │
│       │                    Kernel Space                       │
└───────┼────────────────────────────────────────────────────────┘
        │
┌───────▼────────────────────────────────────────────────────────┐
│  ┌──────────────────────────────────────────────────────────┐ │
│  │        Virtual File System (VFS) Core (vfs_core.c)      │ │
│  │  ┌──────────────────────────────────────────────────┐   │ │
│  │  │ File System Registry                             │   │ │
│  │  │  - Registered file systems (ramfs, sfs, etc.)   │   │ │
│  │  └──────────────────────────────────────────────────┘   │ │
│  │  ┌──────────────────────────────────────────────────┐   │ │
│  │  │ Mount Table                                      │   │ │
│  │  │  - Active mounts (/, /mnt, /dev, etc.)          │   │ │
│  │  └──────────────────────────────────────────────────┘   │ │
│  │  ┌──────────────────────────────────────────────────┐   │ │
│  │  │ Path Resolution                                  │   │ │
│  │  │  - Parse paths, follow mount points             │   │ │
│  │  └──────────────────────────────────────────────────┘   │ │
│  │  ┌──────────────────────────────────────────────────┐   │ │
│  │  │ Operation Dispatch                               │   │ │
│  │  │  - Route calls to specific file system          │   │ │
│  │  └──────────────────────────────────────────────────┘   │ │
│  └──────────────────────────────────────────────────────────┘ │
└────────────┬──────────────┬──────────────┬────────────────────┘
             │              │              │
    ┌────────▼───┐    ┌────▼────┐    ┌───▼────────┐
    │   RAMFS    │    │   SFS   │    │  Future FS │
    │(ramfs_core)│    │(sfs_core)│   │  (ext2,    │
    │            │    │         │    │   fat32)   │
    └─────┬──────┘    └────┬────┘    └────────────┘
          │                │
          │           ┌────▼──────────────────────┐
          │           │  Block Device Layer       │
          │           │  (block_device.c)         │
          │           │  - ramdisk, HDD, SSD      │
          │           └───────────────────────────┘
          │
          └─► Direct Memory (kmalloc/kfree)
```

### Key Components

1. **System Call Interface**: User space → Kernel space bridge
2. **File Descriptor Layer**: Manages open files per process
3. **VFS Core**: Abstraction layer for all file systems
4. **File System Implementations**: RAMFS, SFS, etc.
5. **Block Device Layer**: Physical storage abstraction

---

## VFS Layer

### Purpose

The VFS (Virtual File System) provides a **unified interface** so that:
- Applications don't need to know which file system is used
- Multiple file systems can coexist
- New file systems can be added without changing applications

### Core Data Structures

#### 1. File System Type (`struct file_system_type`)

Defines a file system implementation:

```c
struct file_system_type {
    const char *name;                    // "ramfs", "sfs", "ext2"
    
    // Lifecycle operations
    struct file_system *(*mount)(struct block_device *dev, unsigned long flags);
    void (*unmount)(struct file_system *fs);
    int (*format)(struct block_device *dev);
    
    // Operation tables
    struct file_operations *file_ops;           // File operations
    struct directory_operations *dir_ops;       // Directory operations
};
```

#### 2. File System Instance (`struct file_system`)

Represents a mounted file system:

```c
struct file_system {
    struct file_system_type *type;       // Type of file system
    struct block_device *device;         // Underlying device (if any)
    void *private_data;                  // FS-specific data
    struct vfs_mount *mount_point;       // Where it's mounted
    int flags;                           // Mount flags
};
```

#### 3. File Operations (`struct file_operations`)

Operations on open files:

```c
struct file_operations {
    int (*open)(struct file *file, int flags, int mode);
    int (*close)(struct file *file);
    ssize_t (*read)(struct file *file, void *buf, size_t count, off_t offset);
    ssize_t (*write)(struct file *file, const void *buf, size_t count, off_t offset);
    int (*ioctl)(struct file *file, unsigned int cmd, unsigned long arg);
    off_t (*seek)(struct file *file, off_t offset, int whence);
    int (*sync)(struct file *file);
};
```

#### 4. Directory Operations (`struct directory_operations`)

Operations on directories:

```c
struct directory_operations {
    int (*readdir)(struct file *dir, void *buffer, size_t buffer_size, off_t *offset);
    int (*mkdir)(struct file_system *fs, const char *path, int mode);
    int (*rmdir)(struct file_system *fs, const char *path);
    struct inode *(*lookup)(struct file_system *fs, struct inode *parent, const char *name);
};
```

#### 5. Inode (`struct inode`)

Represents file metadata:

```c
struct inode {
    uint32_t ino;                  // Inode number (unique ID)
    uint32_t mode;                 // File type and permissions
    uint32_t size;                 // File size in bytes
    uint32_t blocks;               // Number of blocks allocated
    uint32_t created_time;         // Creation timestamp
    uint32_t modified_time;        // Last modification timestamp
    uint32_t accessed_time;        // Last access timestamp
    struct file_system *fs;        // Owning file system
    void *private_data;            // FS-specific data
    int ref_count;                 // Reference count
};
```

#### 6. File (`struct file`)

Represents an open file:

```c
struct file {
    struct inode *inode;           // File metadata
    struct file_system *fs;        // File system
    off_t position;                // Current read/write position
    int flags;                     // Open flags (O_RDONLY, O_WRONLY, etc.)
    int mode;                      // File mode
    int ref_count;                 // Reference count
    struct file_operations *ops;   // Operations table
};
```

### VFS Operation Flow

#### Example: Reading a File

```
1. User calls: fd = open("/test/file.txt", O_RDONLY)
   ↓
2. System call enters kernel (syscall interface)
   ↓
3. File descriptor allocated (fd_table)
   ↓
4. VFS resolves path: /test/file.txt
   - Finds mount point: /
   - Finds file system: ramfs
   ↓
5. VFS calls: ramfs->file_ops->open()
   ↓
6. RAMFS creates file structure, returns
   ↓
7. File descriptor (e.g., 3) returned to user

8. User calls: read(fd, buffer, 100)
   ↓
9. VFS looks up file by fd
   ↓
10. VFS calls: ramfs->file_ops->read(file, buffer, 100, position)
    ↓
11. RAMFS reads from memory, returns bytes read
    ↓
12. Bytes returned to user
```

---

## File System Implementations

### RAMFS (RAM File System)

**Purpose**: In-memory file system for temporary storage

**Architecture**:
```
Root Node (/)
├── name: "/"
├── mode: DIRECTORY | 0755
├── children ────┐
                 │
    ┌────────────┴─────────────┐
    │                          │
    ▼                          ▼
Node: "bin/"              Node: "welcome.txt"
├── parent: root          ├── parent: root
├── children: NULL        ├── data: "Welcome..."
└── next ──────────►      └── size: 150 bytes
                          └── next: NULL
```

**Data Structures**:

```c
// RAMFS node (file or directory)
struct ramfs_node {
    char name[RAMFS_MAX_NAME + 1];  // Filename
    uint32_t mode;                   // File type + permissions
    uint32_t size;                   // File size
    uint32_t ino;                    // Unique inode number
    void *data;                      // File content (for files)
    struct ramfs_node *parent;       // Parent directory
    struct ramfs_node *children;     // First child (for directories)
    struct ramfs_node *next;         // Next sibling
    uint32_t created_time;
    uint32_t modified_time;
    uint32_t accessed_time;
};

// RAMFS filesystem data
struct ramfs_fs_data {
    struct ramfs_node *root;         // Root directory
    uint32_t next_ino;               // Next inode number
    uint32_t file_count;             // Total files
    uint32_t used_memory;            // Memory used
};
```

**Key Operations**:
- `ramfs_create_file()`: Allocate node, add to parent
- `ramfs_read_file()`: Copy from node->data to buffer
- `ramfs_write_file()`: Allocate/resize node->data, copy from buffer
- `ramfs_resolve_path()`: Walk tree to find node

**Storage**: Everything in RAM (kmalloc), lost on reboot

### SFS (Simple File System)

**Purpose**: Persistent file system on block devices

**Architecture**:
```
Block Device
┌──────────────────────────────────────────────┐
│ Block 0: Superblock                          │
│  - Magic number: 0x53465300                  │
│  - Total blocks, free blocks                 │
│  - Root inode number                         │
├──────────────────────────────────────────────┤
│ Blocks 1-7: Block Bitmap                     │
│  - 1 bit per block (0=free, 1=used)         │
├──────────────────────────────────────────────┤
│ Blocks 8-63: Inode Table                     │
│  - Fixed-size inodes (64 bytes each)         │
│  - Contains metadata + block pointers        │
├──────────────────────────────────────────────┤
│ Blocks 64+: Data Blocks                      │
│  - File contents                             │
│  - Directory entries                         │
└──────────────────────────────────────────────┘
```

**Data Structures**:

```c
// Superblock (block 0)
struct sfs_superblock {
    uint32_t magic;              // 0x53465300 "SFS\0"
    uint32_t version;            // Version number
    uint32_t block_size;         // 4096 bytes
    uint32_t total_blocks;       // Total blocks on device
    uint32_t inode_blocks;       // Blocks for inodes
    uint32_t data_blocks;        // Blocks for data
    uint32_t free_blocks;        // Free data blocks
    uint32_t free_inodes;        // Free inodes
    uint32_t root_inode;         // Root directory inode
    uint32_t first_data_block;   // First data block number
    uint32_t bitmap_blocks;      // Blocks for bitmap
    // ... timestamps, label, etc.
};

// On-disk inode
struct sfs_inode {
    uint32_t mode;                      // Type + permissions
    uint32_t size;                      // File size
    uint32_t blocks;                    // Blocks allocated
    uint32_t direct[12];                // Direct block pointers
    uint32_t indirect;                  // Indirect block pointer
    uint32_t created_time;
    uint32_t modified_time;
    uint32_t accessed_time;
    uint32_t links;                     // Hard link count
    uint32_t flags;
};

// Directory entry
struct sfs_dirent {
    uint32_t inode;                     // Inode number
    uint16_t rec_len;                   // Entry length
    uint16_t name_len;                  // Name length
    char name[255];                     // Filename
};
```

**Key Operations**:
- `sfs_alloc_block()`: Find free bit in bitmap, mark used
- `sfs_read_block()`: Call block_device_read()
- `sfs_file_read()`: Map offset to block, read blocks
- `sfs_file_write()`: Allocate blocks as needed, write data

**Storage**: Persistent on block device, survives reboot

---

## Building New Features

### Adding New File Operations

#### Example: Add `truncate()` operation

**Step 1**: Update VFS interface in `vfs.h`:

```c
// Add to file_operations structure
struct file_operations {
    // ... existing operations ...
    int (*truncate)(struct file *file, off_t length);  // NEW
};

// Add VFS wrapper
int vfs_truncate(int fd, off_t length);
```

**Step 2**: Implement in VFS core (`vfs_core.c`):

```c
int vfs_truncate(int fd, off_t length)
{
    struct file *file = vfs_get_open_file(fd);
    if (!file) {
        return VFS_EINVAL;
    }
    
    if (file->ops && file->ops->truncate) {
        return file->ops->truncate(file, length);
    }
    
    return VFS_SUCCESS;  // No-op if not supported
}
```

**Step 3**: Implement in RAMFS (`ramfs_core.c`):

```c
static int ramfs_file_truncate(struct file *file, off_t length)
{
    if (!file || !file->inode) {
        return VFS_EINVAL;
    }
    
    struct ramfs_node *node = (struct ramfs_node *)file->inode->private_data;
    if (!node) {
        return VFS_EINVAL;
    }
    
    if (length == 0) {
        // Truncate to zero
        if (node->data) {
            kfree(node->data);
            node->data = NULL;
        }
        node->size = 0;
    } else if (length < node->size) {
        // Shrink file
        void *new_data = kmalloc(length);
        if (!new_data) {
            return VFS_ENOMEM;
        }
        memcpy(new_data, node->data, length);
        kfree(node->data);
        node->data = new_data;
        node->size = length;
    }
    // If length > size, do nothing (could extend with zeros)
    
    file->inode->size = node->size;
    node->modified_time++;
    
    return VFS_SUCCESS;
}

// Update operations table
static struct file_operations ramfs_file_ops = {
    .open = ramfs_file_open,
    .close = ramfs_file_close,
    .read = ramfs_file_read,
    .write = ramfs_file_write,
    .seek = ramfs_file_seek,
    .sync = ramfs_file_sync,
    .truncate = ramfs_file_truncate,  // NEW
    .ioctl = NULL
};
```

**Step 4**: Implement in SFS (`sfs_core.c`):

```c
static int sfs_file_truncate(struct file *file, off_t length)
{
    if (!file || !file->inode) {
        return VFS_EINVAL;
    }
    
    struct sfs_inode_data *inode_data = 
        (struct sfs_inode_data *)file->inode->private_data;
    if (!inode_data) {
        return VFS_EINVAL;
    }
    
    struct sfs_inode *disk_inode = &inode_data->disk_inode;
    uint32_t old_size = disk_inode->size;
    
    if (length < old_size) {
        // Free excess blocks
        uint32_t old_blocks = (old_size + SFS_BLOCK_SIZE - 1) / SFS_BLOCK_SIZE;
        uint32_t new_blocks = (length + SFS_BLOCK_SIZE - 1) / SFS_BLOCK_SIZE;
        
        for (uint32_t i = new_blocks; i < old_blocks && i < SFS_DIRECT_BLOCKS; i++) {
            if (disk_inode->direct[i] != 0) {
                sfs_free_block(file->fs, disk_inode->direct[i]);
                disk_inode->direct[i] = 0;
                disk_inode->blocks--;
            }
        }
        
        disk_inode->size = length;
        inode_data->dirty = 1;
    }
    
    file->inode->size = disk_inode->size;
    return VFS_SUCCESS;
}
```

**Step 5**: Add shell command (`builtin.c`):

```c
int shell_truncate(struct shell_context *ctx, int argc, char **argv)
{
    if (argc < 3) {
        shell_print(ctx, "Usage: truncate <file> <size>\n");
        return SHELL_ERROR;
    }
    
    const char *path = argv[1];
    off_t length = atoi(argv[2]);
    
    int fd = vfs_open(path, VFS_O_WRONLY, 0);
    if (fd < 0) {
        shell_print(ctx, "Error: Cannot open file\n");
        return SHELL_ERROR;
    }
    
    int result = vfs_truncate(fd, length);
    vfs_close(fd);
    
    if (result == VFS_SUCCESS) {
        shell_print(ctx, "File truncated successfully\n");
        return SHELL_SUCCESS;
    } else {
        shell_print(ctx, "Error truncating file\n");
        return SHELL_ERROR;
    }
}
```

### Adding File System Attributes

#### Example: Add access control lists (ACLs)

**Step 1**: Extend inode structure:

```c
// In vfs.h
struct inode {
    // ... existing fields ...
    void *acl_data;              // NEW: ACL data
};

// Define ACL structure
struct acl_entry {
    uint32_t uid;                // User ID
    uint32_t permissions;        // Read/write/execute
    struct acl_entry *next;      // Next entry
};
```

**Step 2**: Add operations:

```c
// In vfs.h
int vfs_set_acl(const char *path, struct acl_entry *acl);
struct acl_entry *vfs_get_acl(const char *path);
int vfs_check_permission(struct inode *inode, uint32_t uid, int requested);
```

**Step 3**: Implement in file systems as needed

---

## Adding a New File System

### Step-by-Step Guide

#### Step 1: Create Header File

Create `src/include/myfs.h`:

```c
#ifndef MYFS_H
#define MYFS_H

#include "vfs.h"
#include "block_device.h"

// MyFS constants
#define MYFS_MAGIC        0x4D594653  // "MYFS"
#define MYFS_BLOCK_SIZE   4096

// MyFS structures
struct myfs_superblock {
    uint32_t magic;
    // ... your superblock fields ...
};

struct myfs_inode {
    uint32_t mode;
    uint32_t size;
    // ... your inode fields ...
};

struct myfs_fs_data {
    struct myfs_superblock superblock;
    // ... your private data ...
};

// MyFS operations
extern struct file_system_type myfs_fs_type;

int myfs_init(void);
struct file_system *myfs_mount(struct block_device *dev, unsigned long flags);
void myfs_unmount(struct file_system *fs);
int myfs_format(struct block_device *dev);

#endif
```

#### Step 2: Create Implementation File

Create `src/fs/myfs/myfs_core.c`:

```c
#include "myfs.h"
#include "memory.h"
#include "kernel.h"

// Forward declarations
static int myfs_file_open(struct file *file, int flags, int mode);
static int myfs_file_close(struct file *file);
static ssize_t myfs_file_read(struct file *file, void *buf, size_t count, off_t offset);
static ssize_t myfs_file_write(struct file *file, const void *buf, size_t count, off_t offset);
static off_t myfs_file_seek(struct file *file, off_t offset, int whence);
static int myfs_file_sync(struct file *file);

static int myfs_dir_readdir(struct file *dir, void *buffer, size_t buffer_size, off_t *offset);
static int myfs_dir_mkdir(struct file_system *fs, const char *path, int mode);
static int myfs_dir_rmdir(struct file_system *fs, const char *path);
static struct inode *myfs_dir_lookup(struct file_system *fs, struct inode *parent, const char *name);

// Operations tables
static struct file_operations myfs_file_ops = {
    .open = myfs_file_open,
    .close = myfs_file_close,
    .read = myfs_file_read,
    .write = myfs_file_write,
    .seek = myfs_file_seek,
    .sync = myfs_file_sync,
    .ioctl = NULL
};

static struct directory_operations myfs_dir_ops = {
    .readdir = myfs_dir_readdir,
    .mkdir = myfs_dir_mkdir,
    .rmdir = myfs_dir_rmdir,
    .lookup = myfs_dir_lookup
};

// File system type
struct file_system_type myfs_fs_type = {
    .name = "myfs",
    .mount = myfs_mount,
    .unmount = myfs_unmount,
    .format = myfs_format,
    .file_ops = &myfs_file_ops,
    .dir_ops = &myfs_dir_ops
};

// Initialization
int myfs_init(void)
{
    early_print("Initializing MyFS...\n");
    
    if (vfs_register_filesystem(&myfs_fs_type) != VFS_SUCCESS) {
        early_print("Failed to register MyFS\n");
        return VFS_ERROR;
    }
    
    early_print("MyFS initialized\n");
    return VFS_SUCCESS;
}

// Format operation
int myfs_format(struct block_device *dev)
{
    if (!dev) {
        return VFS_EINVAL;
    }
    
    early_print("Formatting device with MyFS...\n");
    
    // TODO: Implement your format logic
    // 1. Create superblock
    // 2. Initialize allocation structures
    // 3. Create root directory
    // 4. Write to device
    
    return VFS_SUCCESS;
}

// Mount operation
struct file_system *myfs_mount(struct block_device *dev, unsigned long flags)
{
    if (!dev) {
        return NULL;
    }
    
    early_print("Mounting MyFS...\n");
    
    // Allocate file system structure
    struct file_system *fs = kmalloc(sizeof(struct file_system));
    if (!fs) {
        return NULL;
    }
    
    // Allocate private data
    struct myfs_fs_data *data = kmalloc(sizeof(struct myfs_fs_data));
    if (!data) {
        kfree(fs);
        return NULL;
    }
    
    // TODO: Read and validate superblock
    // TODO: Initialize your structures
    
    // Setup file system
    fs->type = &myfs_fs_type;
    fs->device = dev;
    fs->private_data = data;
    fs->mount_point = NULL;
    fs->flags = flags;
    
    early_print("MyFS mounted\n");
    return fs;
}

// Unmount operation
void myfs_unmount(struct file_system *fs)
{
    if (!fs || !fs->private_data) {
        return;
    }
    
    early_print("Unmounting MyFS...\n");
    
    struct myfs_fs_data *data = (struct myfs_fs_data *)fs->private_data;
    
    // TODO: Sync any dirty data
    // TODO: Free your structures
    
    kfree(data);
    kfree(fs);
    
    early_print("MyFS unmounted\n");
}

// Implement all file operations
static int myfs_file_open(struct file *file, int flags, int mode)
{
    // TODO: Implement
    (void)file; (void)flags; (void)mode;
    return VFS_SUCCESS;
}

static int myfs_file_close(struct file *file)
{
    // TODO: Implement
    (void)file;
    return VFS_SUCCESS;
}

static ssize_t myfs_file_read(struct file *file, void *buf, size_t count, off_t offset)
{
    // TODO: Implement
    (void)file; (void)buf; (void)count; (void)offset;
    return 0;
}

static ssize_t myfs_file_write(struct file *file, const void *buf, size_t count, off_t offset)
{
    // TODO: Implement
    (void)file; (void)buf; (void)count; (void)offset;
    return 0;
}

static off_t myfs_file_seek(struct file *file, off_t offset, int whence)
{
    // TODO: Implement
    (void)file; (void)whence;
    return offset;
}

static int myfs_file_sync(struct file *file)
{
    // TODO: Implement
    (void)file;
    return VFS_SUCCESS;
}

// Implement all directory operations
static int myfs_dir_readdir(struct file *dir, void *buffer, size_t buffer_size, off_t *offset)
{
    // TODO: Implement
    (void)dir; (void)buffer; (void)buffer_size; (void)offset;
    return 0;
}

static int myfs_dir_mkdir(struct file_system *fs, const char *path, int mode)
{
    // TODO: Implement
    (void)fs; (void)path; (void)mode;
    return VFS_SUCCESS;
}

static int myfs_dir_rmdir(struct file_system *fs, const char *path)
{
    // TODO: Implement
    (void)fs; (void)path;
    return VFS_SUCCESS;
}

static struct inode *myfs_dir_lookup(struct file_system *fs, struct inode *parent, const char *name)
{
    // TODO: Implement
    (void)fs; (void)parent; (void)name;
    return NULL;
}
```

#### Step 3: Register in Kernel

Add to `src/kernel/main.c`:

```c
#include "myfs.h"

// In kernel_main(), Phase 5 section:
if (myfs_init() != VFS_SUCCESS) {
    early_print("Warning: MyFS initialization failed\n");
}
```

#### Step 4: Update Build System

Add to Makefile:

```makefile
# File system sources
FS_SOURCES += $(SRC_DIR)/fs/myfs/myfs_core.c
```

#### Step 5: Test

```bash
make clean
make kernel ARCH=arm64

# In shell:
/MiniOS> help
# Should see MyFS registered in VFS
```

---

## Examples and Use Cases

### Use Case 1: Logging File System

**Purpose**: Transparently log all file operations

```c
struct logfs_data {
    struct file_system *underlying_fs;  // Wrapped file system
    int log_fd;                          // Log file descriptor
};

static ssize_t logfs_file_read(struct file *file, void *buf, size_t count, off_t offset)
{
    struct logfs_data *data = (struct logfs_data *)file->fs->private_data;
    
    // Log the operation
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "READ: file=%s, count=%zu, offset=%ld\n",
             file->inode->name, count, offset);
    vfs_write(data->log_fd, log_msg, strlen(log_msg));
    
    // Forward to underlying file system
    return data->underlying_fs->type->file_ops->read(file, buf, count, offset);
}
```

### Use Case 2: Compressed File System

**Purpose**: Store files compressed, decompress on read

```c
static ssize_t compfs_file_write(struct file *file, const void *buf, size_t count, off_t offset)
{
    // Compress data
    size_t compressed_size;
    void *compressed = compress_data(buf, count, &compressed_size);
    
    // Write compressed data
    ssize_t result = underlying_write(file, compressed, compressed_size, offset);
    
    kfree(compressed);
    return result;
}

static ssize_t compfs_file_read(struct file *file, void *buf, size_t count, off_t offset)
{
    // Read compressed data
    void *compressed = kmalloc(compressed_size);
    underlying_read(file, compressed, compressed_size, offset);
    
    // Decompress
    size_t decompressed_size;
    decompress_data(compressed, buf, &decompressed_size);
    
    kfree(compressed);
    return decompressed_size;
}
```

### Use Case 3: Network File System

**Purpose**: Access files over network

```c
static ssize_t netfs_file_read(struct file *file, void *buf, size_t count, off_t offset)
{
    struct netfs_data *data = (struct netfs_data *)file->fs->private_data;
    
    // Send read request to server
    struct netfs_request req = {
        .type = NETFS_READ,
        .inode = file->inode->ino,
        .offset = offset,
        .count = count
    };
    
    network_send(data->server_addr, &req, sizeof(req));
    
    // Receive response
    struct netfs_response resp;
    network_recv(data->server_addr, &resp, sizeof(resp));
    
    // Copy data
    memcpy(buf, resp.data, resp.size);
    return resp.size;
}
```

---

## Best Practices

### 1. **Always Check Pointers**
```c
if (!file || !file->inode || !buf) {
    return VFS_EINVAL;
}
```

### 2. **Use Reference Counting**
```c
file->ref_count++;  // When opening
file->ref_count--;  // When closing
if (file->ref_count == 0) {
    kfree(file);    // Only free when no references
}
```

### 3. **Handle Errors Gracefully**
```c
void *data = kmalloc(size);
if (!data) {
    return VFS_ENOMEM;  // Clear error code
}
```

### 4. **Sync on Close**
```c
static int myfs_file_close(struct file *file)
{
    // Flush any pending writes
    if (file->ops && file->ops->sync) {
        file->ops->sync(file);
    }
    return VFS_SUCCESS;
}
```

### 5. **Test Incrementally**
- Implement mount/unmount first
- Add read-only support
- Add write support
- Add directory operations
- Test thoroughly at each step

---

## Quick Reference

### Register File System
```c
vfs_register_filesystem(&my_fs_type);
```

### Mount File System
```c
vfs_mount("device", "/mnt/point", "myfs", 0);
```

### File Operations
```c
int fd = vfs_open("/path/to/file", VFS_O_RDWR, 0644);
vfs_read(fd, buffer, size);
vfs_write(fd, data, size);
vfs_seek(fd, offset, VFS_SEEK_SET);
vfs_close(fd);
```

### Directory Operations
```c
vfs_mkdir("/path/to/dir", 0755);
vfs_readdir(dir_fd, entries, count);
vfs_rmdir("/path/to/dir");
```

---

## Related Documentation

- **FILE_SYSTEM_CONFIGURATION_GUIDE.md** - Configuration instructions
- **FILE_SYSTEM_TEST_RESULTS.md** - Test results and verification
- **FS_IMPLEMENTATION_SUMMARY.md** - Implementation summary
- **docs/PHASE5_RAMFS_README.md** - RAMFS specific details

---

**Last Updated**: October 2024
**Version**: 1.0
**Status**: Complete

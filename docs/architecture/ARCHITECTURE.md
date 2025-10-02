# MiniOS Architecture Design

## Overview

MiniOS is designed as a microkernel-inspired operating system with a clean separation between architecture-specific and architecture-independent code. The design prioritizes educational value, code clarity, and cross-platform compatibility.

## Design Principles

1. **Simplicity**: Keep implementations minimal but functional
2. **Clarity**: Code should be readable and well-documented
3. **Modularity**: Clear separation of concerns between components
4. **Portability**: Abstract hardware differences through clean interfaces
5. **Educational Value**: Demonstrate core OS concepts clearly

## System Architecture

### High-Level Architecture

```
┌─────────────────────────────────────────────────────┐
│                 User Applications                   │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐   │
│  │    Shell    │ │   Editor    │ │    Games    │   │
│  └─────────────┘ └─────────────┘ └─────────────┘   │
├─────────────────────────────────────────────────────┤
│                System Call Interface                │
├─────────────────────────────────────────────────────┤
│                    Kernel Space                     │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐   │
│  │   Process   │ │   Memory    │ │    File     │   │
│  │  Manager    │ │  Manager    │ │   System    │   │
│  └─────────────┘ └─────────────┘ └─────────────┘   │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐   │
│  │   Device    │ │ Interrupt   │ │   Timer     │   │
│  │  Drivers    │ │  Handler    │ │  Service    │   │
│  └─────────────┘ └─────────────┘ └─────────────┘   │
├─────────────────────────────────────────────────────┤
│              Hardware Abstraction Layer            │
├─────────────────────────────────────────────────────┤
│              Architecture-Specific Layer            │
│        ┌─────────────────┬─────────────────┐        │
│        │      ARM64      │     x86-64      │        │
│        │   (AArch64)     │    (x86_64)     │        │
│        └─────────────────┴─────────────────┘        │
└─────────────────────────────────────────────────────┘
```

## Memory Management

### Virtual Memory Layout

#### ARM64 Memory Map
```
Virtual Address Space (48-bit)
0xFFFF_FFFF_FFFF_FFFF ┌─────────────────┐
                      │  Kernel Space   │
                      │    (256TB)      │
0xFFFF_0000_0000_0000 ├─────────────────┤
                      │    Reserved     │
                      │   (Non-canon)   │
0x0000_FFFF_FFFF_FFFF ├─────────────────┤
                      │   User Space    │
                      │    (256TB)      │
0x0000_0000_0000_0000 └─────────────────┘
```

#### x86-64 Memory Map
```
Virtual Address Space (48-bit)
0xFFFF_FFFF_FFFF_FFFF ┌─────────────────┐
                      │  Kernel Space   │
                      │    (128TB)      │
0xFFFF_8000_0000_0000 ├─────────────────┤
                      │    Reserved     │
                      │   (Non-canon)   │
0x0000_7FFF_FFFF_FFFF ├─────────────────┤
                      │   User Space    │
                      │    (128TB)      │
0x0000_0000_0000_0000 └─────────────────┘
```

### Page Table Management
- **Page Size**: 4KB (both architectures)
- **Levels**: 4-level page tables
- **Features**: NX bit, SMEP, SMAP (where available)

## Process Management

### Process Structure
```c
typedef struct process {
    pid_t pid;                    // Process ID
    pid_t parent_pid;             // Parent process ID
    enum process_state state;     // Running, sleeping, zombie, etc.
    
    struct vm_space *vm;          // Virtual memory space
    struct registers *regs;       // Saved registers
    
    struct list_head children;    // Child processes
    struct list_head siblings;    // Sibling processes
    
    struct file_descriptor_table *fdt;  // Open files
    
    char name[PROCESS_NAME_MAX];  // Process name
    
    // Scheduling information
    int priority;
    uint64_t time_slice;
    uint64_t cpu_time_used;
} process_t;
```

### Scheduling Algorithm
- **Algorithm**: Round-robin with priority levels
- **Time Slice**: 10ms default
- **Priority Levels**: 0 (highest) to 31 (lowest)
- **Preemption**: Timer-based preemptive multitasking

## File System

### Virtual File System (VFS)
```c
struct vfs_operations {
    int (*mount)(struct filesystem *fs, const char *path);
    int (*unmount)(const char *path);
    int (*open)(const char *path, int flags);
    int (*close)(int fd);
    ssize_t (*read)(int fd, void *buf, size_t count);
    ssize_t (*write)(int fd, const void *buf, size_t count);
    off_t (*seek)(int fd, off_t offset, int whence);
};
```

### Supported File Systems
1. **Initial RAM Disk (initrd)**: Boot-time file system
2. **FAT32**: Simple, widely supported file system
3. **Future**: Ext2/3 for more advanced features

## Device Driver Framework

### Driver Interface
```c
struct device_driver {
    const char *name;
    int (*probe)(struct device *dev);
    int (*remove)(struct device *dev);
    int (*suspend)(struct device *dev);
    int (*resume)(struct device *dev);
    
    struct driver_operations *ops;
};

struct driver_operations {
    int (*open)(struct device *dev);
    int (*close)(struct device *dev);
    ssize_t (*read)(struct device *dev, void *buf, size_t count);
    ssize_t (*write)(struct device *dev, const void *buf, size_t count);
    int (*ioctl)(struct device *dev, unsigned int cmd, unsigned long arg);
};
```

### Planned Drivers
- **Console**: Text output and keyboard input
- **Serial**: Debug output and communication
- **Timer**: System timing and scheduling
- **Interrupt Controller**: ARM GIC / x86 APIC
- **Storage**: Simple block device support

## System Call Interface

### System Call Numbers
```c
#define SYS_EXIT      1
#define SYS_FORK      2
#define SYS_READ      3
#define SYS_WRITE     4
#define SYS_OPEN      5
#define SYS_CLOSE     6
#define SYS_EXEC      7
#define SYS_WAIT      8
#define SYS_MMAP      9
#define SYS_MUNMAP   10
// ... more system calls
```

### System Call Mechanism

#### ARM64 System Calls
- **Instruction**: `svc #0`
- **Number**: Passed in `x8` register
- **Arguments**: `x0-x5` registers
- **Return**: `x0` register

#### x86-64 System Calls
- **Instruction**: `syscall`
- **Number**: Passed in `rax` register  
- **Arguments**: `rdi, rsi, rdx, rcx, r8, r9`
- **Return**: `rax` register

## Hardware Abstraction Layer (HAL)

### Core HAL Interface
```c
struct hal_operations {
    // Memory management
    void (*map_page)(vaddr_t vaddr, paddr_t paddr, uint64_t flags);
    void (*unmap_page)(vaddr_t vaddr);
    paddr_t (*virt_to_phys)(vaddr_t vaddr);
    
    // Interrupt handling
    void (*enable_interrupts)(void);
    void (*disable_interrupts)(void);
    void (*register_irq_handler)(int irq, irq_handler_t handler);
    
    // Timer operations
    void (*timer_init)(void);
    uint64_t (*get_ticks)(void);
    void (*set_timer)(uint64_t ticks);
    
    // Context switching
    void (*save_context)(struct registers *regs);
    void (*restore_context)(struct registers *regs);
    void (*switch_to_user_mode)(vaddr_t entry_point, vaddr_t stack_pointer);
};
```

## Boot Process

### Boot Sequence
1. **Firmware**: UEFI (ARM64) / BIOS (x86-64)
2. **Bootloader**: Load kernel and initial ramdisk
3. **Kernel Init**: Architecture detection and HAL setup
4. **Memory Setup**: Initialize page tables and heap
5. **Driver Init**: Load essential drivers
6. **Process Init**: Create init process
7. **User Space**: Launch shell

### Boot Information Structure
```c
struct boot_info {
    uint64_t magic;               // Boot magic number
    enum architecture arch;       // ARM64 or x86_64
    
    struct memory_map *mem_map;   // Available memory regions
    size_t mem_map_size;
    
    void *initrd_start;          // Initial ramdisk
    size_t initrd_size;
    
    void *framebuffer;           // Graphics framebuffer
    struct fb_info fb_info;      // FB width, height, etc.
    
    char *cmdline;               // Kernel command line
};
```

## Error Handling and Debugging

### Panic System
```c
void panic(const char *fmt, ...);
void assert(bool condition, const char *msg);

// Architecture-specific panic handlers
void arch_panic_handler(void);
```

### Debug Output
- **Early Boot**: Serial port output
- **Runtime**: Kernel log buffer + serial
- **User Space**: Standard output via system calls

### Logging Levels
```c
enum log_level {
    LOG_DEBUG = 0,
    LOG_INFO  = 1,
    LOG_WARN  = 2,
    LOG_ERROR = 3,
    LOG_FATAL = 4
};
```

## Security Considerations

### Memory Protection
- **User/Kernel Separation**: Distinct address spaces
- **NX Bit**: Non-executable memory pages
- **Stack Guards**: Detect stack overflow
- **ASLR**: Address Space Layout Randomization (future)

### Privilege Levels
- **ARM64**: EL0 (user), EL1 (kernel)
- **x86-64**: Ring 0 (kernel), Ring 3 (user)

## Performance Considerations

### Optimization Strategies
1. **Fast System Calls**: Minimize context switch overhead
2. **Lazy Loading**: Load pages on demand
3. **Cache-Friendly**: Align structures to cache lines
4. **Minimize Copies**: Use zero-copy where possible

### Benchmarking Points
- System call latency
- Context switch time  
- Memory allocation speed
- I/O throughput

## Future Extensions

### Planned Features
- **Networking**: Basic TCP/IP stack
- **SMP**: Multi-core support
- **Security**: Capability-based security
- **Real-time**: Soft real-time features
- **Containers**: Basic containerization

### Research Areas
- **Microkernel Design**: Move more to user space
- **Formal Verification**: Prove correctness
- **Performance**: Optimize critical paths
- **Security**: Advanced protection mechanisms
# Phase 4: Device Drivers & System Services

## Overview

Phase 4 builds upon the Phase 3 memory management foundation to implement core operating system services and a device driver framework. This phase introduces advanced system capabilities including hardware device management, basic process scheduling, and system call interfaces.

## Phase 4 Status: Complete Operating System Services ✅ (100%)

### ✅ COMPLETED (Complete System Services - 100% of Phase 4)
1. **✅ Device Driver Framework** - Unified driver interface and device management
2. **✅ Timer Services** - ARM64 Generic Timer and x86-64 PIT/APIC timer support
3. **✅ UART/Serial Drivers** - ARM64 PL011 and x86-64 16550 communication interfaces
4. **✅ Interrupt Management** - ARM64 GIC and x86-64 PIC/IDT hardware interrupt routing
5. **✅ Process Management** - Task creation, scheduling, and context switching
6. **✅ System Call Interface** - ARM64 SVC and x86-64 SYSCALL user-kernel communication
7. **✅ Cross-Platform APIs** - Unified interfaces hiding architecture differences
8. **✅ Build System Integration** - Complete system service compilation and linking

### Major Achievement: Complete Operating System Core
**MiniOS now has the fundamental services of a real operating system:**
- **Multi-tasking**: Process creation and round-robin scheduling
- **Hardware Management**: Timer, UART, and interrupt controller drivers
- **User-Kernel Interface**: System calls for process control and I/O
- **Context Switching**: Full register preservation across task switches
- **Cross-Platform Support**: Identical APIs across ARM64 and x86-64

### Final Build Results
- **ARM64 Kernel**: 148KB with complete operating system services
- **x86-64 Kernel**: 124KB with full system service implementation
- **System Capabilities**: Multi-tasking, system calls, hardware drivers, interrupts

### Foundation Built Upon (Phase 3)
- ✅ **Memory Management** - Virtual memory, page tables, and allocation
- ✅ **Exception Handling** - ARM64 vectors and x86-64 interrupt framework
- ✅ **Cross-Platform APIs** - Unified interfaces across architectures
- ✅ **Enhanced Kernel** - Memory initialization and testing

## Prerequisites

### Phase 3 Foundation (Required)
- ✅ Memory management operational (ARM64 MMU + x86-64 paging)
- ✅ Exception handling framework installed
- ✅ Physical memory allocation working
- ✅ Enhanced kernel with memory initialization
- ✅ 29 Phase 3 tests passing (100% pass rate)

### Validation Commands
```bash
# Verify Phase 3 foundation
./tools/test-phase3.sh                     # Should show 29/29 PASS
make ARCH=arm64 clean all                  # Should build 84KB kernel
make ARCH=x86_64 clean all                 # Should build 17KB kernel

# Check git status
git branch --show-current                  # Should show phase-4-system-services
git log --oneline -3                       # Should show v0.3.0 and Phase 3 completion
```

## Architecture

### Device Driver Framework

#### Device and Driver Interface
```c
// Device driver operations
struct device_driver {
    const char *name;
    int (*probe)(struct device *dev);
    int (*init)(struct device *dev);
    void (*cleanup)(struct device *dev);
    int (*read)(struct device *dev, void *buf, size_t len, off_t offset);
    int (*write)(struct device *dev, const void *buf, size_t len, off_t offset);
    int (*ioctl)(struct device *dev, unsigned long cmd, unsigned long arg);
};

// Device representation
struct device {
    const char *name;
    uint32_t device_id;
    void *platform_data;
    struct device_driver *driver;
    void *private_data;
};
```

### Timer Services

#### ARM64 Generic Timer
```c
// ARM64 timer driver
struct arm64_timer_dev {
    struct device dev;
    uint64_t frequency;
    void *irq;
};

// Timer operations
int timer_init(struct device *dev);
uint64_t timer_get_frequency(struct device *dev);
void timer_set_compare(struct device *dev, uint64_t compare);
```

#### x86-64 PIT Timer
```c
// x86-64 PIT timer driver
struct pit_timer_dev {
    struct device dev;
    uint16_t frequency;
    uint8_t channel;
};

// PIT operations
int pit_timer_init(struct device *dev);
void pit_timer_set_frequency(struct device *dev, uint16_t freq);
void pit_timer_start(struct device *dev);
```

### UART/Serial Drivers

#### ARM64 PL011 UART
```c
// ARM64 PL011 UART driver
struct pl011_uart_dev {
    struct device dev;
    void *base_addr;
    uint32_t baud_rate;
    uint32_t data_bits;
    uint32_t stop_bits;
    uint32_t parity;
};

// PL011 operations
int pl011_uart_init(struct device *dev);
int pl011_uart_putchar(struct device *dev, char c);
int pl011_uart_getchar(struct device *dev);
void pl011_uart_puts(struct device *dev, const char *s);
```

#### x86-64 16550 UART
```c
// x86-64 16550 UART driver
struct serial16550_dev {
    struct device dev;
    uint16_t base_port;
    uint32_t baud_rate;
    uint8_t line_control;
};

// 16550 operations
int serial16550_init(struct device *dev);
int serial16550_putchar(struct device *dev, char c);
int serial16550_getchar(struct device *dev);
void serial16550_puts(struct device *dev, const char *s);
```

### Interrupt Management

#### ARM64 GIC
```c
// ARM64 GIC interrupt controller
struct gic_dev {
    struct device dev;
    void *gicd_base;    // Distributor base
    void *gicc_base;    // CPU interface base
    uint32_t num_irqs;
};

// GIC operations
int gic_init(struct device *dev);
void gic_enable_irq(struct device *dev, uint32_t irq);
void gic_disable_irq(struct device *dev, uint32_t irq);
void gic_set_irq_type(struct device *dev, uint32_t irq, uint32_t type);
```

#### x86-64 PIC/IDT
```c
// x86-64 interrupt descriptor table
struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_middle;
    uint32_t offset_high;
    uint32_t zero;
};

// IDT operations
void idt_init(void);
void idt_set_gate(uint8_t num, uint64_t handler, uint16_t sel, uint8_t flags);
```

### Process Management

#### Process Control Block
```c
// Process structure
struct process {
    uint32_t pid;
    uint32_t state;           // RUNNING, READY, BLOCKED
    uint32_t priority;

    // CPU context
    struct cpu_context *context;

    // Memory management
    uint64_t page_dir;
    uint64_t kernel_stack;
    uint64_t user_stack;

    // Scheduling
    uint64_t time_slice;
    uint64_t time_used;

    struct process *next;
    struct process *prev;
};
```

#### Context Switching
```c
// CPU context for architecture-specific registers
struct cpu_context {
    // General purpose registers
    uint64_t x0, x1, x2, x3, x4, x5, x6, x7;
    uint64_t x8, x9, x10, x11, x12, x13, x14, x15;
    uint64_t x16, x17, x18, x19, x20, x21, x22, x23;
    uint64_t x24, x25, x26, x27, x28, x29;

    // Special registers
    uint64_t sp;              // Stack pointer
    uint64_t pc;              // Program counter
    uint64_t pstate;          // Processor state
    uint64_t tpidr_el0;       // Thread pointer
};
```

### System Call Interface

#### ARM64 SVC System Calls
```c
// ARM64 system call interface
#define SYSCALL_EXIT      0
#define SYSCALL_WRITE     1
#define SYSCALL_READ      2
#define SYSCALL_OPEN      3
#define SYSCALL_CLOSE     4
#define SYSCALL_YIELD     5
#define SYSCALL_FORK      6
#define SYSCALL_EXEC      7
#define SYSCALL_WAIT      8
#define SYSCALL_GETPID    9

// System call handler
void syscall_handler(struct cpu_context *regs);
int64_t syscall_dispatch(uint64_t syscall_num, uint64_t arg1, uint64_t arg2,
                        uint64_t arg3, uint64_t arg4, uint64_t arg5);
```

#### x86-64 SYSCALL Interface
```c
// x86-64 system call interface
#define SYSCALL_EXIT      0
#define SYSCALL_WRITE     1
#define SYSCALL_READ      2
#define SYSCALL_OPEN      3
#define SYSCALL_CLOSE     4
#define SYSCALL_YIELD     5
#define SYSCALL_FORK      6
#define SYSCALL_EXEC      7
#define SYSCALL_WAIT      8
#define SYSCALL_GETPID    9

// System call handler (x86-64)
void syscall_handler_x86_64(struct cpu_context *regs);
int64_t syscall_dispatch_x86_64(uint64_t syscall_num, uint64_t arg1, uint64_t arg2,
                                uint64_t arg3, uint64_t arg4, uint64_t arg5);
```

## Building Phase 4

### Build Commands
```bash
# Build Phase 4 complete system services
make clean all                              # ARM64 (default)
make ARCH=x86_64 clean all                 # x86-64

# Debug builds
make DEBUG=1 clean all                     # ARM64 debug
make DEBUG=1 ARCH=x86_64 clean all         # x86-64 debug

# Phase 4 testing
./tools/test-phase4.sh                     # 40 comprehensive tests
```

### Expected Output
```bash
# ARM64 build
CC      src/arch/arm64/boot/start.S
CC      src/arch/arm64/kernel/trap.S
CC      src/drivers/timer/arm64_timer.c
CC      src/drivers/uart/pl011_uart.c
CC      src/drivers/interrupt/gic.c
CC      src/kernel/process/process.c
CC      src/kernel/syscall/syscall.c
LD      build/arm64/kernel.elf
OBJCOPY build/arm64/kernel.bin
CREATE  build/arm64/minios.img

# Build results:
# ARM64 kernel: 148KB (with full system services)
# x86-64 kernel: 124KB (with complete implementation)
```

## Testing Phase 4

### Automated Testing Script
```bash
#!/bin/bash
# Phase 4 comprehensive testing

./tools/test-phase4.sh

# Expected output:
# Test 1: Device driver framework - PASS
# Test 2: Timer services - PASS
# Test 3: UART communication - PASS
# Test 4: Interrupt management - PASS
# Test 5: Process creation - PASS
# Test 6: Context switching - PASS
# Test 7: System calls - PASS
# Test 8: Cross-platform APIs - PASS
# Test 9: Process scheduling - PASS
# Test 10: User-kernel interface - PASS
# ...
# Total: 40/40 tests passed (100%)
```

### Manual Validation
```bash
# 1. Build system services
make clean all
ls -la build/arm64/kernel.elf

# 2. Test in VM
make test
# Should see:
# - Timer interrupt messages
# - UART initialization
# - Process creation success
# - System call handling
# - Multi-tasking demonstration

# 3. Debug session
make debug
(gdb) break process_create
(gdb) continue
# Should hit process creation breakpoints
```

## Usage and Features

### Timer Services Usage
```c
// Initialize timer
struct device *timer = device_get_by_name("arm64-timer");
timer->driver->init(timer);

// Set 1-second timer
uint64_t frequency = timer_get_frequency(timer);
timer_set_compare(timer, frequency);

// Timer interrupt handler
void timer_irq_handler(struct device *dev) {
    // Process scheduling
    schedule();
}
```

### UART Communication
```c
// Initialize UART
struct device *uart = device_get_by_name("pl011-uart");
uart->driver->init(uart);

// Send character
uart->driver->write(uart, "Hello", 5, 0);

// Receive character
char c;
uart->driver->read(uart, &c, 1, 0);
```

### Process Management
```c
// Create new process
struct process *proc = process_create("test_process");
process_set_priority(proc, 5);
process_add_to_ready_queue(proc);

// Yield CPU
syscall_yield();

// Fork process
pid_t child_pid = syscall_fork();
if (child_pid == 0) {
    // Child process
    syscall_exec("/test_app", 0, 0);
} else {
    // Parent process
    syscall_wait(child_pid);
}
```

### System Calls
```c
// System call from user space
syscall_write(1, "Hello from user space\n", 23);

// System call implementation in kernel
int64_t syscall_write(uint64_t fd, uint64_t buf, uint64_t count) {
    // Validate user buffer
    if (!validate_user_ptr(buf, count)) {
        return -1;  // EFAULT
    }

    // Handle system call
    return device_write(fd, (void*)buf, count);
}
```

## Troubleshooting Phase 4

### Common Issues

#### Timer Interrupts Not Working
```bash
# Check GIC/PIC initialization
make DEBUG=1
(gdb) break timer_irq_handler
(gdb) continue
# Verify interrupt controller setup
```

#### Process Creation Fails
```bash
# Check memory allocation
(gdb) break kmalloc
(gdb) break process_create
# Verify kernel memory available
```

#### System Calls Crash
```bash
# Check system call handler
(gdb) break syscall_handler
(gdb) continue
# Verify register context preservation
```

### Debug Techniques
```bash
# Enable debug logging
make DEBUG=1

# Use GDB for kernel debugging
make debug
(gdb) break schedule
(gdb) break process_switch
(gdb) break syscall_dispatch
```

## Next Steps

### Ready for Phase 5 when:
1. **✅ All 40 tests pass** - Complete system service validation
2. **✅ Multi-tasking works** - Process creation and scheduling
3. **✅ System calls functional** - User-kernel communication
4. **✅ Hardware drivers operational** - Timer, UART, interrupts
5. **✅ Cross-platform consistency** - Identical behavior on ARM64/x86-64

### Proceed to Phase 5: File System Implementation
- **VFS Layer** - Virtual File System abstraction
- **Block Device Drivers** - Storage device management
- **File System Implementation** - SFS (Simple File System)
- **File Operations** - Open, read, write, close, directory operations

Phase 4 establishes the complete core operating system services needed for file system development in Phase 5!
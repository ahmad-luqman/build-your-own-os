# Phase 4 Implementation Session Prompt

## Context for New Session

You are continuing development of **MiniOS**, an educational operating system project. **Phase 3 (Memory Management & Kernel Loading) is complete** and you need to implement **Phase 4: Device Drivers & System Services**.

### Project Overview
- **Project**: MiniOS - Cross-platform educational operating system
- **Architectures**: ARM64 (UTM on macOS) + x86-64 (QEMU cross-platform)
- **Current Status**: Phase 3 complete, Phase 4 ready to begin
- **Repository**: `/Users/ahmadluqman/src/build-your-own-os`
- **Git Branch**: `main` (Phase 3 released as v0.3.0)
- **Overall Progress**: 43% (3/7 phases complete)

### What's Already Complete ✅

#### Phase 1: Foundation Setup (v0.1.0)
- **Cross-platform Build System** with ARM64 and x86-64 support
- **Development Toolchain**: Build, test, debug, and VM management scripts
- **Automated Testing**: 32 tests with 100% pass rate
- **VM Configurations**: UTM (ARM64) and QEMU (x86-64) ready
- **Basic OS Components**: Kernel stubs and hardware abstraction layer

#### Phase 2: Enhanced Bootloader Implementation (v0.2.0)
- **Enhanced ARM64 UEFI Bootloader** - Proper UEFI application with memory detection
- **Enhanced x86-64 Multiboot2 Bootloader** - Full compliance with boot information parsing
- **Cross-Platform Boot Protocol** - Unified boot_info structure for kernel interface
- **Boot Information Structures** - Memory maps, graphics info, and system data
- **Separated Build System** - Independent bootloader and kernel compilation
- **Comprehensive Testing** - 20 tests with 100% pass rate for Phase 2 functionality

#### Phase 3: Memory Management & Kernel Loading (v0.3.0)
- **Cross-Platform Memory Interface** - Unified memory management API (`memory.h`, `exceptions.h`, `kernel_loader.h`)
- **ARM64 Memory Management** - Full MMU setup with TTBR0/TTBR1, 4KB page tables, exception vectors
- **x86-64 Memory Management** - 4-level paging, NX bit support, memory protection
- **Physical Memory Allocation** - Bitmap-based allocators for both architectures
- **Exception Handling Framework** - 16 ARM64 exception vectors + x86-64 framework
- **Enhanced Kernel Integration** - Memory initialization, allocation testing, error handling
- **Comprehensive Testing** - 29 tests with 100% pass rate

### Phase 4 Goals: Device Drivers & System Services

You need to build upon the Phase 3 memory management foundation to implement core operating system services and device driver framework:

#### For ARM64 (UEFI/UTM):
1. **Timer Services** - ARM Generic Timer integration for scheduling and delays
2. **UART Driver** - Serial console for debugging and communication
3. **Interrupt Controller** - GICv2/GICv3 support for device interrupts
4. **Device Tree Parsing** - Hardware discovery and device configuration
5. **Process Management** - Basic task creation and context switching
6. **System Call Interface** - SVC instruction handling for user-kernel interface

#### For x86-64 (Multiboot2/QEMU):
1. **Timer Services** - PIT, APIC timer for scheduling and timing
2. **Serial Driver** - 16550 UART for debugging and communication  
3. **Interrupt Controller** - IDT setup, PIC/APIC configuration
4. **PCI Bus Enumeration** - Device discovery and configuration
5. **Process Management** - Basic task creation and context switching
6. **System Call Interface** - SYSCALL instruction handling

#### Cross-Platform Requirements:
1. **Device Driver Framework** - Unified driver interface and registration
2. **Interrupt Management** - Architecture-abstracted interrupt handling
3. **Process Scheduler** - Round-robin scheduler with time slicing
4. **System Call Framework** - Cross-platform syscall interface
5. **Basic IPC Mechanisms** - Message passing and shared memory
6. **Service Management** - Kernel service registration and discovery

### Current Project Structure

```
build-your-own-os/
├── 📄 Makefile                     ✅ Enhanced build system with Phase 3 support
├── 📁 src/
│   ├── 📁 include/                 ✅ Enhanced with memory management APIs
│   │   ├── boot_protocol.h         ✅ Cross-platform boot interface
│   │   ├── memory.h                ✅ Memory management interface
│   │   ├── exceptions.h            ✅ Exception handling interface
│   │   └── kernel_loader.h         ✅ Kernel loading interface
│   ├── 📁 kernel/                  ✅ Enhanced with memory management
│   │   ├── main.c                  ✅ Memory initialization and testing
│   │   ├── memory.c                ✅ Cross-platform memory implementation
│   │   └── exceptions.c            ✅ Cross-platform exception handling
│   ├── 📁 arch/
│   │   ├── 📁 arm64/               ✅ Complete memory management implementation
│   │   │   ├── boot.S              ✅ UEFI entry point
│   │   │   ├── uefi_boot.c         ✅ UEFI bootloader implementation
│   │   │   ├── init.c              ✅ Architecture initialization
│   │   │   ├── memory/             ✅ MMU setup and allocation
│   │   │   │   ├── mmu.c           ✅ ARM64 MMU configuration
│   │   │   │   └── allocator.c     ✅ Physical memory allocation
│   │   │   ├── interrupts/         ✅ Exception handling
│   │   │   │   ├── vectors.S       ✅ 16 exception vectors
│   │   │   │   └── handlers.c      ✅ Exception handling
│   │   │   └── kernel_loader/      ✅ ELF loading framework
│   │   │       └── elf_loader.c    ✅ Kernel loading interface
│   │   └── 📁 x86_64/              ✅ Complete memory management implementation
│   │       ├── boot.asm            ✅ Multiboot2 entry with long mode
│   │       ├── boot_main.c         ✅ Multiboot2 parsing implementation
│   │       ├── init.c              ✅ Architecture initialization
│   │       ├── memory/             ✅ Paging setup and allocation
│   │       │   ├── paging.c        ✅ x86-64 4-level paging
│   │       │   └── allocator.c     ✅ Physical memory allocation
│   │       ├── interrupts/         ✅ Exception handling framework
│   │       │   └── handlers.c      ✅ Basic exception handling
│   │       └── kernel_loader/      ✅ ELF loading framework
│   │           └── elf_loader.c    ✅ Kernel loading interface
│   ├── 📁 drivers/                 📁 Ready for Phase 4 (EMPTY - TODO)
│   │   ├── 📁 uart/                🎯 CREATE FOR PHASE 4
│   │   ├── 📁 timer/               🎯 CREATE FOR PHASE 4
│   │   ├── 📁 interrupt/           🎯 CREATE FOR PHASE 4
│   │   └── 📁 pci/                 🎯 CREATE FOR PHASE 4
│   ├── 📁 fs/                      📁 Ready for Phase 5
│   └── 📁 userland/                📁 Ready for Phase 6
├── 📁 tools/                       ✅ Complete development toolchain
│   ├── test-phase1.sh             ✅ Phase 1 tests (32 tests, 100% pass)
│   ├── test-phase2.sh             ✅ Phase 2 tests (20 tests, 100% pass)
│   ├── test-phase3.sh             ✅ Phase 3 tests (29 tests, 100% pass)
│   └── quick-test-phase3.sh       🎯 CREATE FOR PHASE 4
├── 📁 vm-configs/                  ✅ VM configurations
├── 📁 docs/                        ✅ Comprehensive documentation
│   ├── PHASE1_USAGE.md            ✅ Phase 1 usage guide
│   ├── PHASE2_IMPLEMENTATION.md   ✅ Phase 2 implementation guide
│   ├── PHASE3_IMPLEMENTATION.md   ✅ Phase 3 implementation guide
│   └── [8 other guides]           ✅ Complete documentation
└── 📄 PHASE_PROGRESS_TRACKER.md   📋 43% complete (3/7 phases)

Git Branches & Tags:
├── main                            ✅ Production (v0.3.0 - Phase 3 complete)
├── develop                         ✅ Integration branch (merged Phase 3)
├── phase-1-foundation             ✅ Phase 1 work (v0.1.0)
├── phase-2-bootloader             ✅ Phase 2 work (v0.2.0)
├── phase-3-kernel-loading         ✅ Phase 3 work (v0.3.0)
└── phase-4-system-services        🎯 CURRENT BRANCH FOR PHASE 4
```

### Quick Start Commands

#### Get oriented:
```bash
cd /Users/ahmadluqman/src/build-your-own-os
git status           # Should be on phase-4-system-services branch
git log --oneline -5 # Should show v0.3.0 and Phase 3 completion
make info            # Check build system status
```

#### Verify Phase 3 functionality:
```bash
./tools/test-phase3.sh        # Should show all 29 ✅ PASS
make ARCH=arm64 clean all     # Should build with memory management
make ARCH=x86_64 clean all    # Should build with memory management
```

#### Current build status:
```bash
make clean
make ARCH=arm64 all      # Should create enhanced kernel with memory management
make ARCH=x86_64 all     # Should create enhanced kernel with memory management
```

#### Available documentation:
```bash
ls docs/                           # 10 comprehensive guides including Phase 3
cat docs/PHASE3_IMPLEMENTATION.md  # Phase 3 implementation details
cat docs/BUILD.md                  # Build system usage
cat PHASE_PROGRESS_TRACKER.md     # Overall progress (43% complete)
```

### What You Need to Implement

#### 1. Device Driver Framework (`src/drivers/` and `src/include/`)

**Create Driver Interface**:
```c
// src/include/driver.h
struct device_driver {
    const char *name;
    int (*probe)(struct device *dev);
    int (*init)(struct device *dev);
    void (*cleanup)(struct device *dev);
    int (*read)(struct device *dev, void *buf, size_t len, off_t offset);
    int (*write)(struct device *dev, const void *buf, size_t len, off_t offset);
    int (*ioctl)(struct device *dev, unsigned int cmd, unsigned long arg);
};

// Driver registration and management
int driver_register(struct device_driver *driver);
void driver_unregister(struct device_driver *driver);
struct device *device_create(const char *name, struct device_driver *driver);
```

**Device Management Framework**:
```c
// src/kernel/device.c
struct device {
    char name[64];
    struct device_driver *driver;
    void *private_data;
    struct device *next;
    int device_id;
    uint32_t flags;
};

// Device tree for ARM64, PCI enumeration for x86-64
int device_init(struct boot_info *boot_info);
```

#### 2. Timer Services (`src/drivers/timer/`)

**ARM64 Generic Timer**:
```c
// src/drivers/timer/arm64_timer.c
struct arm64_timer {
    uint64_t frequency;     // Timer frequency from CNTFRQ_EL0
    uint32_t control;       // Timer control settings
};

// Timer operations
void arm64_timer_init(void);
uint64_t arm64_timer_get_count(void);
void arm64_timer_set_timeout(uint64_t ticks);
void arm64_timer_enable_interrupt(void);
```

**x86-64 Timer Support**:
```c
// src/drivers/timer/x86_64_timer.c  
struct x86_64_timer {
    uint32_t frequency;     // PIT/APIC timer frequency
    uint16_t divisor;       // Timer divisor for desired frequency
};

// Timer operations
void x86_64_pit_init(uint32_t frequency);
void x86_64_apic_timer_init(void);
uint64_t x86_64_timer_get_ticks(void);
```

#### 3. UART/Serial Drivers (`src/drivers/uart/`)

**ARM64 PL011 UART**:
```c
// src/drivers/uart/pl011.c
struct pl011_uart {
    volatile uint32_t *base;    // MMIO base address
    uint32_t baudrate;          // Baud rate configuration
    uint32_t clock_rate;        // Input clock rate
};

// UART operations
int pl011_init(struct device *dev);
void pl011_putc(struct device *dev, char c);
char pl011_getc(struct device *dev);
void pl011_puts(struct device *dev, const char *str);
```

**x86-64 16550 UART**:
```c
// src/drivers/uart/16550.c
struct uart_16550 {
    uint16_t port;              // I/O port base address
    uint32_t baudrate;          // Baud rate
    uint8_t line_control;       // Line control settings
};

// UART operations  
int uart_16550_init(struct device *dev);
void uart_16550_putc(struct device *dev, char c);
char uart_16550_getc(struct device *dev);
```

#### 4. Interrupt Management (`src/drivers/interrupt/`)

**ARM64 GIC (Generic Interrupt Controller)**:
```c
// src/drivers/interrupt/gic.c
struct gic_controller {
    volatile uint32_t *distributor_base;    // GICD base address
    volatile uint32_t *cpu_interface_base;  // GICC base address
    uint32_t num_irqs;                     // Number of supported IRQs
};

// GIC operations
int gic_init(void);
void gic_enable_irq(uint32_t irq);
void gic_disable_irq(uint32_t irq);
void gic_set_priority(uint32_t irq, uint8_t priority);
uint32_t gic_acknowledge_irq(void);
void gic_end_of_interrupt(uint32_t irq);
```

**x86-64 Interrupt Controllers**:
```c
// src/drivers/interrupt/pic.c - 8259 PIC
// src/drivers/interrupt/apic.c - Local APIC
struct interrupt_controller {
    const char *name;
    int (*init)(void);
    void (*enable_irq)(uint32_t irq);
    void (*disable_irq)(uint32_t irq);
    void (*send_eoi)(uint32_t irq);
};
```

#### 5. Process Management (`src/kernel/process/`)

**Task Structure**:
```c
// src/include/process.h
struct task {
    uint32_t pid;                   // Process ID
    uint32_t state;                 // Task state (running, ready, blocked)
    void *stack_base;               // Stack base address
    size_t stack_size;              // Stack size
    struct cpu_context context;     // Saved CPU context
    struct task *next;              // Next task in scheduler queue
    uint64_t time_slice;            // Remaining time slice
    uint32_t priority;              // Task priority
};

// Process operations
int process_create(void (*entry)(void), const char *name);
void process_yield(void);
void process_sleep(uint64_t milliseconds);
void process_exit(int exit_code);
```

**Scheduler Implementation**:
```c
// src/kernel/process/scheduler.c
struct scheduler {
    struct task *current_task;      // Currently running task
    struct task *ready_queue;       // Ready task queue
    uint64_t tick_count;            // Scheduler tick counter
    uint32_t time_slice_quantum;    // Time slice length
};

// Scheduler operations
void scheduler_init(void);
void scheduler_tick(void);
struct task *scheduler_pick_next_task(void);
void scheduler_switch_to_task(struct task *task);
```

#### 6. System Call Interface (`src/kernel/syscall/`)

**System Call Framework**:
```c
// src/include/syscall.h
#define SYSCALL_EXIT        0
#define SYSCALL_PRINT       1
#define SYSCALL_READ        2
#define SYSCALL_WRITE       3
#define SYSCALL_GETPID      4
#define SYSCALL_SLEEP       5

// System call handler type
typedef long (*syscall_handler_t)(long arg0, long arg1, long arg2, long arg3);

// System call operations
void syscall_init(void);
long syscall_dispatch(uint32_t syscall_num, long arg0, long arg1, long arg2, long arg3);
int syscall_register(uint32_t num, syscall_handler_t handler);
```

**Architecture-Specific Syscall Entry**:
```c
// ARM64: Use SVC instruction
// x86-64: Use SYSCALL instruction

// src/arch/arm64/syscall.S - SVC handler
// src/arch/x86_64/syscall.S - SYSCALL handler
```

#### 7. Enhanced Kernel Integration (`src/kernel/`)

**Updated Kernel Main**:
```c
// src/kernel/main.c - Enhanced for Phase 4
void kernel_main(struct boot_info *boot_info) {
    // Phase 2: Basic boot info validation ✅
    // Phase 3: Memory management initialization ✅
    
    // Phase 4: System services initialization
    device_init(boot_info);          // Initialize device framework
    timer_init();                    // Initialize timer services
    interrupt_init();                // Initialize interrupt management
    process_init();                  // Initialize process management
    syscall_init();                  // Initialize system call interface
    
    // Start system services
    early_print("Starting system services...\n");
    
    // Create initial kernel tasks
    process_create(idle_task, "idle");
    process_create(init_task, "init");
    
    // Display system information
    show_system_info(boot_info);
    
    early_print("System services ready!\n");
    early_print("MiniOS is ready (Phase 4 - Device Drivers & System Services)\n");
    
    // Start scheduler
    scheduler_start();
    
    // Should never return
    arch_halt();
}
```

### Expected Build Process After Implementation

```bash
# Should work with enhanced functionality:
make clean
make ARCH=arm64        # Creates enhanced kernel with device drivers
make ARCH=x86_64       # Creates enhanced kernel with system services

# Should boot and show advanced features:
make test ARCH=arm64   # Shows device initialization, interrupt handling
make test ARCH=x86_64  # Shows timer services, process creation
```

### Success Criteria for Phase 4

#### Functional Requirements:
1. **Device Framework Active** - Device registration and management working
2. **Timer Services Running** - Scheduling timers operational on both architectures
3. **UART Communication** - Serial debugging output functional
4. **Interrupt Handling** - Hardware interrupts properly routed and handled
5. **Basic Process Management** - Task creation and simple scheduling working
6. **System Call Interface** - Basic syscalls functional for user-kernel communication

#### Quality Requirements:
1. **Clean Architecture** - Good separation between device drivers and kernel
2. **Comprehensive Testing** - Device driver and system service tests
3. **Documentation Updated** - Implementation and usage guides for Phase 4
4. **Performance** - Efficient interrupt handling and context switching

### Testing Strategy

#### Phase 4 Tests to Implement:
```bash
# Create: tools/test-phase4.sh
# Tests:
- Device driver framework functionality
- Timer service initialization and operation
- UART driver communication
- Interrupt controller setup and handling
- Basic process creation and scheduling
- System call interface validation
- Cross-platform compatibility
```

#### Integration Testing:
```bash
# Ensure all previous phases still work:
./tools/test-phase1.sh  # Should maintain 100% pass rate
./tools/test-phase2.sh  # Should maintain 100% pass rate  
./tools/test-phase3.sh  # Should maintain 100% pass rate

# Phase 4 should enhance, not break existing functionality
```

### Development Approach

#### Recommended Implementation Order:

1. **Device Driver Framework** (foundation)
   - Create device and driver interfaces
   - Implement device registration and management
   - Add device enumeration (device tree for ARM64, PCI for x86-64)

2. **Timer Services** (scheduling foundation)
   - ARM64 Generic Timer integration
   - x86-64 PIT/APIC timer support
   - Cross-platform timer abstraction

3. **UART/Serial Drivers** (debugging support)
   - ARM64 PL011 UART driver
   - x86-64 16550 UART driver
   - Enhanced early_print with driver backend

4. **Interrupt Management** (hardware events)
   - ARM64 GIC controller setup
   - x86-64 IDT and APIC configuration
   - Interrupt routing and handling

5. **Basic Process Management** (multitasking)
   - Task structure and creation
   - Simple round-robin scheduler
   - Context switching for both architectures

6. **System Call Interface** (user-kernel communication)
   - System call framework and dispatch
   - Architecture-specific entry points (SVC/SYSCALL)
   - Basic system calls (exit, print, sleep)

7. **Integration and Testing** (validation)
   - Cross-platform testing of all services
   - Performance testing and optimization
   - Documentation and usage guides

### Key Challenges

#### Device Management:
- **ARM64**: Device tree parsing and MMIO address mapping
- **x86-64**: PCI enumeration and I/O port management
- **Cross-platform**: Unified device interface despite different discovery methods

#### Interrupt Handling:
- **ARM64**: GIC controller complexity and IRQ routing
- **x86-64**: Legacy PIC vs modern APIC configuration
- **Cross-platform**: Common interrupt interface and handler registration

#### Process Management:
- **ARM64**: EL0/EL1 privilege switching and context preservation
- **x86-64**: Ring 3/Ring 0 transitions and segment setup
- **Cross-platform**: Common task structure and scheduling algorithm

#### System Calls:
- **ARM64**: SVC instruction handling and parameter passing
- **x86-64**: SYSCALL instruction setup and MSR configuration
- **Cross-platform**: Common syscall numbering and parameter conventions

### Branch Management

#### Current Status:
```bash
git status                       # Should be on phase-4-system-services
git log --oneline -3             # Should show v0.3.0 and Phase 3 completion
```

#### Feature branches for major components:
```bash
git checkout -b feature/device-framework
git checkout -b feature/timer-services  
git checkout -b feature/uart-drivers
git checkout -b feature/interrupt-management
git checkout -b feature/process-management
git checkout -b feature/syscall-interface
```

#### When Phase 4 is Complete:
```bash
git checkout develop
git merge phase-4-system-services --no-ff
git checkout main  
git merge develop --no-ff
git tag v0.4.0 -m "Phase 4 Complete: Device Drivers & System Services"
```

### Key Resources

#### Documentation to Reference:
1. **[docs/PHASE3_IMPLEMENTATION.md](docs/PHASE3_IMPLEMENTATION.md)** - Memory management foundation
2. **[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)** - System design and memory layout  
3. **[docs/BUILD.md](docs/BUILD.md)** - Build system usage and customization
4. **[PHASE_PROGRESS_TRACKER.md](PHASE_PROGRESS_TRACKER.md)** - Overall progress (43% complete)

#### External References:
- **ARM Architecture Reference Manual** - ARMv8-A exception handling, GIC, Generic Timer
- **Intel/AMD System Programming Guides** - x86-64 interrupt handling, APIC, system calls
- **Device Tree Specification** - ARM64 hardware description format
- **PCI Specification** - x86-64 device enumeration and configuration
- **POSIX System Call Interface** - Standard system call conventions

### Expected Timeline

- **Week 1**: Device driver framework and timer services
- **Week 2**: UART drivers and interrupt management  
- **Week 3**: Basic process management and context switching
- **Week 4**: System call interface and service integration
- **Days 5-7**: Integration testing, documentation, and validation

### Current System Status

#### Verified Working (Phase 3):
- ✅ **Memory Management**: ARM64 MMU and x86-64 paging fully functional
- ✅ **Physical Allocation**: Bitmap allocators operational on both architectures
- ✅ **Exception Framework**: ARM64 vectors and x86-64 handlers installed
- ✅ **Enhanced Kernel**: Memory initialization and testing complete
- ✅ **Build System**: Phase 3 enhancements fully integrated
- ✅ **Testing**: 29 Phase 3 tests + 20 Phase 2 tests + 32 Phase 1 tests all passing

#### Ready for Enhancement:
- 🎯 **Device Framework**: Memory management provides foundation for device drivers
- 🎯 **Interrupt Handling**: Exception framework ready for hardware interrupt routing
- 🎯 **Process Management**: Virtual memory infrastructure ready for task isolation
- 🎯 **System Services**: Exception handling ready for system call implementation

---

## Your Task

Implement **Phase 4: Device Drivers & System Services** for MiniOS:

1. **Create Phase 4 documentation structure** and initialize development environment
2. **Design device driver framework** with cross-platform device and driver interfaces
3. **Implement timer services** for ARM64 Generic Timer and x86-64 PIT/APIC
4. **Create UART/serial drivers** for debugging and communication on both architectures  
5. **Set up interrupt management** with GIC for ARM64 and IDT/APIC for x86-64
6. **Implement basic process management** with task creation and simple scheduling
7. **Add system call interface** with architecture-specific entry points
8. **Test comprehensively** with new Phase 4 test suite
9. **Document implementation** and update progress tracking

**Start by checking the current system status and understanding the Phase 3 foundation. The memory management and exception handling provide a solid base for implementing advanced system services!**

The foundation is excellent - Phase 3 provides everything needed for successful Phase 4 implementation! 🚀

---

## Quick Validation Commands

```bash
# Verify current status
git branch --show-current                   # Should show phase-4-system-services
git log --oneline -3                       # Should show v0.3.0 and Phase 3 completion  
./tools/test-phase3.sh                     # Should show 29/29 ✅ PASS
make clean && make ARCH=arm64 all          # Should build successfully with memory management
make clean && make ARCH=x86_64 all         # Should build successfully with memory management

# Check build artifacts  
ls -lh build/arm64/{kernel.elf,minios.img} # Should show 84KB kernel, 16MB image
ls -lh build/x86_64/{kernel.elf,minios.iso} # Should show 17KB kernel, 384KB ISO

# Review foundation
cat docs/PHASE3_IMPLEMENTATION.md          # Understand memory management foundation
cat src/include/{memory,exceptions}.h      # Review available interfaces
cat PHASE_PROGRESS_TRACKER.md              # Check overall project status (43%)
```
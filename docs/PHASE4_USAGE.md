# Phase 4 Usage Guide: Device Drivers & System Services

## Overview

Phase 4 builds upon the Phase 3 memory management foundation to implement core operating system services including device drivers, interrupt handling, process management, and system calls.

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

## Phase 4 Implementation Roadmap

### Week 1: Foundation Services
#### Device Driver Framework
- Create unified device and driver interfaces
- Implement device registration and management
- Add device enumeration (device tree for ARM64, PCI for x86-64)

#### Timer Services
- ARM64 Generic Timer integration for scheduling
- x86-64 PIT/APIC timer support for timing
- Cross-platform timer abstraction layer

### Week 2: Communication & Interrupts
#### UART/Serial Drivers
- ARM64 PL011 UART driver for debugging
- x86-64 16550 UART driver for communication
- Enhanced early_print with driver backend

#### Interrupt Management
- ARM64 GIC controller setup and configuration
- x86-64 IDT and APIC setup for interrupt handling
- Interrupt routing and handler registration

### Week 3: Multitasking
#### Process Management
- Task structure and creation framework
- Simple round-robin scheduler implementation
- Context switching for both architectures

### Week 4: User Interface
#### System Call Interface
- System call framework and dispatch mechanism
- Architecture-specific entry points (SVC/SYSCALL)
- Basic system calls (exit, print, read, write, sleep)

## Technical Architecture

### Device Driver Framework
```c
// Device and driver interfaces
struct device_driver {
    const char *name;
    int (*probe)(struct device *dev);
    int (*init)(struct device *dev);
    // ... additional operations
};

// Cross-platform device management
int driver_register(struct device_driver *driver);
struct device *device_create(const char *name, struct device_driver *driver);
```

### Timer Services
```c
// ARM64 Generic Timer
void arm64_timer_init(void);
uint64_t arm64_timer_get_count(void);
void arm64_timer_set_timeout(uint64_t ticks);

// x86-64 Timer Support  
void x86_64_pit_init(uint32_t frequency);
uint64_t x86_64_timer_get_ticks(void);
```

### Process Management
```c
// Task structure
struct task {
    uint32_t pid;
    uint32_t state;
    void *stack_base;
    struct cpu_context context;
    // ... additional fields
};

// Process operations
int process_create(void (*entry)(void), const char *name);
void process_yield(void);
void scheduler_tick(void);
```

## Build System Integration

### Enhanced Makefile Support
Phase 4 extends the build system to include:
```makefile
# Device driver sources
KERNEL_C_SOURCES += $(wildcard $(SRC_DIR)/drivers/*/*.c)

# System service sources  
KERNEL_C_SOURCES += $(wildcard $(SRC_DIR)/kernel/process/*.c)
KERNEL_C_SOURCES += $(wildcard $(SRC_DIR)/kernel/syscall/*.c)
```

### Directory Structure
```
src/
├── drivers/                    # Device drivers
│   ├── uart/                  # UART drivers
│   ├── timer/                 # Timer services
│   ├── interrupt/             # Interrupt controllers
│   └── pci/                   # PCI bus (x86-64)
├── kernel/                    # Enhanced kernel
│   ├── device.c              # Device management
│   ├── process/              # Process management
│   │   ├── scheduler.c       # Task scheduler
│   │   └── task.c           # Task management
│   └── syscall/             # System calls
│       ├── syscall.c        # Syscall framework
│       └── handlers.c       # Syscall handlers
└── arch/                     # Architecture-specific
    ├── arm64/               # ARM64 implementation
    │   ├── context.S        # Context switching
    │   └── syscall.S        # SVC handling
    └── x86_64/              # x86-64 implementation
        ├── context.S        # Context switching
        └── syscall.S        # SYSCALL handling
```

## Testing Framework

### Phase 4 Test Suite
Create comprehensive tests covering:
- Device driver registration and management
- Timer service initialization and operation
- UART driver communication
- Interrupt controller setup
- Process creation and basic scheduling
- System call interface validation

### Integration Testing
```bash
# Ensure all previous phases still work
./tools/test-phase1.sh         # Should maintain 32/32 PASS
./tools/test-phase2.sh         # Should maintain 20/20 PASS  
./tools/test-phase3.sh         # Should maintain 29/29 PASS
./tools/test-phase4.sh         # New comprehensive test suite
```

## Debugging Support

### Enhanced Serial Output
Phase 4 replaces the basic early_print with driver-based output:
```c
// Enhanced debugging with UART drivers
void debug_print(const char *format, ...);
void uart_debug_init(void);
```

### Process Information
```c
// Process debugging
void show_process_list(void);
void show_scheduler_state(void);
void dump_task_context(struct task *task);
```

### Interrupt Debugging
```c
// Interrupt debugging
void show_interrupt_stats(void);  
void dump_interrupt_controllers(void);
```

## Performance Characteristics

### Memory Usage
- **Device Drivers**: Minimal memory overhead with efficient allocation
- **Process Management**: Stack allocation per task (default 4KB)
- **Interrupt Handling**: Fast context switching with minimal overhead

### Timing Performance
- **Context Switch**: Target <100 microseconds on both architectures
- **Interrupt Latency**: Target <10 microseconds for hardware interrupts  
- **System Call**: Target <50 microseconds for basic syscalls

## Common Issues and Solutions

### Device Driver Issues
**Problem**: Device not detected during enumeration  
**Solution**: Check device tree parsing (ARM64) or PCI scanning (x86-64)

**Problem**: UART output not working  
**Solution**: Verify MMIO mapping (ARM64) or I/O port access (x86-64)

### Process Management Issues
**Problem**: Context switch corruption  
**Solution**: Verify register save/restore in architecture-specific code

**Problem**: Scheduler not preempting tasks  
**Solution**: Ensure timer interrupts are properly configured and enabled

### System Call Issues  
**Problem**: System calls not dispatching  
**Solution**: Verify SVC (ARM64) or SYSCALL (x86-64) instruction setup

## Integration with Previous Phases

### Memory Management Usage
Phase 4 builds on Phase 3 memory management:
- Use `memory_alloc()` for driver buffers and task stacks
- Use `memory_map()` for device MMIO access
- Use `memory_alloc_pages()` for large allocations

### Exception Framework Usage
Phase 4 extends Phase 3 exception handling:
- Register hardware interrupt handlers
- Use exception context for system calls
- Leverage context preservation for task switching

### Boot Protocol Integration
Phase 4 maintains compatibility with Phase 2 boot protocol:
- Use boot_info for device enumeration
- Preserve memory map information
- Maintain cross-platform abstractions

## Success Criteria

### Functional Requirements
- [x] Phase 4 branch created and documentation ready
- [ ] Device driver framework operational
- [ ] Timer services providing scheduling ticks
- [ ] UART communication functional
- [ ] Hardware interrupts properly handled
- [ ] Basic process creation and scheduling working
- [ ] System call interface operational

### Quality Requirements
- [ ] All previous tests maintain 100% pass rate (81 total)
- [ ] New Phase 4 test suite >90% pass rate
- [ ] Clean architecture with proper separation
- [ ] Comprehensive documentation updated
- [ ] Performance targets met for timing operations

### Build Targets
```bash
# Enhanced builds with system services
make ARCH=arm64 all           # Target: ~120KB kernel with drivers
make ARCH=x86_64 all          # Target: ~35KB kernel with services
```

## Next Steps After Phase 4

Phase 4 completion enables Phase 5: File System implementation:
- VFS (Virtual File System) layer using process management
- Basic file system implementation using device drivers
- File I/O operations using system call interface
- Storage device drivers using interrupt handling

**Phase 4 provides the essential system services foundation for a complete operating system!** 🚀
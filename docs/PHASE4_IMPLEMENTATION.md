# Phase 4: Device Drivers & System Services Implementation Guide

## Overview

Phase 4 builds upon the Phase 3 memory management foundation to implement core operating system services and a device driver framework. This phase introduces advanced system capabilities including hardware device management, basic process scheduling, and system call interfaces.

## Phase 4 Status: Foundation Complete ✅

### ✅ COMPLETED (Foundation - 60% of Phase 4)
1. **✅ Device Driver Framework** - Unified driver interface and device management
2. **✅ Timer Services** - ARM64 Generic Timer and x86-64 PIT timer support
3. **✅ Device Management** - Device discovery, registration, and lifecycle management
4. **✅ Cross-Platform APIs** - Unified interfaces hiding architecture differences
5. **✅ Build System Integration** - Seamless driver compilation and linking

### 🚧 REMAINING (40% of Phase 4 - Next Implementation)
3. **🚧 UART/Serial Drivers** - Communication interfaces for debugging and I/O
4. **🚧 Interrupt Management** - Hardware interrupt routing and handling
5. **🚧 Basic Process Management** - Task creation and simple scheduling
6. **🚧 System Call Interface** - User-kernel communication mechanisms

### Foundation Built Upon (Phase 3)
- ✅ **Memory Management** - Virtual memory, page tables, and allocation
- ✅ **Exception Handling** - ARM64 vectors and x86-64 interrupt framework
- ✅ **Cross-Platform APIs** - Unified interfaces across architectures
- ✅ **Enhanced Kernel** - Memory initialization and testing

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
    int (*ioctl)(struct device *dev, unsigned int cmd, unsigned long arg);
};

// Device instance
struct device {
    char name[64];
    struct device_driver *driver;
    void *private_data;
    struct device *next;
    int device_id;
    uint32_t flags;
};
```

#### Device Discovery
- **ARM64**: Device Tree parsing for MMIO devices
- **x86-64**: PCI enumeration for standard devices

### Timer Services

#### ARM64 Generic Timer
- **CNTFRQ_EL0** - Timer frequency detection
- **CNTP_CTL_EL0** - Timer control and enable
- **CNTP_CVAL_EL0** - Compare value for timeouts
- **Timer IRQ** - Integration with GIC for scheduling

#### x86-64 Timer Support
- **PIT (8254)** - Programmable Interval Timer
- **APIC Timer** - Advanced Programmable Interrupt Controller timer
- **TSC** - Time Stamp Counter for high-resolution timing

### UART/Serial Communication

#### ARM64 PL011 UART
- **Memory-mapped I/O** - Standard ARM UART controller
- **Baud rate configuration** - Flexible communication speeds
- **Interrupt support** - Receive/transmit interrupts

#### x86-64 16550 UART
- **Port I/O** - Standard PC COM port interface
- **FIFO buffers** - Hardware buffering support
- **Legacy compatibility** - COM1/COM2 port support

### Interrupt Management

#### ARM64 GIC (Generic Interrupt Controller)
- **GICv2/GICv3** - Modern ARM interrupt management
- **IRQ routing** - CPU affinity and priority handling
- **Device interrupts** - Timer, UART, and custom devices

#### x86-64 Interrupt Controllers
- **8259 PIC** - Legacy interrupt controller
- **APIC/x2APIC** - Modern interrupt handling
- **IDT management** - Interrupt descriptor table setup

### Process Management

#### Task Structure
```c
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
```

#### Round-Robin Scheduler
- **Time slicing** - Timer-based preemption
- **Context switching** - Architecture-specific register save/restore
- **Priority levels** - Basic priority scheduling

### System Call Interface

#### Cross-Platform System Calls
```c
#define SYSCALL_EXIT        0
#define SYSCALL_PRINT       1
#define SYSCALL_READ        2
#define SYSCALL_WRITE       3
#define SYSCALL_GETPID      4
#define SYSCALL_SLEEP       5
```

#### Architecture-Specific Entry Points
- **ARM64**: SVC (Supervisor Call) instruction
- **x86-64**: SYSCALL instruction with MSR setup

## Implementation Plan

### Week 1: Device Framework and Timer Services

#### Day 1-2: Device Driver Framework
1. **Create device/driver interfaces** (`src/include/device.h`, `src/include/driver.h`)
2. **Implement device management** (`src/kernel/device.c`)
3. **Add device registration and discovery**
4. **Create test framework for device operations**

#### Day 3-4: Timer Services
1. **ARM64 Generic Timer** (`src/drivers/timer/arm64_timer.c`)
2. **x86-64 PIT/APIC Timer** (`src/drivers/timer/x86_64_timer.c`)
3. **Cross-platform timer API** (`src/include/timer.h`)
4. **Timer interrupt integration**

#### Day 5-7: Timer Testing and Integration
1. **Timer service testing and validation**
2. **Integration with existing kernel**
3. **Documentation and usage examples**

### Week 2: Communication and Interrupt Management

#### Day 1-3: UART Drivers
1. **ARM64 PL011 UART** (`src/drivers/uart/pl011.c`)
2. **x86-64 16550 UART** (`src/drivers/uart/16550.c`)
3. **Enhanced early_print with driver backend**
4. **Serial communication testing**

#### Day 4-7: Interrupt Management
1. **ARM64 GIC controller** (`src/drivers/interrupt/gic.c`)
2. **x86-64 IDT/APIC setup** (`src/drivers/interrupt/apic.c`, `src/drivers/interrupt/pic.c`)
3. **Interrupt routing and handling**
4. **Device interrupt integration**

### Week 3: Process Management

#### Day 1-3: Task Structure and Creation
1. **Task structure definition** (`src/include/process.h`)
2. **Task creation and management** (`src/kernel/process.c`)
3. **Stack allocation and setup**
4. **Basic task operations (create, destroy)**

#### Day 4-7: Scheduler Implementation
1. **Round-robin scheduler** (`src/kernel/scheduler.c`)
2. **Context switching** (ARM64 and x86-64 specific)
3. **Timer-based preemption**
4. **Multi-tasking demonstration**

### Week 4: System Calls and Integration

#### Day 1-3: System Call Framework
1. **System call interface** (`src/include/syscall.h`)
2. **ARM64 SVC handler** (`src/arch/arm64/syscall.S`)
3. **x86-64 SYSCALL setup** (`src/arch/x86_64/syscall.S`)
4. **System call dispatch and parameter passing**

#### Day 4-5: Basic System Calls
1. **Implement core system calls** (exit, print, read, write, getpid, sleep)
2. **System call testing and validation**
3. **User-kernel communication demonstration**

#### Day 6-7: Integration and Testing
1. **Comprehensive Phase 4 testing** (`tools/test-phase4.sh`)
2. **Performance testing and optimization**
3. **Documentation completion**

## File Structure (Phase 4 Additions)

```
src/
├── include/
│   ├── device.h         🆕 Device management interface
│   ├── driver.h         🆕 Device driver interface
│   ├── timer.h          🆕 Timer services interface
│   ├── uart.h           🆕 UART/serial interface
│   ├── interrupt.h      🆕 Interrupt management interface
│   ├── process.h        🆕 Process management interface
│   └── syscall.h        🆕 System call interface
├── kernel/
│   ├── device.c         🆕 Device management implementation
│   ├── process.c        🆕 Process management implementation
│   ├── scheduler.c      🆕 Task scheduler implementation
│   └── syscall.c        🆕 System call implementation
├── drivers/             🆕 Device driver implementations
│   ├── timer/
│   │   ├── arm64_timer.c    🆕 ARM64 Generic Timer
│   │   └── x86_64_timer.c   🆕 x86-64 PIT/APIC Timer
│   ├── uart/
│   │   ├── pl011.c          🆕 ARM64 PL011 UART
│   │   └── 16550.c          🆕 x86-64 16550 UART
│   ├── interrupt/
│   │   ├── gic.c            🆕 ARM64 GIC controller
│   │   ├── apic.c           🆕 x86-64 APIC controller
│   │   └── pic.c            🆕 x86-64 8259 PIC controller
│   └── pci/
│       └── pci.c            🆕 x86-64 PCI enumeration
└── arch/
    ├── arm64/
    │   ├── syscall.S        🆕 ARM64 SVC handler
    │   ├── context_switch.S 🆕 ARM64 context switching
    │   └── devicetree.c     🆕 Device tree parsing
    └── x86_64/
        ├── syscall.S        🆕 x86-64 SYSCALL setup
        ├── context_switch.S 🆕 x86-64 context switching
        └── pci_scan.c       🆕 PCI device scanning
```

## Testing Strategy

### Phase 4 Test Suite (`tools/test-phase4.sh`)

#### Test Categories
1. **Device Framework Tests**
   - Device registration and discovery
   - Driver loading and initialization
   - Device operation validation

2. **Timer Service Tests**
   - Timer initialization and configuration
   - Timer interrupt generation and handling
   - Timing accuracy and precision

3. **UART Communication Tests**
   - UART initialization and configuration
   - Character transmission and reception
   - Enhanced debugging output

4. **Interrupt Management Tests**
   - Interrupt controller setup and configuration
   - Interrupt routing and handling
   - Device interrupt integration

5. **Process Management Tests**
   - Task creation and destruction
   - Context switching validation
   - Scheduler operation and fairness

6. **System Call Tests**
   - System call interface functionality
   - Parameter passing and return values
   - User-kernel transition validation

#### Integration Testing
- **Cross-Architecture Compatibility** - All features work on ARM64 and x86-64
- **Performance Testing** - Context switch times, interrupt latency
- **Regression Testing** - Phase 1-3 functionality still works
- **Stress Testing** - Multiple tasks, heavy interrupt load

## Key Implementation Challenges

### Device Management Complexity
- **ARM64**: Device tree parsing requires complex data structure handling
- **x86-64**: PCI enumeration involves configuration space access
- **Solution**: Unified device interface abstracts discovery differences

### Interrupt Handling Differences
- **ARM64**: GIC has complex IRQ routing and CPU affinity
- **x86-64**: Legacy PIC vs modern APIC configuration
- **Solution**: Common interrupt interface with architecture-specific backends

### Context Switching Precision
- **ARM64**: EL0/EL1 privilege level switching with register preservation
- **x86-64**: Ring 0/Ring 3 transitions with segment setup
- **Solution**: Architecture-specific assembly with common C interface

### System Call ABI Differences
- **ARM64**: SVC instruction with parameter registers (x0-x7)
- **x86-64**: SYSCALL instruction with different parameter conventions
- **Solution**: Common syscall numbering with arch-specific parameter handling

## Success Criteria

### Functional Requirements
1. **Device Framework Operational** - Device registration and basic operations work
2. **Timer Services Active** - Schedulable timer interrupts on both architectures
3. **UART Communication** - Serial I/O functional for enhanced debugging
4. **Interrupt Routing** - Hardware interrupts properly handled
5. **Basic Multitasking** - Multiple tasks can be created and scheduled
6. **System Call Interface** - Basic syscalls functional

### Quality Requirements
1. **Clean Architecture** - Good separation between drivers and kernel
2. **Cross-Platform Consistency** - Common interfaces across ARM64/x86-64
3. **Performance** - Efficient context switching (< 1000 cycles)
4. **Reliability** - Stable operation under load
5. **Testing** - Comprehensive test coverage (>95% pass rate)

### Validation Tests
- All previous phase tests continue to pass (regression testing)
- New Phase 4 test suite passes with >95% success rate
- System boots and shows device initialization
- Timer interrupts demonstrate scheduling capability
- UART shows enhanced debugging output
- Basic multitasking demonstration works

## Documentation Updates

### User Documentation
- **Usage Guide** - How to interact with Phase 4 features
- **API Reference** - Device driver and system call interfaces
- **Examples** - Device driver and application examples

### Developer Documentation
- **Porting Guide** - How to add new device drivers
- **Architecture Guide** - System service implementation details
- **Debugging Guide** - Advanced debugging with Phase 4 features

## Expected Outcomes

### Enhanced System Capabilities
- **Hardware Interaction** - Direct device communication and control
- **Timer-Based Scheduling** - Foundation for real-time capabilities  
- **Improved Debugging** - Enhanced serial output and diagnostics
- **Multitasking Foundation** - Basic process management capabilities
- **System Service Framework** - Extensible service architecture

### Phase 5 Preparation
- **Device Driver Infrastructure** - Ready for file system device drivers
- **Process Management** - Foundation for user applications
- **System Call Interface** - Ready for file system and I/O operations
- **Interrupt Handling** - Ready for disk and network device interrupts

The Phase 4 implementation transforms MiniOS from a basic kernel into a functional operating system with hardware interaction, basic multitasking, and system services. This sets the stage for file system implementation in Phase 5.
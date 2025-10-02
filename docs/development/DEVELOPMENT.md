# Development Guide

## Overview

This guide consolidates development practices, planning, and context preservation for MiniOS development.

## Development Plan

MiniOS follows a structured 7-phase development approach:

### Phase 1: Foundation Setup ✅
- Cross-platform build system
- Project structure and basic tooling
- Automated testing framework

### Phase 2: Bootloader Implementation ✅
- UEFI bootloader for ARM64
- Multiboot2 bootloader for x86-64
- Cross-platform boot abstraction

### Phase 3: Memory Management ✅
- Virtual memory implementation
- Page tables and MMU setup
- Memory allocation and management

### Phase 4: System Services ✅
- Device driver framework
- Interrupt handling
- Process management and scheduling
- System call interface

### Phase 5: File System ✅
- VFS (Virtual File System) layer
- SFS (Simple File System) implementation
- Block device abstraction

### Phase 6: User Interface 🔄
- Shell implementation
- Command-line utilities
- User applications

### Phase 7: Polish & Documentation ⏳
- Code cleanup and optimization
- Comprehensive documentation
- Final testing and validation

## Context Preservation System

### Session Management
Each development session should:
1. **Review Current Status** - Check what was completed in previous sessions
2. **Set Clear Goals** - Define specific objectives for the current session
3. **Document Progress** - Record achievements and challenges
4. **Plan Next Steps** - Outline what needs to be done in follow-up sessions

### Development Session Template
```
## Session: [Date] - [Topic]
### Previous Status
- What was completed in last session
- Any outstanding issues or blockers

### Session Goals
- [ ] Specific goal 1
- [ ] Specific goal 2
- [ ] Specific goal 3

### Progress Made
- Achievements during this session
- Code changes made
- Tests implemented/passed

### Issues Encountered
- Problems faced and solutions found
- Outstanding issues that need attention

### Next Steps
- [ ] Follow-up task 1
- [ ] Follow-up task 2
- [ ] Research/learning needed

### Files Modified
- List of files changed during session
```

## Development Workflow

### Code Development Process
1. **Understand Requirements** - Review phase documentation and specifications
2. **Design Solution** - Plan implementation approach
3. **Write Code** - Implement following project standards
4. **Test Thoroughly** - Run appropriate test suites
5. **Document Changes** - Update relevant documentation
6. **Commit Changes** - Follow git workflow guidelines

### Quality Standards
- **Cross-Platform Compatibility** - Code must work on both ARM64 and x86-64
- **Clean Architecture** - Follow established HAL patterns
- **Comprehensive Testing** - Include tests for new features
- **Clear Documentation** - Update docs for API changes
- **Consistent Style** - Follow existing code style and conventions

### Testing Strategy
- **Unit Tests** - Test individual components
- **Integration Tests** - Test component interactions
- **Phase Tests** - Comprehensive phase validation
- **Cross-Platform Tests** - Verify behavior on both architectures

## Architecture Guidelines

### Hardware Abstraction Layer (HAL)
MiniOS uses a clean HAL to separate architecture-specific code from platform-independent functionality:

```
src/
├── arch/                 # Architecture-specific code
│   ├── arm64/           # ARM64 implementation
│   └── x86_64/          # x86-64 implementation
├── kernel/              # Architecture-independent kernel
├── drivers/             # Device drivers with HAL interfaces
└── common/              # Shared utilities and data structures
```

### Cross-Platform APIs
All platform-independent code should use unified APIs that abstract architecture differences:

```c
// Example: Timer abstraction
struct timer_ops {
    void (*init)(void);
    uint64_t (*get_frequency)(void);
    void (*set_compare)(uint64_t compare);
    void (*enable)(void);
};

// Architecture-specific implementations
extern struct timer_ops arm64_timer_ops;
extern struct timer_ops x86_64_timer_ops;
```

## Build System Integration

### Cross-Compilation Support
The build system automatically handles:
- Architecture detection
- Toolchain selection
- Compiler flags and options
- Linker script selection

### Makefile Structure
```makefile
# Architecture detection
ARCH ?= arm64
ifeq ($(ARCH),arm64)
    CROSS_PREFIX = aarch64-elf-
    LINKER_SCRIPT = src/arch/arm64/linker.ld
else ifeq ($(ARCH),x86_64)
    CROSS_PREFIX = x86_64-elf-
    LINKER_SCRIPT = src/arch/x86_64/linker.ld
endif
```

## Debugging and Troubleshooting

### Debug Builds
Use debug builds for development:
```bash
make DEBUG=1
```

Debug builds include:
- Debug symbols (-g)
- No optimization (-O0)
- Debug logging enabled (-DDEBUG)
- Verbose error messages

### GDB Debugging
For kernel debugging:
```bash
make debug
```

Common debugging breakpoints:
```gdb
break main              # Kernel entry point
break trap_handler      # Exception/interrupt handler
break schedule          # Process scheduler
break syscall_handler   # System call entry
```

### Common Issues
- **Build Failures** - Check toolchain installation and PATH
- **Linking Errors** - Verify linker scripts and object files
- **Runtime Crashes** - Use debug builds and GDB
- **Cross-Platform Issues** - Test on both architectures

## Documentation Standards

### Code Documentation
- **Function Headers** - Document purpose, parameters, return values
- **Complex Algorithms** - Explain logic and design decisions
- **Architecture-Specific Code** - Document differences and requirements

### Update Documentation
When making changes:
1. **API Changes** - Update relevant reference documentation
2. **New Features** - Add to appropriate phase documentation
3. **Architecture Changes** - Update architecture documentation
4. **Build Changes** - Update build instructions

## Collaboration Guidelines

### Git Workflow
- **Feature Branches** - Create branches for new features
- **Clear Commits** - Write descriptive commit messages
- **Regular Merges** - Keep main branch up to date
- **Code Reviews** - Review changes before merging

### Session Handoffs
When ending a development session:
1. **Summarize Progress** - Document what was accomplished
2. **Note Issues** - Record any problems encountered
3. **Plan Next Steps** - Outline what should be done next
4. **Clean Up** - Commit changes and clean workspace

## Performance Considerations

### Memory Management
- **Efficient Allocation** - Use appropriate allocation strategies
- **Memory Leaks** - Track and prevent memory leaks
- **Page Table Optimization** - Optimize page table usage

### CPU Performance
- **Interrupt Handling** - Minimize interrupt latency
- **Context Switching** - Optimize process switching overhead
- **System Calls** - Efficient system call implementation

## Security Considerations

### Memory Protection
- **User/Kernel Separation** - Enforce memory boundaries
- **Input Validation** - Validate all user inputs
- **Buffer Overflows** - Prevent buffer overflow vulnerabilities

### System Integrity
- **Privilege Levels** - Use appropriate privilege levels
- **Access Control** - Implement proper access controls
- **Error Handling** - Secure error handling practices

---

This development guide provides the framework for consistent, high-quality development of MiniOS. Following these practices ensures maintainable code and successful project completion.
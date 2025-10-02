# MiniOS Documentation

## Overview

MiniOS is an educational operating system project supporting ARM64 and x86-64 architectures. This documentation is organized to help you understand, build, and contribute to the project effectively.

## Quick Start

1. **[Getting Started](GETTING_STARTED.md)** - Prerequisites, setup, and first build
2. **[User Guide](reference/USER_GUIDE.md)** - Using MiniOS and available features
3. **[Phase 1](phases/PHASE1.md)** - Foundation setup and build system

## Documentation Structure

### 📚 Core Documentation
- **[Getting Started](GETTING_STARTED.md)** - Complete setup guide for new developers
- **[Architecture](architecture/ARCHITECTURE.md)** - System design and architectural patterns
- **[Build Guide](development/BUILD.md)** - Detailed build instructions and troubleshooting

### 🏗️ Development Phases
The project follows a 7-phase development approach. Each phase builds upon the previous one:

- **[Phase 1: Foundation Setup](phases/PHASE1.md)** - Cross-platform build system
- **[Phase 2: Bootloader](phases/PHASE2.md)** - UEFI (ARM64) and Multiboot2 (x86-64)
- **[Phase 3: Memory Management](phases/PHASE3.md)** - Virtual memory and paging
- **[Phase 4: System Services](phases/PHASE4.md)** - Device drivers, processes, syscalls
- **[Phase 5: File System](phases/PHASE5.md)** - VFS layer and SFS implementation
- **[Phase 6: User Interface](phases/PHASE6.md)** - Shell and CLI utilities
- **[Phase 7: Polish & Documentation](phases/PHASE7.md)** - Final touches and cleanup

### 🏛️ Architecture & Design
- **[System Architecture](architecture/ARCHITECTURE.md)** - Overall system design
- **[File System Architecture](architecture/FILE_SYSTEM_ARCH.md)** - VFS and storage design
- **[Build System](architecture/BUILD_SYSTEM.md)** - Cross-platform build architecture

### 🔧 Development Resources
- **[Build Instructions](development/BUILD.md)** - Build system usage and commands
- **[Development Workflow](development/DEVELOPMENT.md)** - Development practices and guidelines
- **[Git Workflow](development/GIT_WORKFLOW.md)** - Branching strategy and contribution process

### 📖 Reference Materials
- **[User Guide](reference/USER_GUIDE.md)** - Complete usage guide
- **[File Creation Features](reference/FILE_CREATION.md)** - File system operations
- **[FS Quick Reference](reference/FS_QUICK_REFERENCE.md)** - File system API reference

### 📋 Historical & Planning
- **[Roadmap](archive/ROADMAP.md)** - Project development roadmap
- **[Session Notes](archive/SESSION_NOTES.md)** - Development session summaries
- **[Status Updates](archive/STATUS_UPDATES.md)** - Historical progress reports

## Current Status

**Phase 5 Complete - File System Implementation** ✅

### Completed Phases
- ✅ **Phase 1**: Foundation Setup - Cross-platform build system
- ✅ **Phase 2**: Bootloader Implementation - UEFI and Multiboot2 support
- ✅ **Phase 3**: Memory Management - Virtual memory with MMU/paging
- ✅ **Phase 4**: System Services - Device drivers, processes, interrupts
- ✅ **Phase 5**: File System - VFS layer, SFS implementation, block devices

### In Progress
- 🔄 **Phase 6**: User Interface - Shell implementation and CLI utilities

### Planned
- ⏳ **Phase 7**: Polish & Documentation - Final cleanup and documentation

## Build Commands

```bash
# Basic operations
make                    # Build for ARM64 (default)
make ARCH=x86_64        # Build for x86-64
make clean              # Clean build artifacts
make test               # Build and test in VM
make debug              # Start debug session

# Debug builds
make DEBUG=1            # Build with debug symbols and logging
make quick-test         # Quick build and test

# Information
make info               # Show build configuration
make help               # Show available commands
```

## Architecture Support

### Primary Target: ARM64
- **Platform**: UTM Virtual Machine on macOS
- **Toolchain**: aarch64-elf-gcc, aarch64-elf-ld, aarch64-elf-as
- **Bootloader**: UEFI
- **Image Format**: .img disk image

### Secondary Target: x86-64
- **Platform**: QEMU on multiple host OS
- **Toolchain**: x86_64-elf-gcc, x86_64-elf-ld, nasm
- **Bootloader**: Multiboot2
- **Image Format**: .iso disk image

## Key Features

### Cross-Platform Design
- **Hardware Abstraction Layer (HAL)** - Clean architecture separation
- **Unified APIs** - Same interfaces across ARM64 and x86-64
- **Architecture-Specific Optimizations** - Performance tuned per platform

### Modern OS Features
- **Virtual Memory** - 4KB pages with 4-level page tables
- **Preemptive Multitasking** - Round-robin scheduler with priorities
- **Device Driver Framework** - Modular driver architecture
- **File System** - VFS abstraction with SFS implementation
- **System Calls** - ARM64 SVC and x86-64 SYSCALL interfaces

### Development Tools
- **Comprehensive Testing** - Automated test suites for each phase
- **Debug Support** - GDB integration with architecture-specific debuggers
- **Cross-Compilation** - Full cross-compilation from macOS/Linux
- **VM Integration** - Automated VM testing and deployment

## Testing

### Phase-Specific Testing
```bash
./tools/test-phase1.sh      # Phase 1 foundation testing
./tools/test-phase3.sh      # Phase 3 memory testing
./tools/test-phase4.sh      # Phase 4 system services testing
```

### Comprehensive Testing
```bash
./test_phases.sh            # Test all completed phases
./test_all_phases.sh        # Comprehensive testing
./demo_phases_12.sh         # Demo phases 1-2
```

## Contributing

### Development Workflow
1. **Set up environment** - Follow [Getting Started](GETTING_STARTED.md)
2. **Choose a phase** - Work on current development phase
3. **Test thoroughly** - Run phase-specific test suites
4. **Document changes** - Update relevant documentation
5. **Submit changes** - Follow [Git Workflow](development/GIT_WORKFLOW.md)

### Code Standards
- **Cross-platform compatibility** - Code must work on both ARM64 and x86-64
- **Clean architecture** - Follow established HAL patterns
- **Comprehensive testing** - Include tests for new features
- **Documentation** - Update relevant docs for API changes

## Help and Support

### Common Issues
- **Build failures** - Check [Build Guide](development/BUILD.md) troubleshooting
- **VM issues** - Verify QEMU/UTM installation and configuration
- **Toolchain problems** - Ensure cross-compilers are in PATH

### Getting Help
- **Check phase documentation** - Each phase has detailed guides
- **Review test outputs** - Test suites provide diagnostic information
- **Examine build logs** - Make builds provide verbose error messages

## File Organization

This documentation is organized to help you find information quickly:

- **Getting Started** - New developer setup and orientation
- **Phases** - Sequential development documentation
- **Architecture** - System design and technical details
- **Development** - Build system and development practices
- **Reference** - API documentation and usage guides
- **Archive** - Historical planning and session notes

## LLM Configuration Files

For AI assistants working on this project:
- **[CLAUDE.md](../CLAUDE.md)** - Claude Code configuration
- **[.llm-config/](../.llm-config/)** - Additional LLM configuration files

These files contain project-specific instructions and context for AI assistants.

---

**MiniOS** - Educational operating system demonstrating core OS concepts with modern, clean architecture and comprehensive documentation.
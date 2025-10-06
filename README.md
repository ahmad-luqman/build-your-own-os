# MiniOS - A Learning Operating System

A minimal operating system project designed for educational purposes, supporting ARM64 and x86-64 architectures.

## 🚀 Quick Start

```bash
git clone https://github.com/ahmadluqman/build-your-own-os.git
cd build-your-own-os

# Check prerequisites and setup
./tools/check-prerequisites.sh

# Build and test
make clean all
make test
```

## 📖 Documentation

**Start here:** [docs/README.md](docs/README.md) - Complete documentation and guides

**New developers:** [docs/GETTING_STARTED.md](docs/GETTING_STARTED.md) - Setup and first build

## Current Status

🎉 **Phase 6 In Progress - Shell Implementation**

✅ **Phase 5 Complete** - File system with SFS production ready
✅ **Phase 6 Partial** - Shell with tab completion, pipes, I/O redirection
✅ **75% Complete** (5.5 of 7 phases)
✅ **Cross-platform** ARM64 (UTM) and x86-64 (QEMU) support

## Architecture Support

- **Primary Target**: ARM64 (AArch64) for UTM Virtual Machine on macOS
- **Secondary Target**: x86-64 for broader compatibility
- Cross-platform build system using abstraction layers

## Features (Implemented)

- [x] **Phase 1**: Foundation Setup - Cross-platform build system and development tools
- [x] **Phase 2**: Bootloader Implementation - UEFI (ARM64) and Multiboot2 (x86-64)
- [x] **Phase 3**: Memory Management - MMU/Paging with virtual memory and allocation
- [x] **Phase 4**: System Services - Device drivers, processes, interrupts, system calls
- [x] **Phase 5**: File System - VFS layer, SFS implementation, block devices, file I/O, persistence
- [x] **Phase 6** *(75% complete)*: Shell with tab completion, pipes, I/O redirection, 25+ commands
- [ ] **Phase 7**: Polish & Documentation - Final optimizations and comprehensive guides

### Recent Achievements (October 2025)
- ✅ **Memory Management Audit** - Complete leak detection and cache management (Issue #12)
- ✅ **Pipe Execution** - Full pipe support with `|` operator (Issue #16)
- ✅ **Tab Completion** - Fixed cursor positioning bug (Issue #1)
- ✅ **SFS File Operations** - 100% feature parity with RAMFS (Issues #4-#10, #15)
- ✅ **Shell Commands** - 25+ built-in commands including meminfo, mount, umount

## Repository Structure

```
build-your-own-os/
├── docs/                   # Documentation
├── src/                    # Source code
│   ├── boot/              # Bootloader
│   ├── kernel/            # Kernel code
│   ├── arch/              # Architecture-specific code
│   │   ├── arm64/         # ARM64 specific
│   │   └── x86_64/        # x86-64 specific
│   ├── drivers/           # Device drivers
│   ├── fs/                # File system
│   └── userland/          # User applications
├── tools/                  # Build tools and official test scripts
├── scripts/                # Development and testing scripts
│   └── testing/           # Testing scripts (see scripts/testing/README.md)
├── tests/                  # Testing framework
├── tmp/                    # Temporary files and test logs (gitignored)
└── vm-configs/            # Virtual machine configurations
```

## Getting Started

### Quick Start (Phase 1)
```bash
# Check Phase 1 status
make info
./tools/test-phase1.sh

# Install prerequisites
./tools/check-prerequisites.sh

# Build and test
make ARCH=arm64
make test
```

### Quick Links
- 📋 **[Project Status](CURRENT_STATUS.md)** - Current progress and achievements
- 🗺️ **[Roadmap](ROADMAP.md)** - Future plans and vision
- 🔧 **[Known Issues](docs/reference/KNOWN_ISSUES.md)** - Open issues and limitations
- ✅ **[Completed Features](docs/reference/COMPLETED_FEATURES.md)** - All working features
- 📊 **[GitHub Project Board](https://github.com/users/ahmad-luqman/projects/9)** - Issue tracking
- 🤖 **[CLAUDE.md](CLAUDE.md)** - AI assistant development guide

## Documentation

### Essential Documents
- **[ROADMAP.md](ROADMAP.md)** - Development roadmap and future plans
- **[CURRENT_STATUS.md](CURRENT_STATUS.md)** - Current project status and progress
- **[CLAUDE.md](CLAUDE.md)** - AI assistant guidance for development

### Getting Started
- [Getting Started Guide](docs/GETTING_STARTED.md) - Setup and first build
- [Prerequisites](docs/PREREQUISITES.md) - Environment setup requirements
- [VM Setup](docs/VM_SETUP.md) - Virtual machine configuration (UTM/QEMU)

### User Documentation
- [User Guide](docs/reference/USER_GUIDE.md) - How to use MiniOS shell and commands
- [File System Quick Reference](docs/reference/FS_QUICK_REFERENCE.md) - File system operations
- [Running MiniOS Guide](docs/reference/RUNNING_MINIOS_GUIDE.md) - Boot and run instructions

### Reference Documentation
- [Completed Features](docs/reference/COMPLETED_FEATURES.md) - Comprehensive list of all working features
- [Known Issues](docs/reference/KNOWN_ISSUES.md) - Current issues and limitations
- [SFS Status](docs/reference/SFS_STATUS.md) - File system capabilities and testing results

### Developer Documentation
- [Architecture](docs/architecture/ARCHITECTURE.md) - System design and architecture
- [File System Architecture](docs/architecture/FILE_SYSTEM_ARCHITECTURE.md) - FS layer details
- [Build System](docs/development/BUILD.md) - Build instructions and customization
- [Development Guide](docs/development/DEVELOPMENT.md) - Development workflow
- [Git Workflow](docs/development/GIT_WORKFLOW.md) - Branching and commit strategy

### Phase Documentation
- [Phase 1: Foundation](docs/phases/PHASE1.md) - Build system and tooling ✅
- [Phase 2: Bootloader](docs/phases/PHASE2.md) - Boot implementation ✅
- [Phase 3: Memory Management](docs/phases/PHASE3.md) - MMU and paging ✅
- [Phase 4: System Services](docs/phases/PHASE4.md) - Drivers and processes ✅
- [Phase 5: File System](docs/phases/PHASE5.md) - VFS and SFS ✅
- [Phase 6: Shell](docs/phases/PHASE6.md) - User interface 🔄 (75%)
- [Phase 7: Polish](docs/phases/PHASE7.md) - Final optimizations ⏳

### Testing Documentation
- [Testing Scripts README](scripts/testing/README.md) - Test suite reference

## Contributing

This is an educational project. Feel free to follow along and modify as needed for your learning goals.

## License

This project is for educational purposes. See [LICENSE](LICENSE) for details.
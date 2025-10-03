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

🎉 **Phase 5 Complete - File System Implementation**

✅ **5 of 7 phases complete** (71% progress)
✅ **Complete operating system** with file system support
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
- [x] **Phase 5**: File System - VFS layer, SFS implementation, block devices, file I/O
- [ ] **Phase 6**: User Interface - Shell implementation and CLI utilities  
- [ ] **Phase 7**: Polish & Documentation - Final optimizations and comprehensive guides

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

### Complete Documentation
1. **Phase 1 Usage**: See [PHASE1_USAGE.md](docs/PHASE1_USAGE.md) - How to run current implementation
2. **Prerequisites**: See [PREREQUISITES.md](docs/PREREQUISITES.md) - Environment setup
3. **Build Instructions**: See [BUILD.md](docs/BUILD.md) - Detailed build system
4. **Development Plan**: See [DEVELOPMENT_PLAN.md](docs/DEVELOPMENT_PLAN.md) - Complete roadmap
5. **VM Setup**: See [VM_SETUP.md](docs/VM_SETUP.md) - Virtual machine configuration
6. **Testing Guide**: See [PHASE1_TESTING.md](docs/PHASE1_TESTING.md) - Validation procedures
7. **Testing Scripts**: See [scripts/testing/README.md](scripts/testing/README.md) - Test script reference

## Documentation

- [Development Plan](docs/DEVELOPMENT_PLAN.md) - Complete roadmap and implementation steps
- [Architecture Design](docs/ARCHITECTURE.md) - System architecture and design decisions
- [Build System](docs/BUILD.md) - How to build the OS
- [VM Configuration](docs/VM_SETUP.md) - Setting up UTM and other VMs
- [API Reference](docs/API.md) - Kernel and system call documentation

## Contributing

This is an educational project. Feel free to follow along and modify as needed for your learning goals.

## License

This project is for educational purposes. See [LICENSE](LICENSE) for details.
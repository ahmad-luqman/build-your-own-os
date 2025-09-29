# MiniOS - A Learning Operating System

A minimal operating system project designed for educational purposes, supporting ARM64 and x86-64 architectures.

## Project Overview

This project aims to build a minimal operating system from scratch that can:
- Boot on ARM64 (for UTM on macOS) and x86-64 architectures
- Provide a basic shell interface
- Run simple CLI applications
- Serve as a learning platform for OS development concepts

## Current Status

🚧 **Project is in planning phase**

## Architecture Support

- **Primary Target**: ARM64 (AArch64) for UTM Virtual Machine on macOS
- **Secondary Target**: x86-64 for broader compatibility
- Cross-platform build system using abstraction layers

## Features (Planned)

- [x] Project planning and documentation
- [ ] Bootloader (UEFI and Legacy BIOS support)
- [ ] Kernel initialization
- [ ] Memory management
- [ ] Process management
- [ ] File system (simple FAT32)
- [ ] Device drivers (keyboard, display)
- [ ] Shell implementation
- [ ] Basic CLI utilities

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
├── tools/                  # Build tools and scripts
├── tests/                  # Testing framework
└── vm-configs/            # Virtual machine configurations
```

## Getting Started

1. **Prerequisites**: See [PREREQUISITES.md](docs/PREREQUISITES.md)
2. **Build Instructions**: See [BUILD.md](docs/BUILD.md)
3. **Development Plan**: See [DEVELOPMENT_PLAN.md](docs/DEVELOPMENT_PLAN.md)
4. **VM Setup**: See [VM_SETUP.md](docs/VM_SETUP.md)

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
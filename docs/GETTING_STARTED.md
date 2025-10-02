# Getting Started with MiniOS

## Overview

MiniOS is an educational operating system supporting ARM64 and x86-64 architectures. This guide will help you set up your development environment and build your first MiniOS kernel.

## Prerequisites

### Required Software

#### Build Tools
- **Make** - Build system tool
- **Git** - Version control
- **Python 3** - For image creation scripts

#### Cross-Compilation Toolchains
- **ARM64**: `aarch64-elf-gcc`, `aarch64-elf-ld`, `aarch64-elf-as`
- **x86-64**: `x86_64-elf-gcc`, `x86_64-elf-ld`, `nasm`

#### Virtualization
- **QEMU** - For testing x86-64 builds
- **UTM** (macOS) - For testing ARM64 builds

### Installation

#### macOS (Recommended)
```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install Homebrew (if not already installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install required packages
brew install make git python3 qemu nasm cdrtools

# Install cross-compilers (if available)
brew install aarch64-elf-gcc x86_64-elf-gcc
```

#### Linux
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install build-essential git python3 qemu-system-x86 nasm xorriso

# Cross-compilers (may require building from source)
# See detailed installation instructions in development/BUILD.md
```

### Quick Environment Check

Run the automated prerequisite checker:
```bash
./tools/check-prerequisites.sh
```

This script will:
- Check for all required tools
- Report what's installed and what's missing
- Offer to auto-install missing packages on macOS

## Project Setup

### 1. Clone the Repository
```bash
git clone https://github.com/your-repo/build-your-own-os.git
cd build-your-own-os
```

### 2. Verify Project Structure
```bash
ls -la
# Expected output:
# Makefile  src/  docs/  tools/  vm-configs/  CLAUDE.md
```

### 3. Test the Build System
```bash
# Show build information
make info

# Show available commands
make help

# Test file permissions
ls -la tools/*.sh vm-configs/*.sh
# All scripts should be executable (-rwxr-xr-x)
```

## Your First Build

### Without Cross-Compilers (Test Mode)
Even without cross-compilers installed, you can test the build system:

```bash
# Test ARM64 configuration
make info ARCH=arm64

# Test x86-64 configuration
make info ARCH=x86_64

# Test debug build configuration
make info DEBUG=1
```

### With Cross-Compilers (Full Build)
If you have the complete toolchain installed:

```bash
# Build for ARM64 (default)
make

# Build for x86-64
make ARCH=x86_64

# Debug build with symbols
make DEBUG=1

# Clean and rebuild
make clean all
```

Expected build output:
```
CC      src/arch/arm64/boot/start.S
CC      src/arch/arm64/kernel/trap.S
CC      src/kernel/main.c
LD      build/arm64/kernel.elf
OBJCOPY build/arm64/kernel.bin
CREATE  build/arm64/minios.img

Build complete: build/arm64/minios.img
```

## Testing Your Build

### Automated Testing
```bash
# Run Phase 1 tests (build system validation)
./tools/test-phase1.sh

# Expected output: 30+ tests passing
# This validates the build system without requiring a full toolchain
```

### Virtual Machine Testing
If you have QEMU installed and a successful build:

```bash
# Test in VM (requires successful build)
make test

# Test with longer timeout
make test TIMEOUT=60

# Debug session
make debug
```

## Development Workflow

### Basic Commands
```bash
# Build and test cycle
make clean all     # Clean build
make test          # Test in VM

# Debug build and testing
make DEBUG=1 clean all
make debug

# Information gathering
make info          # Show build configuration
make help          # Show all commands
```

### Architecture Switching
```bash
# Switch between architectures
make ARCH=arm64    # ARM64 (default)
make ARCH=x86_64   # x86-64

# Clean when switching architectures
make clean
make ARCH=x86_64 all
```

## Project Structure

### Source Code Organization
```
src/
├── arch/                 # Architecture-specific code
│   ├── arm64/           # ARM64 implementation
│   └── x86_64/          # x86-64 implementation
├── boot/                # Bootloader code
├── kernel/              # Core kernel functionality
├── drivers/             # Device drivers
├── fs/                  # File system implementation
├── shell/               # Shell and user interface
└── userland/            # User space applications
```

### Build Outputs
```
build/
├── arm64/              # ARM64 build artifacts
│   ├── kernel.elf      # Kernel executable
│   ├── kernel.bin      # Binary kernel
│   └── minios.img      # Bootable disk image
└── x86_64/             # x86-64 build artifacts
    ├── kernel.elf      # Kernel executable
    ├── kernel.bin      # Binary kernel
    └── minios.iso      # Bootable ISO image
```

## Common Issues and Solutions

### "Command not found: make"
```bash
# macOS
xcode-select --install

# Linux
sudo apt install build-essential
```

### "Permission denied" for scripts
```bash
# Fix script permissions
chmod +x tools/*.sh tools/*.py vm-configs/*.sh
```

### "aarch64-elf-gcc: command not found"
```bash
# Expected without cross-compilers
# Install via Homebrew (macOS)
brew install aarch64-elf-gcc

# Or build from source (advanced)
# See development/BUILD.md for detailed instructions
```

### QEMU not found
```bash
# macOS
brew install qemu

# Linux Ubuntu/Debian
sudo apt install qemu-system-x86 qemu-system-arm
```

### Build fails with linking errors
```bash
# Check for missing linker scripts
ls -la src/arch/*/linker.ld

# Clean and retry
make clean
make ARCH=arm64
```

## Next Steps

### 1. Complete Environment Setup
- ✅ Install all prerequisites
- ✅ Verify build system works
- ✅ Test basic build commands

### 2. Explore the Project
- Read [Architecture Overview](architecture/ARCHITECTURE.md)
- Review current [Phase Status](phases/)
- Check [User Guide](reference/USER_GUIDE.md)

### 3. Start Development
- Work on current development phase
- Run phase-specific tests
- Follow [Git Workflow](development/GIT_WORKFLOW.md)

### 4. Learn the Build System
- Study [Build Guide](development/BUILD.md)
- Understand [cross-platform support](architecture/BUILD_SYSTEM.md)
- Review [development practices](development/DEVELOPMENT.md)

## Architecture-Specific Notes

### ARM64 Development
- **Primary Target**: UTM Virtual Machine on macOS
- **Toolchain**: aarch64-elf-gcc (Homebrew)
- **Image Format**: .img disk image
- **Testing**: UTM or QEMU system-aarch64

### x86-64 Development
- **Secondary Target**: QEMU on multiple platforms
- **Toolchain**: x86_64-elf-gcc + nasm
- **Image Format**: .iso image
- **Testing**: QEMU system-x86_64

### Cross-Platform Development
MiniOS uses a clean Hardware Abstraction Layer (HAL) that allows:
- Same source code across architectures
- Architecture-specific optimizations
- Unified APIs for core functionality
- Consistent behavior across platforms

## Resources

### Documentation
- **[Phase Documentation](phases/)** - Sequential development guides
- **[Architecture Docs](architecture/)** - System design and patterns
- **[Development Guides](development/)** - Build system and workflows
- **[Reference Materials](reference/)** - API documentation

### Tools and Scripts
- **[tools/check-prerequisites.sh](../tools/check-prerequisites.sh)** - Environment validation
- **[tools/test-phase1.sh](../tools/test-phase1.sh)** - Build system testing
- **[tools/debug.sh](../tools/debug.sh)** - Debug session setup

### Configuration Files
- **[CLAUDE.md](../CLAUDE.md)** - Claude Code AI assistant configuration
- **[Makefile](../Makefile)** - Main build configuration
- **[.gitignore](../.gitignore)** - Version control ignore patterns

## Getting Help

### Troubleshooting Steps
1. **Run prerequisite checker** - `./tools/check-prerequisites.sh`
2. **Check build logs** - Look for specific error messages
3. **Consult phase docs** - Each phase has troubleshooting sections
4. **Review build guide** - Detailed troubleshooting in development/BUILD.md

### Community and Support
- Check project issues and discussions
- Review test outputs for diagnostic information
- Examine build logs for specific error patterns

---

Welcome to MiniOS! This educational OS project demonstrates modern operating system concepts with clean, cross-platform architecture. Happy hacking!
# Prerequisites for MiniOS Development

## System Requirements

### macOS Development Environment
- **macOS version**: 10.15+ (Catalina or later)
- **Xcode Command Line Tools**: Required for basic development tools
- **Homebrew**: Package manager for installing development dependencies
- **UTM**: Virtual machine software for ARM64 testing

### Hardware Requirements
- **RAM**: Minimum 8GB (16GB recommended for comfortable VM usage)
- **Storage**: At least 10GB free space for tools and builds
- **Architecture**: Intel or Apple Silicon Mac (both supported)

## Required Tools

### 1. Basic Development Tools
```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install Homebrew (if not already installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

### 2. Cross-Compilation Toolchain

#### Option A: Pre-built Toolchains (Recommended)
```bash
# Install ARM64 cross-compiler
brew install aarch64-elf-gcc

# Install x86-64 cross-compiler  
brew install x86_64-elf-gcc

# Alternative: Use universal toolchain
brew install gcc-cross-gdb
```

#### Option B: Build from Source (Advanced)
If pre-built toolchains aren't available, see [TOOLCHAIN_BUILD.md](TOOLCHAIN_BUILD.md)

### 3. Assembly Tools
```bash
# NASM assembler for x86-64
brew install nasm

# GNU Assembler (usually comes with gcc)
# Verify with: as --version
```

### 4. Build Tools
```bash
# GNU Make
brew install make

# Python 3 (for build scripts)
brew install python3

# CMake (alternative build system)
brew install cmake

# Ninja (fast build system)
brew install ninja
```

### 5. Virtual Machine Software

#### UTM (Primary for ARM64)
```bash
# Install UTM via Homebrew
brew install --cask utm

# Or download from: https://mac.getutm.app/
```

#### QEMU (Cross-platform testing)
```bash
# Install QEMU for multiple architectures
brew install qemu

# Verify ARM64 support
qemu-system-aarch64 --version

# Verify x86-64 support  
qemu-system-x86_64 --version
```

### 6. Debugging and Analysis Tools
```bash
# GDB for debugging
brew install gdb

# Objdump for binary analysis
# (Usually comes with binutils)
brew install binutils

# Hexdump for binary inspection
# (Built into macOS)
```

### 7. Additional Utilities
```bash
# Git (should be installed with Xcode tools)
git --version

# Disk image tools
brew install cdrtools  # For creating ISO images

# Text processing
brew install gnu-sed   # GNU sed for consistent behavior
```

## Virtual Machine Setup

### UTM Configuration for ARM64
1. **Download UTM**: Install from Mac App Store or website
2. **VM Requirements**:
   - Architecture: ARM64 (AArch64)
   - RAM: 512MB-1GB for testing
   - Storage: 2GB virtual disk
   - Boot method: UEFI

### QEMU Configuration
```bash
# Test ARM64 QEMU
qemu-system-aarch64 -machine virt -cpu cortex-a72 -m 512M -nographic

# Test x86-64 QEMU  
qemu-system-x86_64 -m 512M -nographic
```

## Verification Script

Create a verification script to check all prerequisites:

```bash
#!/bin/bash
# Save as tools/check-prerequisites.sh

echo "Checking MiniOS development prerequisites..."

# Check basic tools
command -v git >/dev/null 2>&1 || echo "❌ Git not found"
command -v make >/dev/null 2>&1 || echo "❌ Make not found"  
command -v python3 >/dev/null 2>&1 || echo "❌ Python3 not found"

# Check cross-compilers
command -v aarch64-elf-gcc >/dev/null 2>&1 || echo "❌ ARM64 cross-compiler not found"
command -v x86_64-elf-gcc >/dev/null 2>&1 || echo "❌ x86-64 cross-compiler not found"

# Check assemblers
command -v nasm >/dev/null 2>&1 || echo "❌ NASM not found"

# Check VMs
command -v qemu-system-aarch64 >/dev/null 2>&1 || echo "❌ QEMU ARM64 not found"
command -v qemu-system-x86_64 >/dev/null 2>&1 || echo "❌ QEMU x86-64 not found"

# Check UTM (if installed via Homebrew)
if [ -d "/Applications/UTM.app" ]; then
    echo "✅ UTM found"
else
    echo "⚠️  UTM not found in /Applications"
fi

echo "Prerequisite check complete!"
```

## Alternative Setup Methods

### Docker Environment (Optional)
For consistent cross-platform development:

```dockerfile
# Dockerfile for MiniOS development
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    gcc-aarch64-linux-gnu \
    gcc-x86-64-linux-gnu \
    nasm \
    qemu-system-arm \
    qemu-system-x86 \
    python3 \
    git

WORKDIR /workspace
```

### GitHub Codespaces (Cloud Development)
Pre-configured development environment in the cloud with all tools installed.

## Common Issues and Solutions

### Issue: Cross-compiler not found
**Solution**: 
```bash
# Add Homebrew paths to shell profile
echo 'export PATH="/opt/homebrew/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc
```

### Issue: UTM won't start ARM64 VM
**Solution**: 
- Ensure you're on Apple Silicon Mac for best ARM64 performance
- On Intel Macs, use QEMU for ARM64 emulation

### Issue: Permission denied for /dev/kvm
**Solution**: 
- macOS doesn't use KVM; this is expected
- Use hardware acceleration options in UTM/QEMU

### Issue: Build fails with "missing headers"
**Solution**:
```bash
# Install additional development headers
xcode-select --install
brew install binutils
```

## Next Steps

After completing prerequisites:
1. Run the verification script
2. Set up your first VM configuration: [VM_SETUP.md](VM_SETUP.md)  
3. Test build system: [BUILD.md](BUILD.md)
4. Start with Phase 1 development: [DEVELOPMENT_PLAN.md](DEVELOPMENT_PLAN.md)

## Getting Help

- **Homebrew issues**: `brew doctor`
- **Cross-compiler issues**: Check [TOOLCHAIN_BUILD.md](TOOLCHAIN_BUILD.md)
- **VM issues**: See [VM_SETUP.md](VM_SETUP.md)
- **Build issues**: See [BUILD.md](BUILD.md)
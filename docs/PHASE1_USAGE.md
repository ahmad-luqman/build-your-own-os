# Phase 1 Usage Guide - How to Run Foundation Setup

## Quick Start

### 1. Basic Testing (No Prerequisites Required)
```bash
# Navigate to project directory
cd build-your-own-os

# Test the build system
make info
make help

# Run comprehensive Phase 1 tests
./tools/test-phase1.sh

# Check what prerequisites you need
./tools/check-prerequisites.sh
```

### 2. Install Prerequisites (Automated)
```bash
# Run prerequisite checker with auto-install
./tools/check-prerequisites.sh
# Answer 'y' when prompted to install missing packages

# This will install via Homebrew:
# - qemu (ARM64 and x86-64 emulation)
# - nasm (x86-64 assembler)
# - cdrtools (ISO creation)
# - Cross-compilers (if available)
```

### 3. Test Build System (After Prerequisites)
```bash
# Build for ARM64 (default architecture)
make

# Build for x86-64
make ARCH=x86_64

# Build with debug symbols
make DEBUG=1

# Clean and rebuild
make clean all

# Test in virtual machine
make test

# Start debug session
make debug
```

## Detailed Usage

### Build System Commands

#### Information Commands
```bash
# Show build configuration
make info

# Show build configuration for specific architecture
make info ARCH=arm64
make info ARCH=x86_64

# Show debug build configuration
make info DEBUG=1

# Show all available commands
make help
```

#### Build Commands
```bash
# Build everything (kernel + bootloader + image)
make all                    # ARM64 (default)
make ARCH=x86_64 all       # x86-64

# Build specific components
make kernel                 # Kernel ELF only
make bootloader            # Bootloader only
make image                 # Bootable image only

# Architecture-specific builds
make arm64                 # Build for ARM64
make x86_64               # Build for x86-64

# Debug builds
make DEBUG=1              # Build with debug symbols
make DEBUG=1 ARCH=x86_64  # Debug build for x86-64
```

#### Testing Commands
```bash
# Test in virtual machine
make test                  # Test ARM64 in QEMU
make test ARCH=x86_64     # Test x86-64 in QEMU

# Quick build and test
make quick-test           # Build + test in one command

# Debug in VM
make debug                # Start debug session
make debug ARCH=x86_64   # Debug x86-64
```

#### Utility Commands
```bash
# Clean build artifacts
make clean

# Clean and rebuild
make clean all

# Show file information
ls -la build/arm64/       # ARM64 build outputs
ls -la build/x86_64/      # x86-64 build outputs
```

### Development Tools Usage

#### Prerequisites Management
```bash
# Check what's installed/missing
./tools/check-prerequisites.sh

# Get detailed status
./tools/check-prerequisites.sh --help

# Re-run after installing tools
./tools/check-prerequisites.sh
```

#### VM Testing
```bash
# Test specific architecture with timeout
./tools/test-vm.sh arm64 30        # 30 second timeout
./tools/test-vm.sh x86_64 60       # 60 second timeout

# Test with GUI (not headless)
./tools/test-vm.sh arm64 30 false

# Get help
./tools/test-vm.sh --help
```

#### Debugging
```bash
# Start debug session
./tools/debug.sh arm64
./tools/debug.sh x86_64

# Use different GDB port
./tools/debug.sh arm64 5678

# Get debug help
./tools/debug.sh --help
```

#### Image Creation
```bash
# Create ARM64 disk image
python3 tools/create_image.py \
  --arch arm64 \
  --kernel build/arm64/kernel.elf \
  --bootloader build/arm64/bootloader.elf \
  --output build/arm64/minios.img

# Create x86-64 ISO image
python3 tools/create_iso.py \
  --kernel build/x86_64/kernel.elf \
  --bootloader build/x86_64/bootloader.bin \
  --output build/x86_64/minios.iso

# Verbose output
python3 tools/create_image.py --verbose [other options]
```

#### VM Configuration
```bash
# Run ARM64 VM directly
./vm-configs/qemu-arm64.sh

# Run x86-64 VM directly  
./vm-configs/qemu-x86_64.sh

# These scripts expect images to exist in build/
```

### Testing and Validation

#### Automated Testing
```bash
# Run full Phase 1 test suite
./tools/test-phase1.sh

# Expected output: Should show 30-32 tests passing
# Pass rate should be 90%+ for healthy Phase 1
```

#### Manual Validation
```bash
# 1. Check project structure
ls -la                     # Should show Makefile, src/, docs/, tools/
find . -name "*.h" -o -name "*.c" -o -name "*.S" -o -name "*.asm"

# 2. Verify executable permissions
ls -la tools/*.sh vm-configs/*.sh
# All should show -rwxr-xr-x (executable)

# 3. Test Python tools
python3 tools/create_image.py --help
python3 tools/create_iso.py --help

# 4. Test build system without prerequisites
make info                  # Should show configuration
make help                  # Should show help text

# 5. Test with prerequisites (if installed)
make ARCH=arm64           # Should build or show clear errors
make ARCH=x86_64          # Should build or show clear errors
```

## Expected Results at Each Stage

### Stage 1: Basic Project Setup ✅
After initial setup, you should have:
- Working `make info` and `make help`
- All scripts executable (`./tools/test-phase1.sh` passes structure tests)
- Clear error messages when prerequisites are missing

### Stage 2: Prerequisites Installed 🔄
After running `./tools/check-prerequisites.sh`:
- QEMU installed (`qemu-system-aarch64`, `qemu-system-x86_64` available)
- NASM installed (`nasm` available)
- Cross-compilers available (may require manual installation)

### Stage 3: Full Build Capability 🎯
After all prerequisites are available:
- `make ARCH=arm64` produces `build/arm64/kernel.elf` and `minios.img`
- `make ARCH=x86_64` produces `build/x86_64/kernel.elf` and `minios.iso`
- `make test` runs VMs and shows kernel output
- `make debug` starts GDB debugging sessions

## Troubleshooting

### Common Issues and Solutions

#### "Command not found: make"
```bash
# Install Xcode Command Line Tools
xcode-select --install

# Verify installation
which make
```

#### "Permission denied" for scripts
```bash
# Fix all script permissions
chmod +x tools/*.sh tools/*.py vm-configs/*.sh

# Verify
ls -la tools/*.sh
```

#### "aarch64-elf-gcc: command not found"
```bash
# This is expected without cross-compilers
# Install via Homebrew (may not be available)
brew install aarch64-elf-gcc

# Alternative: Build from source (advanced)
# See docs/PREREQUISITES.md for instructions
```

#### "python3: command not found"
```bash
# Install Python 3
brew install python3

# Verify
python3 --version
```

#### Build fails with linking errors
```bash
# Check linker scripts exist
ls -la src/arch/*/linker.ld

# Clean and retry
make clean
make ARCH=arm64

# Check for typos in source files
```

#### VM tests fail immediately
```bash
# This is expected without built images
# First build, then test
make ARCH=arm64
make test

# Or check VM without image (should show clear error)
./tools/test-vm.sh arm64
```

### Getting Help

#### Command-Line Help
```bash
make help                           # Build system help
./tools/test-vm.sh --help          # VM testing help
./tools/debug.sh --help            # Debugging help
python3 tools/create_image.py --help  # Image creation help
```

#### Documentation
- **[README.md](../README.md)** - Project overview
- **[PREREQUISITES.md](PREREQUISITES.md)** - Environment setup
- **[BUILD.md](BUILD.md)** - Detailed build system guide
- **[PHASE1_TESTING.md](PHASE1_TESTING.md)** - Testing procedures
- **[VM_SETUP.md](VM_SETUP.md)** - Virtual machine configuration

## Success Criteria

### Phase 1 is working correctly when:

1. **✅ Structure Tests Pass**
   - `./tools/test-phase1.sh` shows 90%+ pass rate
   - All essential files exist
   - All scripts are executable

2. **✅ Build System Works**
   - `make info` shows configuration without errors
   - `make help` displays comprehensive help
   - Architecture detection works (`make info ARCH=x86_64`)

3. **✅ Tools Function**
   - Python scripts show help without errors
   - VM scripts fail gracefully when images don't exist
   - Prerequisites checker identifies missing/installed tools

4. **✅ Error Handling**
   - Missing prerequisites show clear error messages
   - Build attempts fail gracefully with helpful guidance
   - No confusing or cryptic error messages

### Ready for Phase 2 when:

1. **Prerequisites installed** - Cross-compilers and QEMU available
2. **Builds succeed** - `make ARCH=arm64` and `make ARCH=x86_64` work
3. **VMs boot** - `make test` shows kernel output in VM
4. **Debug works** - `make debug` starts GDB sessions

## Next Steps

Once Phase 1 is working:

1. **Complete prerequisite installation**
2. **Test full build process** 
3. **Verify VM environment works**
4. **Begin Phase 2**: Enhanced bootloader implementation

Phase 1 provides the solid foundation for all subsequent development!
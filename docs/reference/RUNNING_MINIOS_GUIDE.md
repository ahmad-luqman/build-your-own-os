# Complete Guide: Running MiniOS on macOS with UTM and QEMU

This guide provides step-by-step instructions to build and run MiniOS on your macOS system using either UTM (recommended for ARM64) or QEMU.

## Prerequisites ✅

The following are already installed on your system:
- ✅ QEMU (`qemu-system-aarch64` and `qemu-system-x86_64` found)
- ✅ Cross-compilation toolchains (ARM64 and x86_64 GCC)
- ✅ MiniOS built successfully for both architectures

## Built Images Status ✅

```bash
# ARM64 Image (16MB disk image)
build/arm64/minios.img     # For UTM and QEMU ARM64

# x86_64 Image (480KB ISO)
build/x86_64/minios.iso    # For UTM and QEMU x86_64
```

## Method 1: QEMU (Command Line) - Recommended for Testing

### Running ARM64 Version

```bash
# Quick start - ARM64 (recommended on Apple Silicon Macs)
cd /Users/ahmadluqman/src/build-your-own-os

# Run with QEMU directly
qemu-system-aarch64 \
    -machine virt \
    -cpu cortex-a72 \
    -m 512M \
    -drive file=build/arm64/minios.img,format=raw,if=virtio \
    -netdev user,id=net0 \
    -device virtio-net-device,netdev=net0 \
    -nographic \
    -serial mon:stdio
```

### Running x86_64 Version

```bash
# Quick start - x86_64 (universal compatibility)
cd /Users/ahmadluqman/src/build-your-own-os

# Run with QEMU directly
qemu-system-x86_64 \
    -m 512M \
    -cdrom build/x86_64/minios.iso \
    -boot d \
    -nographic \
    -serial mon:stdio
```

### Using Built-in Scripts

```bash
# Use the provided VM scripts (may need minor fixes)
./vm-configs/qemu-arm64.sh      # ARM64 version
./vm-configs/qemu-x86_64.sh     # x86_64 version

# Or use the test script
./tools/test-vm.sh arm64 60     # ARM64 with 60s timeout
./tools/test-vm.sh x86_64 60    # x86_64 with 60s timeout
```

## Method 2: UTM (GUI) - Great for Interactive Use

### Installing UTM

```bash
# Install UTM via Homebrew
brew install --cask utm

# Or download from Mac App Store or https://mac.getutm.app/
```

### Setting up ARM64 VM in UTM

1. **Open UTM** and click "Create a New Virtual Machine"

2. **Choose "Virtualize"** (for Apple Silicon) or "Emulate" (for Intel)

3. **Select "Other"** for operating system

4. **Configure VM**:
   - **Name**: MiniOS ARM64
   - **Architecture**: ARM64 (aarch64)
   - **System**: 
     - Boot: UEFI
     - Machine: virt-4.0 or latest
   - **CPU**: 1-2 cores
   - **Memory**: 512MB - 1GB
   - **Storage**: 
     - **Import existing disk**: Select `build/arm64/minios.img`
     - Interface: VirtIO

5. **Network**: Shared Network (enabled)

6. **Display**: Console mode (recommended) or VGA

7. **Save and Start** the VM

### Setting up x86_64 VM in UTM

1. **Create New VM** → **Emulate** → **Other**

2. **Configure VM**:
   - **Name**: MiniOS x86_64  
   - **Architecture**: x86_64
   - **Memory**: 512MB
   - **Storage**: 
     - **CD/DVD**: Select `build/x86_64/minios.iso`
     - Set as boot device

3. **Save and Start** the VM

## Method 3: Build and Test Scripts

### Using Make Targets

```bash
# Build and test in one command
make ARCH=arm64 test       # Build and test ARM64
make ARCH=x86_64 test      # Build and test x86_64

# Quick test (build + run)
make ARCH=arm64 quick-test
```

### Testing with Different Configurations

```bash
# Debug build with more logging
make ARCH=arm64 DEBUG=1 test

# Headless testing (no GUI)
./tools/test-vm.sh arm64 30 true

# GUI testing  
./tools/test-vm.sh arm64 30 false
```

## What to Expect When MiniOS Boots

### Successful Boot Sequence

```
[BOOT] MiniOS ARM64 Bootloader v1.0.0
[BOOT] Loading kernel...
[KERN] MiniOS v1.0.0 - Educational Operating System
[KERN] Architecture: ARM64 (AArch64)
[KERN] Memory: 512MB detected
[KERN] Initializing MMU and paging...
[KERN] Device discovery...
[KERN] File system initialized
[KERN] Shell starting...

Welcome to MiniOS v1.0.0!
Type 'help' for available commands.

MiniOS> 
```

### Available Commands in MiniOS Shell

Once booted, you can try these commands:

```bash
MiniOS> help              # Show all available commands
MiniOS> ls -la /          # List root directory
MiniOS> ps                # Show running processes  
MiniOS> mem               # Show memory usage
MiniOS> mkdir test        # Create directory
MiniOS> echo "Hello" > hello.txt   # Create file
MiniOS> cat hello.txt     # Read file
MiniOS> calc 2 + 3 * 4    # Calculator
MiniOS> tictactoe         # Play tic-tac-toe
MiniOS> top               # System monitor
MiniOS> reboot            # Restart system
```

## Troubleshooting

### Common Issues and Solutions

#### 1. "QEMU not found"
```bash
# Install QEMU if missing
brew install qemu

# Verify installation
which qemu-system-aarch64
which qemu-system-x86_64
```

#### 2. "Image file not found"
```bash
# Rebuild the images
make ARCH=arm64 clean all    # Rebuild ARM64
make ARCH=x86_64 clean all   # Rebuild x86_64

# Verify files exist
ls -lh build/arm64/minios.img
ls -lh build/x86_64/minios.iso
```

#### 3. "VM hangs at boot"
- **ARM64**: Try reducing memory to 256MB
- **x86_64**: Check if virtualization is enabled
- **Both**: Try debug build: `make DEBUG=1 ARCH=<arch>`

#### 4. "No output in console"
- Add `-serial stdio` flag to QEMU command
- For UTM: Enable console/serial output in settings

#### 5. "Permission denied" on scripts
```bash
# Make scripts executable
chmod +x vm-configs/*.sh
chmod +x tools/*.sh
```

### Getting Debug Information

```bash
# Build with debug symbols and verbose logging
make ARCH=arm64 DEBUG=1 all

# Run with debug output
qemu-system-aarch64 \
    -machine virt -cpu cortex-a72 -m 512M \
    -drive file=build/arm64/minios.img,format=raw,if=virtio \
    -nographic -serial mon:stdio \
    -d guest_errors,unimp
```

### Performance Tips

#### For Better Performance
- **UTM**: Enable hardware acceleration if available
- **QEMU**: Use appropriate CPU models
- **Both**: Start with minimal memory (256-512MB)

#### For Development
- Use console mode (nographic) for faster testing
- Use smaller timeout values for automated testing
- Keep multiple terminal sessions for build/test cycles

## Quick Reference Commands

```bash
# Build both architectures
make ARCH=arm64 clean all && make ARCH=x86_64 clean all

# Test both architectures  
./tools/test-vm.sh arm64 30 && ./tools/test-vm.sh x86_64 30

# Run ARM64 in QEMU
qemu-system-aarch64 -machine virt -cpu cortex-a72 -m 512M \
    -drive file=build/arm64/minios.img,format=raw,if=virtio \
    -nographic -serial mon:stdio

# Run x86_64 in QEMU
qemu-system-x86_64 -m 512M -cdrom build/x86_64/minios.iso \
    -boot d -nographic -serial mon:stdio

# Exit QEMU
# Press Ctrl+A, then X
# Or type 'quit' in QEMU monitor (Ctrl+A, then C)
```

## Advanced Usage

### Debugging with GDB

```bash
# Start QEMU with GDB server
qemu-system-aarch64 -machine virt -cpu cortex-a72 -m 512M \
    -drive file=build/arm64/minios.img,format=raw,if=virtio \
    -nographic -serial mon:stdio \
    -gdb tcp::1234 -S

# In another terminal, connect GDB
aarch64-elf-gdb build/arm64/kernel.elf
(gdb) target remote localhost:1234
(gdb) continue
```

### File System Testing

MiniOS includes a complete file system. Test it with:

```bash
MiniOS> ls /                    # List root directory
MiniOS> mkdir projects          # Create directory
MiniOS> cd projects            # Change directory
MiniOS> echo "Hello MiniOS" > greeting.txt
MiniOS> cat greeting.txt       # Read file
MiniOS> ls -la                 # List with details
```

## Educational Value

Running MiniOS demonstrates:

- **Cross-platform development**: Same OS running on ARM64 and x86_64
- **Bootloader implementation**: UEFI (ARM64) and Multiboot2 (x86_64)  
- **Memory management**: Virtual memory and paging
- **Process management**: Multitasking and scheduling
- **File systems**: VFS layer with SFS implementation
- **Device drivers**: Timer, UART, interrupt management
- **System calls**: Complete syscall interface
- **Shell interface**: Interactive command-line environment

---

**🎉 You now have a complete, bootable operating system running on your Mac!**

This represents a fully functional educational OS with cross-platform support, memory management, file systems, and interactive shell - built from scratch!
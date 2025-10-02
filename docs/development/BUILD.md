# Build System for MiniOS

## Overview

MiniOS uses a flexible build system supporting cross-compilation for ARM64 and x86-64 architectures with GNU Make as the primary build tool.

## Quick Start

```bash
# Build for ARM64 (default)
make

# Build for x86-64  
make ARCH=x86_64

# Clean build
make clean

# Build and test in VM
make test ARCH=arm64

# Build debug version
make DEBUG=1
```

## Build Targets

### Primary Targets
- `all` (default): Build kernel and create bootable image
- `kernel`: Build kernel only
- `bootloader`: Build bootloader only
- `image`: Create bootable disk image
- `iso`: Create bootable ISO (x86-64 only)
- `clean`: Remove all build artifacts
- `test`: Build and run in VM

### Architecture-Specific Targets
- `arm64`: Build for ARM64/AArch64
- `x86_64`: Build for x86-64/AMD64

### Debug Targets
- `debug`: Build with debug symbols and logging
- `gdb`: Start GDB debugging session
- `qemu-debug`: Run QEMU with GDB server

## Build Configuration

### Makefile Variables
```makefile
# Architecture (arm64 or x86_64)
ARCH ?= arm64

# Debug build (0 or 1)
DEBUG ?= 0

# Optimization level
OPT_LEVEL ?= -O2

# Build directory
BUILD_DIR ?= build

# Source directory
SRC_DIR ?= src

# Target triple for cross-compilation
TARGET_TRIPLE_ARM64 = aarch64-elf
TARGET_TRIPLE_X86_64 = x86_64-elf
```

### Cross-Compilation Setup
```makefile
ifeq ($(ARCH),arm64)
    CC = aarch64-elf-gcc
    AS = aarch64-elf-as
    LD = aarch64-elf-ld
    OBJCOPY = aarch64-elf-objcopy
    OBJDUMP = aarch64-elf-objdump
else ifeq ($(ARCH),x86_64)
    CC = x86_64-elf-gcc
    AS = nasm
    LD = x86_64-elf-ld
    OBJCOPY = x86_64-elf-objcopy
    OBJDUMP = x86_64-elf-objdump
endif
```

## Directory Structure

```
build-your-own-os/
├── Makefile                    # Main build file
├── build/                      # Build output
│   ├── arm64/                 # ARM64 build artifacts
│   └── x86_64/                # x86-64 build artifacts
├── src/
│   ├── boot/                  # Bootloader source
│   ├── kernel/                # Kernel source
│   ├── arch/                  # Architecture-specific code
│   ├── drivers/               # Device drivers
│   ├── fs/                    # File system
│   └── userland/              # User applications
└── tools/                     # Build scripts and tools
```

## Main Makefile

### Core Makefile Structure
```makefile
# build-your-own-os/Makefile

# Default configuration
ARCH ?= arm64
DEBUG ?= 0
BUILD_DIR ?= build
SRC_DIR ?= src

# Include architecture-specific configuration
include tools/build/arch-$(ARCH).mk

# Compiler flags
CFLAGS = -ffreestanding -nostdlib -nostartfiles -nodefaultlibs
CFLAGS += -Wall -Wextra -Werror
CFLAGS += -std=c11
CFLAGS += -I$(SRC_DIR)/include
CFLAGS += -I$(SRC_DIR)/arch/$(ARCH)/include

ifeq ($(DEBUG),1)
    CFLAGS += -g -DDEBUG -O0
else
    CFLAGS += -O2 -DNDEBUG
endif

# Linker flags
LDFLAGS = -nostdlib -static

# Source files
KERNEL_SOURCES = $(wildcard $(SRC_DIR)/kernel/*.c)
KERNEL_SOURCES += $(wildcard $(SRC_DIR)/arch/$(ARCH)/*.c)
KERNEL_SOURCES += $(wildcard $(SRC_DIR)/drivers/*.c)

ASM_SOURCES = $(wildcard $(SRC_DIR)/arch/$(ARCH)/*.S)

# Object files
KERNEL_OBJECTS = $(KERNEL_SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/$(ARCH)/%.o)
ASM_OBJECTS = $(ASM_SOURCES:$(SRC_DIR)/%.S=$(BUILD_DIR)/$(ARCH)/%.o)

# Targets
.PHONY: all clean kernel bootloader image test

all: kernel bootloader image

kernel: $(BUILD_DIR)/$(ARCH)/kernel.elf

bootloader: $(BUILD_DIR)/$(ARCH)/bootloader.elf

image: $(BUILD_DIR)/$(ARCH)/minios.img

# Build kernel
$(BUILD_DIR)/$(ARCH)/kernel.elf: $(KERNEL_OBJECTS) $(ASM_OBJECTS)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -T $(SRC_DIR)/arch/$(ARCH)/linker.ld -o $@ $^

# Compile C sources
$(BUILD_DIR)/$(ARCH)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble sources
$(BUILD_DIR)/$(ARCH)/%.o: $(SRC_DIR)/%.S
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# Clean build
clean:
	rm -rf $(BUILD_DIR)

# Test in VM
test: all
	@echo "Testing MiniOS for $(ARCH)..."
	bash tools/test-vm.sh $(ARCH)

# Debug with GDB
debug: all
	@echo "Starting debug session for $(ARCH)..."
	bash tools/debug.sh $(ARCH)
```

### Architecture-Specific Build Files

#### ARM64 Configuration
```makefile
# tools/build/arch-arm64.mk

CC = aarch64-elf-gcc
AS = aarch64-elf-as  
LD = aarch64-elf-ld
OBJCOPY = aarch64-elf-objcopy
OBJDUMP = aarch64-elf-objdump

# ARM64-specific flags
CFLAGS += -mcpu=cortex-a72
CFLAGS += -mgeneral-regs-only
ASFLAGS = -mcpu=cortex-a72

# Bootloader build
$(BUILD_DIR)/$(ARCH)/bootloader.elf: $(SRC_DIR)/boot/arm64/boot.S
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $(BUILD_DIR)/$(ARCH)/boot.o
	$(LD) $(LDFLAGS) -T $(SRC_DIR)/boot/arm64/boot.ld \
		-o $@ $(BUILD_DIR)/$(ARCH)/boot.o

# Create disk image
$(BUILD_DIR)/$(ARCH)/minios.img: $(BUILD_DIR)/$(ARCH)/kernel.elf $(BUILD_DIR)/$(ARCH)/bootloader.elf
	@mkdir -p $(dir $@)
	python3 tools/create_image.py --arch arm64 \
		--kernel $(BUILD_DIR)/$(ARCH)/kernel.elf \
		--bootloader $(BUILD_DIR)/$(ARCH)/bootloader.elf \
		--output $@
```

#### x86-64 Configuration
```makefile
# tools/build/arch-x86_64.mk

CC = x86_64-elf-gcc
AS = nasm
LD = x86_64-elf-ld  
OBJCOPY = x86_64-elf-objcopy
OBJDUMP = x86_64-elf-objdump

# x86-64-specific flags
CFLAGS += -mcmodel=kernel
CFLAGS += -mno-red-zone
CFLAGS += -mno-mmx -mno-sse -mno-sse2
ASFLAGS = -f elf64

# Bootloader build  
$(BUILD_DIR)/$(ARCH)/bootloader.bin: $(SRC_DIR)/boot/x86_64/boot.asm
	@mkdir -p $(dir $@)
	$(AS) -f bin $< -o $@

# Create ISO image
$(BUILD_DIR)/$(ARCH)/minios.iso: $(BUILD_DIR)/$(ARCH)/kernel.elf $(BUILD_DIR)/$(ARCH)/bootloader.bin
	@mkdir -p $(dir $@)
	python3 tools/create_iso.py \
		--kernel $(BUILD_DIR)/$(ARCH)/kernel.elf \
		--bootloader $(BUILD_DIR)/$(ARCH)/bootloader.bin \
		--output $@
```

## Build Tools

### Image Creation Script
```python
#!/usr/bin/env python3
# tools/create_image.py

import argparse
import struct
import os

def create_arm64_image(kernel_path, bootloader_path, output_path):
    """Create ARM64 disk image."""
    
    # Read kernel and bootloader
    with open(kernel_path, 'rb') as f:
        kernel_data = f.read()
    
    with open(bootloader_path, 'rb') as f:
        bootloader_data = f.read()
    
    # Create image (simple concatenation for now)
    with open(output_path, 'wb') as f:
        # Write bootloader first
        f.write(bootloader_data)
        
        # Pad to 1MB boundary
        current_size = len(bootloader_data)
        padding_needed = (1024 * 1024) - (current_size % (1024 * 1024))
        f.write(b'\x00' * padding_needed)
        
        # Write kernel
        f.write(kernel_data)
        
        # Pad to minimum image size (16MB)
        current_size = f.tell()
        min_size = 16 * 1024 * 1024
        if current_size < min_size:
            f.write(b'\x00' * (min_size - current_size))

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Create MiniOS disk image')
    parser.add_argument('--arch', required=True, choices=['arm64', 'x86_64'])
    parser.add_argument('--kernel', required=True, help='Kernel ELF file')
    parser.add_argument('--bootloader', required=True, help='Bootloader file')
    parser.add_argument('--output', required=True, help='Output image file')
    
    args = parser.parse_args()
    
    if args.arch == 'arm64':
        create_arm64_image(args.kernel, args.bootloader, args.output)
    else:
        # x86_64 ISO creation would be implemented here
        pass
```

### Testing Script
```bash
#!/bin/bash
# tools/test-vm.sh

set -e

ARCH=$1
BUILD_DIR="build"
TIMEOUT=30

if [ -z "$ARCH" ]; then
    echo "Usage: $0 {arm64|x86_64}"
    exit 1
fi

# Check if image exists
case $ARCH in
    arm64)
        IMAGE_FILE="$BUILD_DIR/$ARCH/minios.img"
        VM_CMD="qemu-system-aarch64 -machine virt -cpu cortex-a72 -m 512M -drive file=$IMAGE_FILE,format=raw"
        ;;
    x86_64)
        IMAGE_FILE="$BUILD_DIR/$ARCH/minios.iso"
        VM_CMD="qemu-system-x86_64 -m 512M -cdrom $IMAGE_FILE"
        ;;
    *)
        echo "Unknown architecture: $ARCH"
        exit 1
        ;;
esac

if [ ! -f "$IMAGE_FILE" ]; then
    echo "Error: Image file $IMAGE_FILE not found"
    echo "Run 'make ARCH=$ARCH' first"
    exit 1
fi

echo "Testing MiniOS ($ARCH) with image: $IMAGE_FILE"
echo "VM will timeout after ${TIMEOUT} seconds"

# Run VM with timeout
timeout $TIMEOUT $VM_CMD -nographic -serial mon:stdio || {
    echo "Test completed (timeout or manual exit)"
}
```

### Debug Script
```bash
#!/bin/bash
# tools/debug.sh

ARCH=$1
BUILD_DIR="build"
GDB_PORT=1234

if [ -z "$ARCH" ]; then
    echo "Usage: $0 {arm64|x86_64}"
    exit 1
fi

case $ARCH in
    arm64)
        KERNEL_ELF="$BUILD_DIR/$ARCH/kernel.elf"
        IMAGE_FILE="$BUILD_DIR/$ARCH/minios.img"
        GDB_CMD="aarch64-elf-gdb"
        VM_CMD="qemu-system-aarch64 -machine virt -cpu cortex-a72 -m 512M"
        VM_CMD="$VM_CMD -drive file=$IMAGE_FILE,format=raw"
        VM_CMD="$VM_CMD -gdb tcp::$GDB_PORT -S"
        ;;
    x86_64)
        KERNEL_ELF="$BUILD_DIR/$ARCH/kernel.elf"
        IMAGE_FILE="$BUILD_DIR/$ARCH/minios.iso"
        GDB_CMD="x86_64-elf-gdb"
        VM_CMD="qemu-system-x86_64 -m 512M -cdrom $IMAGE_FILE"
        VM_CMD="$VM_CMD -gdb tcp::$GDB_PORT -S"
        ;;
esac

echo "Starting debug session for $ARCH"
echo "1. VM will start with GDB server on port $GDB_PORT"
echo "2. Connect with: $GDB_CMD $KERNEL_ELF"
echo "3. In GDB: target remote localhost:$GDB_PORT"

# Start VM in background
$VM_CMD -nographic -serial mon:stdio &
VM_PID=$!

echo "VM started with PID $VM_PID"
echo "Connect GDB to localhost:$GDB_PORT"

# Wait for user to finish debugging
read -p "Press Enter to stop VM..."
kill $VM_PID 2>/dev/null
```

## Build Process Details

### Compilation Steps
1. **Source Analysis**: Scan source directories for files
2. **Dependency Check**: Check if rebuild is needed
3. **Cross-Compilation**: Compile C sources with architecture-specific flags
4. **Assembly**: Assemble architecture-specific assembly files
5. **Linking**: Link objects into kernel ELF file
6. **Bootloader**: Build architecture-specific bootloader
7. **Image Creation**: Package kernel and bootloader into bootable image

### Linker Scripts

#### ARM64 Linker Script
```ld
/* src/arch/arm64/linker.ld */

ENTRY(_start)

SECTIONS
{
    . = 0x40080000;

    .text : {
        *(.text.start)
        *(.text*)
    }

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data*)
    }

    .bss : {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    }

    __kernel_end = .;
}
```

#### x86-64 Linker Script
```ld
/* src/arch/x86_64/linker.ld */

ENTRY(_start)

SECTIONS
{
    . = 1M;

    .text : {
        *(.multiboot)
        *(.text*)
    }

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data*)
    }

    .bss : {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    }
}
```

## Build Optimization

### Parallel Builds
```makefile
# Enable parallel compilation
MAKEFLAGS += -j$(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
```

### Incremental Builds
```makefile
# Dependency generation
DEPS = $(KERNEL_OBJECTS:.o=.d) $(ASM_OBJECTS:.o=.d)

-include $(DEPS)

$(BUILD_DIR)/$(ARCH)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -c $< -o $@
```

### Build Cache
```makefile
# Use ccache if available
ifneq ($(shell which ccache 2>/dev/null),)
    CC := ccache $(CC)
endif
```

## Troubleshooting

### Common Build Issues

#### Cross-compiler not found
```bash
# Install missing cross-compiler
brew install aarch64-elf-gcc
brew install x86_64-elf-gcc
```

#### Linker errors
- Check linker script syntax
- Verify all referenced symbols are defined
- Check for circular dependencies

#### Image creation fails
- Verify Python 3 is available
- Check file permissions
- Ensure enough disk space

### Build System Debugging
```makefile
# Add to Makefile for debugging
debug-makefile:
	@echo "ARCH: $(ARCH)"
	@echo "CC: $(CC)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "SOURCES: $(KERNEL_SOURCES)"
	@echo "OBJECTS: $(KERNEL_OBJECTS)"
```

## Next Steps

After setting up the build system:
1. **Test Build**: Try building a minimal kernel
2. **VM Integration**: Test images in virtual machines
3. **Development Workflow**: Set up continuous testing
4. **Begin Implementation**: Start with Phase 1 development

For implementation details, see [DEVELOPMENT_PLAN.md](DEVELOPMENT_PLAN.md).
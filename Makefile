# MiniOS Build System
# Cross-platform operating system build configuration

# Default configuration
ARCH ?= arm64
DEBUG ?= 0
BUILD_DIR ?= build
SRC_DIR ?= src
TOOLS_DIR ?= tools

# Create build directory
$(shell mkdir -p $(BUILD_DIR)/$(ARCH))

# Include architecture-specific configuration
include $(TOOLS_DIR)/build/arch-$(ARCH).mk

# Compiler flags (common)
CFLAGS = -ffreestanding -nostdlib -nostartfiles -nodefaultlibs
CFLAGS += -Wall -Wextra -Werror
CFLAGS += -std=c11
CFLAGS += -I$(SRC_DIR)/include
CFLAGS += -I$(SRC_DIR)/arch/$(ARCH)/include

# Debug vs Release flags
ifeq ($(DEBUG),1)
    CFLAGS += -g -DDEBUG -O0 -DLOG_LEVEL=0
    BUILD_TYPE = debug
else
    CFLAGS += -O2 -DNDEBUG -DLOG_LEVEL=2
    BUILD_TYPE = release
endif

# Linker flags
LDFLAGS = -nostdlib -static

# Source files
KERNEL_C_SOURCES = $(wildcard $(SRC_DIR)/kernel/*.c)
KERNEL_C_SOURCES += $(wildcard $(SRC_DIR)/arch/$(ARCH)/*.c)
KERNEL_C_SOURCES += $(wildcard $(SRC_DIR)/drivers/*.c)

KERNEL_ASM_SOURCES = $(wildcard $(SRC_DIR)/arch/$(ARCH)/*.S)
KERNEL_ASM_SOURCES += $(wildcard $(SRC_DIR)/arch/$(ARCH)/*.asm)

# Object files
KERNEL_C_OBJECTS = $(KERNEL_C_SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/$(ARCH)/%.o)
KERNEL_ASM_OBJECTS = $(KERNEL_ASM_SOURCES:$(SRC_DIR)/%.S=$(BUILD_DIR)/$(ARCH)/%.o)
KERNEL_ASM_OBJECTS += $(KERNEL_ASM_SOURCES:$(SRC_DIR)/%.asm=$(BUILD_DIR)/$(ARCH)/%.o)

ALL_OBJECTS = $(KERNEL_C_OBJECTS) $(KERNEL_ASM_OBJECTS)

# Dependency files
DEPS = $(KERNEL_C_OBJECTS:.o=.d)

# Default target
.PHONY: all clean kernel bootloader image test debug help info

all: info kernel bootloader image

# Display build information
info:
	@echo "=== MiniOS Build System ==="
	@echo "Architecture: $(ARCH)"
	@echo "Build Type:   $(BUILD_TYPE)"
	@echo "Compiler:     $(CC)"
	@echo "Build Dir:    $(BUILD_DIR)/$(ARCH)"
	@echo "=========================="

# Build kernel
kernel: $(BUILD_DIR)/$(ARCH)/kernel.elf

$(BUILD_DIR)/$(ARCH)/kernel.elf: $(ALL_OBJECTS) $(SRC_DIR)/arch/$(ARCH)/linker.ld
	@echo "Linking kernel for $(ARCH)..."
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -T $(SRC_DIR)/arch/$(ARCH)/linker.ld -o $@ $(ALL_OBJECTS)
	@echo "Kernel built: $@"
	@$(OBJDUMP) -h $@

# Compile C sources
$(BUILD_DIR)/$(ARCH)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -c $< -o $@

# Assemble sources (GAS format)
$(BUILD_DIR)/$(ARCH)/%.o: $(SRC_DIR)/%.S
	@echo "Assembling $<..."
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

# Include dependency files
-include $(DEPS)

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)
	@echo "Clean complete."

# Test in VM
test: all
	@echo "Testing MiniOS ($(ARCH)) in virtual machine..."
	@bash $(TOOLS_DIR)/test-vm.sh $(ARCH)

# Debug session
debug: all
	@echo "Starting debug session for $(ARCH)..."
	@bash $(TOOLS_DIR)/debug.sh $(ARCH)

# Quick test (build + run)
quick-test: all test

# Architecture-specific builds
arm64:
	$(MAKE) ARCH=arm64

x86_64:
	$(MAKE) ARCH=x86_64

# Help target
help:
	@echo "MiniOS Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all           Build kernel, bootloader, and image (default)"
	@echo "  kernel        Build kernel only"
	@echo "  bootloader    Build bootloader only"
	@echo "  image         Create bootable image"
	@echo "  test          Build and test in VM"
	@echo "  debug         Build and start debug session"
	@echo "  clean         Remove all build artifacts"
	@echo ""
	@echo "Variables:"
	@echo "  ARCH=<arch>   Target architecture (arm64, x86_64)"
	@echo "  DEBUG=1       Build with debug symbols and logging"
	@echo ""
	@echo "Examples:"
	@echo "  make                    # Build for ARM64 (default)"
	@echo "  make ARCH=x86_64        # Build for x86-64"
	@echo "  make DEBUG=1 test       # Debug build and test"
	@echo "  make clean all          # Clean build"
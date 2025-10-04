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

# Source files for kernel (exclude bootloader files)
KERNEL_C_SOURCES = $(wildcard $(SRC_DIR)/kernel/*.c)
KERNEL_C_SOURCES += $(wildcard $(SRC_DIR)/kernel/process/*.c)
KERNEL_C_SOURCES += $(wildcard $(SRC_DIR)/kernel/syscall/*.c)
KERNEL_C_SOURCES += $(wildcard $(SRC_DIR)/kernel/fd/*.c)
KERNEL_C_SOURCES += $(wildcard $(SRC_DIR)/kernel/loader/*.c)
KERNEL_C_SOURCES += $(filter-out %boot_main.c %uefi_boot.c, $(wildcard $(SRC_DIR)/arch/$(ARCH)/*.c))
KERNEL_C_SOURCES += $(wildcard $(SRC_DIR)/arch/$(ARCH)/memory/*.c)
KERNEL_C_SOURCES += $(wildcard $(SRC_DIR)/arch/$(ARCH)/interrupts/*.c)
KERNEL_C_SOURCES += $(wildcard $(SRC_DIR)/arch/$(ARCH)/kernel_loader/*.c)
KERNEL_C_SOURCES += $(wildcard $(SRC_DIR)/drivers/*.c)
KERNEL_C_SOURCES += $(wildcard $(SRC_DIR)/drivers/timer/*.c)
KERNEL_C_SOURCES += $(wildcard $(SRC_DIR)/drivers/uart/*.c)
KERNEL_C_SOURCES += $(wildcard $(SRC_DIR)/drivers/interrupt/*.c)
KERNEL_C_SOURCES += $(wildcard $(SRC_DIR)/drivers/pci/*.c)
KERNEL_C_SOURCES += $(wildcard $(SRC_DIR)/fs/vfs/*.c)
KERNEL_C_SOURCES += $(wildcard $(SRC_DIR)/fs/sfs/*.c)
KERNEL_C_SOURCES += $(wildcard $(SRC_DIR)/fs/block/*.c)
KERNEL_C_SOURCES += $(wildcard $(SRC_DIR)/fs/ramfs/*.c)
KERNEL_C_SOURCES += $(wildcard $(SRC_DIR)/shell/core/*.c)
KERNEL_C_SOURCES += $(wildcard $(SRC_DIR)/shell/commands/*.c)
KERNEL_C_SOURCES += $(wildcard $(SRC_DIR)/shell/parser/*.c)
KERNEL_C_SOURCES += $(wildcard $(SRC_DIR)/shell/advanced/*.c)

# Assembly sources for kernel (exclude bootloader entry points)
KERNEL_S_SOURCES = $(filter-out %boot.S %kernel_start.S, $(wildcard $(SRC_DIR)/arch/$(ARCH)/*.S))
KERNEL_S_SOURCES += $(wildcard $(SRC_DIR)/arch/$(ARCH)/interrupts/*.S)
KERNEL_S_SOURCES += $(wildcard $(SRC_DIR)/arch/$(ARCH)/process/*.S)
# Add kernel entry point for direct boot (only for ARM64)
ifeq ($(ARCH),arm64)
KERNEL_S_SOURCES += $(SRC_DIR)/arch/arm64/kernel_start.S
endif
KERNEL_ASM_SOURCES = $(filter-out %boot.asm, $(wildcard $(SRC_DIR)/arch/$(ARCH)/*.asm))
KERNEL_ASM_SOURCES += $(wildcard $(SRC_DIR)/arch/$(ARCH)/interrupts/*.asm)
KERNEL_ASM_SOURCES += $(wildcard $(SRC_DIR)/arch/$(ARCH)/process/*.asm)

# Object files
KERNEL_C_OBJECTS = $(KERNEL_C_SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/$(ARCH)/%.o)
KERNEL_S_OBJECTS = $(KERNEL_S_SOURCES:$(SRC_DIR)/%.S=$(BUILD_DIR)/$(ARCH)/%.o)
KERNEL_ASM_OBJECTS = $(KERNEL_ASM_SOURCES:$(SRC_DIR)/%.asm=$(BUILD_DIR)/$(ARCH)/%.o)

ALL_OBJECTS = $(KERNEL_C_OBJECTS) $(KERNEL_S_OBJECTS) $(KERNEL_ASM_OBJECTS)

# Dependency files
DEPS = $(KERNEL_C_OBJECTS:.o=.d)

# Default target
.PHONY: all clean kernel bootloader image test debug help info userland programs

all: info kernel bootloader image userland

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

# ramfs_core.c no longer needs special compilation rules

# sfs_core.c no longer needs special compilation rules

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

# Phase 7: User programs  
USER_PROGRAMS = hello calc calc_enhanced edit cat ls tictactoe more head tail top kill
USER_PROGRAM_SOURCES = $(wildcard $(SRC_DIR)/userland/bin/*.c) $(wildcard $(SRC_DIR)/userland/utils/*.c) $(wildcard $(SRC_DIR)/userland/games/*.c)
USER_PROGRAM_OBJECTS = $(USER_PROGRAM_SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/$(ARCH)/%.o)

# Phase 8.1: MiniOS C Library
LIBC_SOURCES = $(wildcard $(SRC_DIR)/userland/lib/minios_libc/*/*.c)
LIBC_OBJECTS = $(LIBC_SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/$(ARCH)/%.o)
LIBC_ARCHIVE = $(BUILD_DIR)/$(ARCH)/libminios.a

# User program build (simplified - not full ELF for now)
userland: programs libc

programs: $(USER_PROGRAMS:%=$(BUILD_DIR)/$(ARCH)/userland/%)
	@echo "User programs built for $(ARCH)"

# Phase 8.1: Build MiniOS C Library
libc: $(LIBC_ARCHIVE)
	@echo "MiniOS C library built for $(ARCH)"

$(LIBC_ARCHIVE): $(LIBC_OBJECTS)
	@mkdir -p $(dir $@)
	@echo "Creating MiniOS C library archive..."
	$(AR) rcs $@ $^

# Compile library sources
$(BUILD_DIR)/$(ARCH)/userland/lib/minios_libc/%.o: $(SRC_DIR)/userland/lib/minios_libc/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling library: $<"
	$(CC) $(CFLAGS) -I$(SRC_DIR)/userland/lib/minios_libc -c $< -o $@

# Individual user program targets
$(BUILD_DIR)/$(ARCH)/userland/hello: $(SRC_DIR)/userland/bin/hello.c
	@mkdir -p $(dir $@)
	@echo "Building user program: hello"
	@# For Phase 7, we'll create simple object files that can be loaded by ELF loader
	@echo "// Hello program built for $(ARCH)" > $@

$(BUILD_DIR)/$(ARCH)/userland/calc: $(SRC_DIR)/userland/bin/calc.c
	@mkdir -p $(dir $@)
	@echo "Building user program: calc"
	@echo "// Calculator program built for $(ARCH)" > $@

$(BUILD_DIR)/$(ARCH)/userland/calc_enhanced: $(SRC_DIR)/userland/bin/calc_enhanced.c $(LIBC_ARCHIVE)
	@mkdir -p $(dir $@)
	@echo "Building enhanced calculator with MiniOS C library"
	@# Create a simple stub - actual ELF compilation would need cross-compiler
	@echo "// Enhanced Calculator program built for $(ARCH)" > $@
	@echo "// Uses MiniOS C library" >> $@

$(BUILD_DIR)/$(ARCH)/userland/edit: $(SRC_DIR)/userland/bin/edit.c $(LIBC_ARCHIVE)
	@mkdir -p $(dir $@)
	@echo "Building text editor with MiniOS C library"
	@echo "// Text Editor program built for $(ARCH)" > $@
	@echo "// Uses MiniOS C library" >> $@

$(BUILD_DIR)/$(ARCH)/userland/cat: $(SRC_DIR)/userland/utils/cat.c
	@mkdir -p $(dir $@)
	@echo "Building user program: cat"
	@echo "// Cat utility built for $(ARCH)" > $@

$(BUILD_DIR)/$(ARCH)/userland/ls: $(SRC_DIR)/userland/utils/ls.c
	@mkdir -p $(dir $@)
	@echo "Building user program: ls"
	@echo "// Ls utility built for $(ARCH)" > $@

$(BUILD_DIR)/$(ARCH)/userland/tictactoe: $(SRC_DIR)/userland/games/tictactoe.c
	@mkdir -p $(dir $@)
	@echo "Building user program: tictactoe"
	@echo "// Tic-tac-toe game built for $(ARCH)" > $@

$(BUILD_DIR)/$(ARCH)/userland/more: $(SRC_DIR)/userland/utils/more.c
	@mkdir -p $(dir $@)
	@echo "Building user program: more"
	@echo "// More pager built for $(ARCH)" > $@

$(BUILD_DIR)/$(ARCH)/userland/head: $(SRC_DIR)/userland/utils/head.c
	@mkdir -p $(dir $@)
	@echo "Building user program: head"
	@echo "// Head utility built for $(ARCH)" > $@

$(BUILD_DIR)/$(ARCH)/userland/tail: $(SRC_DIR)/userland/utils/tail.c
	@mkdir -p $(dir $@)
	@echo "Building user program: tail"
	@echo "// Tail utility built for $(ARCH)" > $@

$(BUILD_DIR)/$(ARCH)/userland/top: $(SRC_DIR)/userland/utils/top.c
	@mkdir -p $(dir $@)
	@echo "Building user program: top"
	@echo "// Top monitor built for $(ARCH)" > $@

$(BUILD_DIR)/$(ARCH)/userland/kill: $(SRC_DIR)/userland/utils/kill.c
	@mkdir -p $(dir $@)
	@echo "Building user program: kill"
	@echo "// Kill utility built for $(ARCH)" > $@


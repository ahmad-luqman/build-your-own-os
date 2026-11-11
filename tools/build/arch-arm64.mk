# ARM64 (AArch64) Architecture Configuration

# Cross-compilation toolchain
CC = aarch64-elf-gcc
AS = aarch64-elf-as
LD = aarch64-elf-ld
OBJCOPY = aarch64-elf-objcopy
OBJDUMP = aarch64-elf-objdump
NM = aarch64-elf-nm
STRIP = aarch64-elf-strip

# Architecture-specific compiler flags
ARCH_CFLAGS = -mcpu=cortex-a72
ARCH_CFLAGS += -mgeneral-regs-only
ARCH_CFLAGS += -fno-stack-protector
ARCH_CFLAGS += -mstrict-align
# Disable SIMD vectorization that causes stack corruption in exception context
ARCH_CFLAGS += -fno-tree-vectorize
ARCH_CFLAGS += -fno-slp-vectorize
ARCH_CFLAGS += -fno-tree-loop-vectorize
ARCH_CFLAGS += -fno-tree-loop-distribute-patterns
# Force optimization level to -O0 for SFS and VFS to prevent SIMD generation
src/fs/sfs/sfs_core.o: CFLAGS := $(filter-out -O2,$(CFLAGS)) -O0
src/fs/vfs/vfs_core.o: CFLAGS := $(filter-out -O2,$(CFLAGS)) -O0

# Architecture-specific assembler flags
ASFLAGS = -mcpu=cortex-a72

# Add architecture flags to global flags
CFLAGS += $(ARCH_CFLAGS)

# Bootloader target
bootloader: $(BUILD_DIR)/$(ARCH)/bootloader.elf

# ARM64 bootloader objects
ARM64_BOOT_OBJECTS = $(BUILD_DIR)/$(ARCH)/arch/arm64/boot.o \
                     $(BUILD_DIR)/$(ARCH)/arch/arm64/uefi_boot.o

$(BUILD_DIR)/$(ARCH)/bootloader.elf: $(ARM64_BOOT_OBJECTS)
	@echo "Building ARM64 bootloader..."
	@mkdir -p $(dir $@)
	$(LD) -nostdlib -static -T $(SRC_DIR)/arch/arm64/boot.ld \
		-o $@ $(ARM64_BOOT_OBJECTS)
	@echo "Bootloader built: $@"

# ARM64 bootloader assembly
$(BUILD_DIR)/$(ARCH)/arch/arm64/boot.o: $(SRC_DIR)/arch/arm64/boot.S
	@echo "Assembling ARM64 bootloader entry..."
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

# ARM64 bootloader C code
$(BUILD_DIR)/$(ARCH)/arch/arm64/uefi_boot.o: $(SRC_DIR)/arch/arm64/uefi_boot.c
	@echo "Compiling ARM64 UEFI bootloader..."
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(SRC_DIR)/arch/arm64/include -c $< -o $@

# Create disk image
image: $(BUILD_DIR)/$(ARCH)/minios.img

$(BUILD_DIR)/$(ARCH)/minios.img: $(BUILD_DIR)/$(ARCH)/kernel.elf $(BUILD_DIR)/$(ARCH)/bootloader.elf
	@echo "Creating ARM64 disk image..."
	@mkdir -p $(dir $@)
	python3 $(TOOLS_DIR)/create_image.py --arch arm64 \
		--kernel $(BUILD_DIR)/$(ARCH)/kernel.elf \
		--bootloader $(BUILD_DIR)/$(ARCH)/bootloader.elf \
		--output $@
	@echo "Disk image created: $@"
	@ls -lh $@

# Architecture-specific debug info
arch-info:
	@echo "ARM64 Architecture Configuration:"
	@echo "  CPU: Cortex-A72"
	@echo "  Toolchain: aarch64-elf-*"
	@echo "  Page Size: 4KB"
	@echo "  Address Space: 48-bit"
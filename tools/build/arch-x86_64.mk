# x86-64 (AMD64) Architecture Configuration

# Cross-compilation toolchain
CC = x86_64-elf-gcc
AS = nasm
LD = x86_64-elf-ld
OBJCOPY = x86_64-elf-objcopy
OBJDUMP = x86_64-elf-objdump
NM = x86_64-elf-nm
STRIP = x86_64-elf-strip

# Architecture-specific compiler flags
ARCH_CFLAGS = -mcmodel=kernel
ARCH_CFLAGS += -mno-red-zone
ARCH_CFLAGS += -mno-mmx -mno-sse -mno-sse2
ARCH_CFLAGS += -fno-stack-protector

# Architecture-specific assembler flags (NASM)
ASFLAGS = -f elf64

# Bootloader target
bootloader: $(BUILD_DIR)/$(ARCH)/bootloader.elf

# x86-64 bootloader objects
X86_64_BOOT_OBJECTS = $(BUILD_DIR)/$(ARCH)/arch/x86_64/boot.o \
                      $(BUILD_DIR)/$(ARCH)/arch/x86_64/boot_main.o

$(BUILD_DIR)/$(ARCH)/bootloader.elf: $(X86_64_BOOT_OBJECTS)
	@echo "Building x86-64 bootloader..."
	@mkdir -p $(dir $@)
	$(LD) -nostdlib -static -T $(SRC_DIR)/arch/x86_64/bootloader.ld \
		-o $@ $(X86_64_BOOT_OBJECTS)
	@echo "Bootloader built: $@"

# x86-64 bootloader assembly  
$(BUILD_DIR)/$(ARCH)/arch/x86_64/boot.o: $(SRC_DIR)/arch/x86_64/boot.asm
	@echo "Assembling x86-64 bootloader entry..."
	@mkdir -p $(dir $@)
	$(AS) -f elf64 $< -o $@

# x86-64 bootloader C code
$(BUILD_DIR)/$(ARCH)/arch/x86_64/boot_main.o: $(SRC_DIR)/arch/x86_64/boot_main.c
	@echo "Compiling x86-64 Multiboot2 bootloader..."
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(SRC_DIR)/arch/x86_64/include -c $< -o $@

# Create ISO image
image: $(BUILD_DIR)/$(ARCH)/minios.iso

$(BUILD_DIR)/$(ARCH)/minios.iso: $(BUILD_DIR)/$(ARCH)/kernel.elf $(BUILD_DIR)/$(ARCH)/bootloader.elf
	@echo "Creating x86-64 ISO image..."
	@mkdir -p $(dir $@)
	python3 $(TOOLS_DIR)/create_iso.py \
		--kernel $(BUILD_DIR)/$(ARCH)/kernel.elf \
		--bootloader $(BUILD_DIR)/$(ARCH)/bootloader.elf \
		--output $@
	@echo "ISO image created: $@"
	@ls -lh $@

# Handle NASM assembly files
$(BUILD_DIR)/$(ARCH)/%.o: $(SRC_DIR)/%.asm
	@echo "Assembling $< (NASM)..."
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# Architecture-specific debug info
arch-info:
	@echo "x86-64 Architecture Configuration:"
	@echo "  CPU: x86-64 compatible"
	@echo "  Toolchain: x86_64-elf-* + nasm"
	@echo "  Page Size: 4KB"
	@echo "  Address Space: 48-bit"

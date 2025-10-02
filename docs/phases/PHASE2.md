# Phase 2 Implementation Guide: Enhanced Bootloaders

## Overview

Phase 2 enhances MiniOS with fully functional bootloaders that provide rich boot information to the kernel. This phase implements:

- **Enhanced ARM64 UEFI Bootloader** - Proper UEFI application with memory detection and graphics setup
- **Enhanced x86-64 Multiboot2 Bootloader** - Full Multiboot2 compliance with boot information parsing
- **Cross-Platform Boot Protocol** - Unified interface for kernel boot information
- **Boot Information Structures** - Memory maps, graphics info, and system data
- **Architecture Abstraction** - Clean separation of platform-specific bootloader code

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    MiniOS Boot Process                      │
├─────────────────────────────────────────────────────────────┤
│  ARM64 (UEFI/UTM)          │  x86-64 (Multiboot2/QEMU)    │
├─────────────────────────────────────────────────────────────┤
│  1. UEFI Firmware           │  1. GRUB/Multiboot2 Loader   │
│  2. bootloader.elf          │  2. bootloader.elf            │
│     - boot.S (entry)        │     - boot.asm (entry)        │
│     - uefi_boot.c (main)    │     - boot_main.c (main)      │
│  3. Memory Detection        │  3. Parse Multiboot2 Info     │
│  4. Graphics Setup          │  4. Memory Map Processing     │
│  5. Boot Info Creation      │  5. Boot Info Creation        │
│  6. Transfer to Kernel      │  6. Transfer to Kernel        │
├─────────────────────────────────────────────────────────────┤
│              Common Boot Protocol (boot_info)              │
│  - Magic number validation  │  - Memory map entries         │
│  - Architecture detection   │  - Graphics/framebuffer info │
│  - Cross-platform interface│  - Command line parameters    │
└─────────────────────────────────────────────────────────────┘
```

## Implementation Details

### Cross-Platform Boot Protocol

The boot protocol provides a unified interface between bootloaders and kernel:

```c
// src/include/boot_protocol.h
#define BOOT_PROTOCOL_MAGIC 0x4D696E694F53ULL  // "MiniOS"

struct boot_info {
    uint64_t magic;                    // Protocol validation
    uint32_t arch;                     // BOOT_ARCH_ARM64 or BOOT_ARCH_X86_64  
    uint32_t version;                  // Protocol version
    
    struct memory_map_entry *memory_map;
    uint32_t memory_map_entries;
    uint32_t memory_map_size;
    
    struct fb_info framebuffer;        // Graphics information
    
    uint64_t kernel_start;             // Kernel load address
    uint64_t kernel_size;              // Kernel size
    
    char cmdline[256];                 // Command line
    void *arch_specific;               // Architecture-specific data
};
```

### ARM64 UEFI Bootloader

**Entry Point (`boot.S`):**
- UEFI entry point receives `image_handle` and `system_table`
- Sets up initial stack and clears BSS
- Calls C bootloader main function

**Main Implementation (`uefi_boot.c`):**
- `uefi_main()` - UEFI application entry point
- `get_memory_map()` - Query UEFI memory services
- `setup_graphics()` - Initialize graphics output protocol
- `setup_boot_info()` - Create unified boot information
- `transfer_to_kernel()` - Hand off to kernel (future: load from storage)

**Key Features:**
- Proper UEFI application structure
- Memory map detection using UEFI services
- Graphics framebuffer setup
- Boot information structure creation
- Clean handoff preparation

### x86-64 Multiboot2 Bootloader

**Entry Point (`boot.asm`):**
- Multiboot2 header for GRUB compatibility
- 32-bit protected mode entry
- Long mode (64-bit) transition
- Page table setup and CPU feature detection
- BSS clearing and C function call

**Main Implementation (`boot_main.c`):**
- `boot_main()` - Called after long mode setup
- `parse_multiboot2_info()` - Parse bootloader-provided information
- `parse_memory_map()` - Extract memory map from Multiboot2
- `parse_framebuffer()` - Extract graphics information
- `setup_boot_info()` - Create unified boot structure

**Key Features:**
- Full Multiboot2 compliance
- Memory map parsing and conversion
- Graphics mode detection
- Boot parameter extraction
- Unified boot info creation

## Build System Integration

### Architecture-Specific Makefiles

**ARM64 (`tools/build/arch-arm64.mk`):**
```makefile
# Bootloader objects
ARM64_BOOT_OBJECTS = $(BUILD_DIR)/$(ARCH)/arch/arm64/boot.o \
                     $(BUILD_DIR)/$(ARCH)/arch/arm64/uefi_boot.o

$(BUILD_DIR)/$(ARCH)/bootloader.elf: $(ARM64_BOOT_OBJECTS)
    $(LD) -nostdlib -static -T $(SRC_DIR)/arch/arm64/boot.ld \
          -o $@ $(ARM64_BOOT_OBJECTS)
```

**x86-64 (`tools/build/arch-x86_64.mk`):**
```makefile
# Bootloader objects
X86_64_BOOT_OBJECTS = $(BUILD_DIR)/$(ARCH)/arch/x86_64/boot.o \
                      $(BUILD_DIR)/$(ARCH)/arch/x86_64/boot_main.o

$(BUILD_DIR)/$(ARCH)/bootloader.elf: $(X86_64_BOOT_OBJECTS)
    $(LD) -nostdlib -static -T $(SRC_DIR)/arch/x86_64/bootloader.ld \
          -o $@ $(X86_64_BOOT_OBJECTS)
```

### Separated Builds

Phase 2 properly separates bootloader and kernel builds:

- **Bootloaders** are standalone ELF files that can boot independently
- **Kernels** are separate ELF files loaded by bootloaders  
- **No circular dependencies** between bootloader and kernel code
- **Clean linking** with appropriate linker scripts for each component

## Usage

### Building Phase 2

```bash
# Build individual components
make ARCH=arm64 bootloader    # ARM64 UEFI bootloader
make ARCH=x86_64 bootloader   # x86-64 Multiboot2 bootloader
make ARCH=arm64 kernel        # ARM64 kernel  
make ARCH=x86_64 kernel       # x86-64 kernel

# Build complete systems
make ARCH=arm64 all           # Bootloader + kernel + disk image
make ARCH=x86_64 all          # Bootloader + kernel + ISO image

# Test builds
./tools/quick-test-phase2.sh  # Quick functionality test
./tools/test-phase2.sh        # Comprehensive test suite
```

### Output Files

**ARM64 Build:**
- `build/arm64/bootloader.elf` - UEFI bootloader application
- `build/arm64/kernel.elf` - ARM64 kernel  
- `build/arm64/minios.img` - Bootable disk image for UTM

**x86-64 Build:**
- `build/x86_64/bootloader.elf` - Multiboot2 bootloader
- `build/x86_64/kernel.elf` - x86-64 kernel
- `build/x86_64/minios.iso` - Bootable ISO image for QEMU/VirtualBox

### Testing in VMs

```bash
# UTM (ARM64 on macOS)
make ARCH=arm64 test

# QEMU (x86-64 cross-platform)  
make ARCH=x86_64 test
```

## Boot Information Flow

### ARM64 UEFI Flow

1. **UEFI Firmware** loads `bootloader.elf` as UEFI application
2. **Boot Entry** (`_start` in `boot.S`) receives UEFI parameters
3. **UEFI Main** (`uefi_main`) initializes and gathers system info:
   - Memory map from `GetMemoryMap()`
   - Graphics info from GOP (Graphics Output Protocol)  
   - Command line from loaded image protocol
4. **Boot Info Setup** creates unified `boot_info` structure
5. **Kernel Transfer** (future: load kernel ELF and jump to entry point)

### x86-64 Multiboot2 Flow

1. **GRUB** loads `bootloader.elf` according to Multiboot2 spec
2. **Boot Entry** (`_start` in `boot.asm`) transitions to long mode
3. **Boot Main** (`boot_main`) parses Multiboot2 information:
   - Memory map from Multiboot2 tags
   - Graphics info from framebuffer tags
   - Command line from string tags  
4. **Boot Info Setup** converts to unified `boot_info` structure
5. **Kernel Transfer** (future: load kernel ELF and jump to entry point)

## Kernel Integration

The kernel now receives rich boot information:

```c
// Enhanced kernel_main signature
void kernel_main(struct boot_info *boot_info) {
    // Validate boot info
    if (boot_info && boot_info_valid(boot_info)) {
        // Display system information
        display_memory_info(boot_info->memory_map, boot_info->memory_map_entries);
        display_graphics_info(&boot_info->framebuffer);
        
        // Use boot parameters
        if (boot_info->cmdline[0]) {
            parse_command_line(boot_info->cmdline);
        }
    }
    
    // Continue with kernel initialization...
}
```

## Future Enhancements (Phase 3+)

Phase 2 bootloaders are designed for extension:

- **Kernel Loading** - Load kernel ELF files from storage (filesystem)
- **Module Loading** - Support for initrd/initramfs and kernel modules  
- **Advanced Memory Management** - Set up page tables and memory protection
- **Device Tree** - ARM64 device tree parsing and setup
- **ACPI/UEFI Services** - Advanced system configuration
- **Secure Boot** - Cryptographic verification of loaded components

## Testing and Validation

### Automated Testing

Phase 2 includes comprehensive test coverage:

```bash
./tools/test-phase2.sh    # Full test suite (20 tests)
./tools/quick-test-phase2.sh  # Quick functionality test (6 tests)
./tools/test-phase1.sh    # Verify Phase 1 still works
```

### Test Categories

1. **Build System Tests** - Verify all components build correctly
2. **Boot Protocol Tests** - Validate headers and constants
3. **Implementation Tests** - Check bootloader functionality  
4. **Integration Tests** - Kernel and bootloader compatibility
5. **File Structure Tests** - Verify output artifacts
6. **Size and Sanity Tests** - Check reasonable file sizes

### Success Criteria

- ✅ All bootloaders build without errors
- ✅ All kernels build without errors  
- ✅ Boot protocol headers are complete
- ✅ Bootable images are created successfully
- ✅ Boot information structures are validated
- ✅ Phase 1 functionality is preserved

## Troubleshooting

### Common Issues

**Build Errors:**
- Ensure cross-compilation toolchains are installed
- Check that architecture-specific headers are included properly
- Verify linker scripts have correct symbol definitions

**Linking Issues:**
- Bootloaders and kernels must be built separately  
- Check that bootloader-specific files are excluded from kernel builds
- Verify external symbols are declared correctly in assembly files

**Boot Protocol Issues:**
- Ensure magic numbers match between bootloader and kernel
- Check structure packing and alignment
- Verify architecture identification is correct

### Debug Builds

```bash
# Build with debug information
make DEBUG=1 ARCH=arm64 all
make DEBUG=1 ARCH=x86_64 all

# Debug with GDB (x86-64)
make debug ARCH=x86_64
```

## Architecture Notes

### ARM64 Specifics

- **UEFI Environment** - Standard UEFI application model
- **Memory Layout** - Assumes 128MB RAM at 0x40000000
- **Graphics** - Simplified GOP (Graphics Output Protocol) setup
- **Entry Point** - Standard UEFI application entry with image_handle/system_table

### x86-64 Specifics  

- **Multiboot2** - Full compliance with Multiboot2 specification
- **Long Mode** - Proper 64-bit transition with page tables
- **Memory Detection** - Uses Multiboot2 memory map tags
- **Graphics** - VGA text mode and framebuffer support

### Cross-Platform Design

The boot protocol abstracts architecture differences:
- **Common Structures** - Memory maps use unified format
- **Architecture Detection** - Runtime identification of platform
- **Extensible Design** - Easy to add new architectures
- **Clean Interfaces** - Minimal architecture-specific code in common components

---

**Phase 2 Complete!** 🎉

The bootloader implementation provides a solid foundation for kernel loading and system initialization across both ARM64 and x86-64 platforms.
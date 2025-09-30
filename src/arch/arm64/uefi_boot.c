/*
 * ARM64 UEFI Bootloader
 * UEFI application for loading MiniOS on ARM64
 */

#include <stdint.h>
#include <stddef.h>
#include "boot_info_arm64.h"

// Simple UEFI types and constants
#define EFI_SUCCESS              0
#define EFI_LOAD_ERROR           1
#define EFI_INVALID_PARAMETER    2
#define EFI_OUT_OF_RESOURCES     9

typedef struct {
    uint32_t Type;
    uint64_t PhysicalStart;
    uint64_t VirtualStart;
    uint64_t NumberOfPages;
    uint64_t Attribute;
} EFI_MEMORY_DESCRIPTOR;

// Global boot information
static struct boot_info g_boot_info;
static struct boot_info_arm64 g_arm64_boot_info;

// Simple string functions (remove unused ones)
static void strcpy_simple(char* dest, const char* src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = 0;
}

// UEFI entry point
EFI_STATUS uefi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table) {
    // Initialize boot info structures
    for (size_t i = 0; i < sizeof(g_boot_info); i++) {
        ((char*)&g_boot_info)[i] = 0;
    }
    for (size_t i = 0; i < sizeof(g_arm64_boot_info); i++) {
        ((char*)&g_arm64_boot_info)[i] = 0;
    }
    
    // Set up basic information
    g_boot_info.magic = BOOT_PROTOCOL_MAGIC;
    g_boot_info.arch = BOOT_ARCH_ARM64;
    g_boot_info.version = BOOT_PROTOCOL_VERSION;
    
    g_arm64_boot_info.magic = BOOT_PROTOCOL_MAGIC;
    g_arm64_boot_info.image_handle = image_handle;
    g_arm64_boot_info.system_table = system_table;
    
    // Get memory map (simplified for now)
    EFI_STATUS status = get_memory_map(&g_arm64_boot_info);
    if (status != EFI_SUCCESS) {
        return status;
    }
    
    // Set up graphics (simplified for now)
    status = setup_graphics(&g_arm64_boot_info);
    if (status != EFI_SUCCESS) {
        // Graphics failure is not fatal, continue
    }
    
    // Set up unified boot info
    setup_boot_info(&g_boot_info, &g_arm64_boot_info);
    
    // Set default command line
    strcpy_simple(g_boot_info.cmdline, "console=uart,mmio,0x9000000");
    
    // Transfer to kernel
    transfer_to_kernel(&g_boot_info);
    
    // Should never return
    return EFI_LOAD_ERROR;
}

EFI_STATUS get_memory_map(struct boot_info_arm64 *boot_info) {
    // For now, create a simple memory map
    // In a real implementation, we'd call UEFI GetMemoryMap()
    
    // Assume we have 128MB RAM starting at 0x40000000 (typical ARM64 setup)
    static struct memory_map_entry simple_memory_map[] = {
        { 0x40000000, 0x8000000, MEMORY_TYPE_AVAILABLE, 0 },  // 128MB RAM
    };
    
    boot_info->uefi_memory_map = simple_memory_map;
    boot_info->memory_map_size = sizeof(simple_memory_map);
    boot_info->descriptor_size = sizeof(struct memory_map_entry);
    
    return EFI_SUCCESS;
}

EFI_STATUS setup_graphics(struct boot_info_arm64 *boot_info) {
    // Set up basic framebuffer info
    // In a real implementation, we'd query UEFI GOP protocol
    boot_info->graphics.width = 1024;
    boot_info->graphics.height = 768;
    boot_info->graphics.bpp = 32;
    boot_info->graphics.format = FB_FORMAT_RGBX;
    boot_info->graphics.pitch = 1024 * 4;
    boot_info->graphics.framebuffer = 0;  // No framebuffer for now
    
    return EFI_SUCCESS;
}

void setup_boot_info(struct boot_info *boot_info, struct boot_info_arm64 *arm64_info) {
    // Copy graphics info
    boot_info->framebuffer = arm64_info->graphics;
    
    // Set up memory map
    boot_info->memory_map = (struct memory_map_entry*)arm64_info->uefi_memory_map;
    boot_info->memory_map_entries = arm64_info->memory_map_size / arm64_info->descriptor_size;
    boot_info->memory_map_size = arm64_info->memory_map_size;
    
    // Set kernel info
    boot_info->kernel_start = 0x40080000;  // Load kernel at 512KB offset
    boot_info->kernel_size = 0;  // Will be filled when we actually load kernel
    
    // Architecture-specific data
    boot_info->arch_specific = arm64_info;
}

void transfer_to_kernel(struct boot_info *boot_info) {
    // Suppress unused parameter warning
    (void)boot_info;
    
    // In a real bootloader, we'd:
    // 1. Load kernel ELF from storage
    // 2. Exit UEFI boot services  
    // 3. Set up MMU
    // 4. Jump to kernel entry point
    
    // For now, just demonstrate we have the boot info
    // The actual kernel loading will be implemented in Phase 3
    
    // Halt - in a real implementation we'd jump to loaded kernel
    while (1) {
        __asm__ volatile("wfi");
    }
}

// Assembly entry point will call this
void _start_c(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table) {
    uefi_main(image_handle, system_table);
    
    // Should never reach here
    while (1) {
        __asm__ volatile("wfi");
    }
}
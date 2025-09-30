/*
 * ARM64-specific Boot Information
 * UEFI boot environment structures
 */

#ifndef BOOT_INFO_ARM64_H
#define BOOT_INFO_ARM64_H

#include <stdint.h>
#include <stddef.h>
#include "boot_protocol.h"

// UEFI types for minimal compatibility
typedef void* EFI_HANDLE;
typedef uint64_t EFI_STATUS;
typedef void* EFI_SYSTEM_TABLE;
typedef void* EFI_BOOT_SERVICES;

// ARM64-specific boot information
struct boot_info_arm64 {
    uint64_t magic;              // Should be BOOT_PROTOCOL_MAGIC
    
    // UEFI information
    EFI_HANDLE image_handle;
    EFI_SYSTEM_TABLE *system_table;
    
    // Memory map from UEFI
    void *uefi_memory_map;
    uint64_t memory_map_size;
    uint64_t memory_map_key;
    uint64_t descriptor_size;
    uint32_t descriptor_version;
    
    // Graphics information
    struct fb_info graphics;
    
    // Kernel loading information
    void *kernel_base;
    uint64_t kernel_size;
    void *kernel_entry;
    
    // Device tree (if available)
    void *device_tree;
    uint64_t device_tree_size;
    
} __attribute__((packed));

// Function declarations for ARM64 bootloader
EFI_STATUS uefi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table);
void setup_boot_info(struct boot_info *boot_info, struct boot_info_arm64 *arm64_info);
EFI_STATUS get_memory_map(struct boot_info_arm64 *boot_info);
EFI_STATUS setup_graphics(struct boot_info_arm64 *boot_info);
void transfer_to_kernel(struct boot_info *boot_info);

#endif // BOOT_INFO_ARM64_H
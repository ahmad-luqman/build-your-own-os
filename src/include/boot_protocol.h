/*
 * MiniOS Boot Protocol
 * Cross-platform boot information structure
 */

#ifndef BOOT_PROTOCOL_H
#define BOOT_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

// Boot protocol magic number
#define BOOT_PROTOCOL_MAGIC 0x4D696E694F53ULL  // "MiniOS"

// Architecture identifiers
#define BOOT_ARCH_ARM64   0x1
#define BOOT_ARCH_X86_64  0x2

// Memory region types
#define MEMORY_TYPE_AVAILABLE    1
#define MEMORY_TYPE_RESERVED     2
#define MEMORY_TYPE_ACPI_RECLAIM 3
#define MEMORY_TYPE_ACPI_NVS     4
#define MEMORY_TYPE_BAD          5

// Framebuffer pixel formats
#define FB_FORMAT_RGB  0
#define FB_FORMAT_BGR  1
#define FB_FORMAT_RGBX 2
#define FB_FORMAT_BGRX 3

// Memory map entry
struct memory_map_entry {
    uint64_t base;           // Base address
    uint64_t length;         // Length in bytes
    uint32_t type;           // Memory type
    uint32_t attributes;     // Architecture-specific attributes
} __attribute__((packed));

// Framebuffer information
struct fb_info {
    uint32_t width;          // Width in pixels
    uint32_t height;         // Height in pixels
    uint32_t pitch;          // Bytes per line
    uint32_t bpp;            // Bits per pixel
    uint32_t format;         // Pixel format
    uint64_t framebuffer;    // Physical address of framebuffer
} __attribute__((packed));

// Main boot information structure
struct boot_info {
    uint64_t magic;          // BOOT_PROTOCOL_MAGIC
    uint32_t arch;           // Architecture (BOOT_ARCH_*)
    uint32_t version;        // Boot protocol version
    
    // Memory information
    struct memory_map_entry *memory_map;
    uint32_t memory_map_entries;
    uint32_t memory_map_size;
    
    // Graphics information
    struct fb_info framebuffer;
    
    // Kernel information
    uint64_t kernel_start;   // Physical start address of kernel
    uint64_t kernel_size;    // Size of kernel in bytes
    
    // Command line
    char cmdline[256];       // Kernel command line
    
    // Architecture-specific data pointer
    void *arch_specific;
    
} __attribute__((packed));

// Boot protocol version
#define BOOT_PROTOCOL_VERSION 1

// Validation function
static inline int boot_info_valid(struct boot_info *info) {
    return (info && info->magic == BOOT_PROTOCOL_MAGIC && 
            info->version == BOOT_PROTOCOL_VERSION);
}

#endif // BOOT_PROTOCOL_H
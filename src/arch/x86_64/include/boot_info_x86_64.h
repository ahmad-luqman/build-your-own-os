/*
 * x86-64-specific Boot Information
 * Multiboot2 boot environment structures
 */

#ifndef BOOT_INFO_X86_64_H
#define BOOT_INFO_X86_64_H

#include <stdint.h>
#include <stddef.h>
#include "boot_protocol.h"

// Multiboot2 tag types
#define MULTIBOOT_TAG_TYPE_END               0
#define MULTIBOOT_TAG_TYPE_CMDLINE           1
#define MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME  2
#define MULTIBOOT_TAG_TYPE_MODULE            3
#define MULTIBOOT_TAG_TYPE_BASIC_MEMINFO     4
#define MULTIBOOT_TAG_TYPE_BOOTDEV           5
#define MULTIBOOT_TAG_TYPE_MMAP              6
#define MULTIBOOT_TAG_TYPE_VBE               7
#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER       8
#define MULTIBOOT_TAG_TYPE_ELF_SECTIONS      9
#define MULTIBOOT_TAG_TYPE_APM               10
#define MULTIBOOT_TAG_TYPE_EFI32             11
#define MULTIBOOT_TAG_TYPE_EFI64             12
#define MULTIBOOT_TAG_TYPE_SMBIOS            13
#define MULTIBOOT_TAG_TYPE_ACPI_OLD          14
#define MULTIBOOT_TAG_TYPE_ACPI_NEW          15
#define MULTIBOOT_TAG_TYPE_NETWORK           16
#define MULTIBOOT_TAG_TYPE_EFI_MMAP          17
#define MULTIBOOT_TAG_TYPE_EFI_BS            18

// Multiboot2 structures
struct multiboot_tag {
    uint32_t type;
    uint32_t size;
} __attribute__((packed));

struct multiboot_tag_string {
    struct multiboot_tag tag;
    char string[0];
} __attribute__((packed));

struct multiboot_mmap_entry {
    uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t zero;
} __attribute__((packed));

struct multiboot_tag_mmap {
    struct multiboot_tag tag;
    uint32_t entry_size;
    uint32_t entry_version;
    struct multiboot_mmap_entry entries[0];
} __attribute__((packed));

struct multiboot_tag_framebuffer {
    struct multiboot_tag tag;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint16_t reserved;
} __attribute__((packed));

// x86-64-specific boot information
struct boot_info_x86_64 {
    uint64_t magic;              // Should be BOOT_PROTOCOL_MAGIC
    
    // Multiboot2 information
    uint32_t multiboot_magic;    // Multiboot2 magic from bootloader
    void *multiboot_info;        // Pointer to Multiboot2 info structure
    
    // Parsed memory map
    struct memory_map_entry *memory_map;
    uint32_t memory_map_entries;
    
    // Graphics information
    struct fb_info graphics;
    
    // Kernel information
    void *kernel_base;
    uint64_t kernel_size;
    
    // Command line
    char *cmdline;
    
} __attribute__((packed));

// Function declarations for x86-64 bootloader
void parse_multiboot2_info(uint32_t magic, void *info, struct boot_info_x86_64 *boot_info);
void setup_boot_info(struct boot_info *boot_info, struct boot_info_x86_64 *x86_info);
void parse_memory_map(struct multiboot_tag_mmap *mmap_tag, struct boot_info_x86_64 *boot_info);
void parse_framebuffer(struct multiboot_tag_framebuffer *fb_tag, struct boot_info_x86_64 *boot_info);

// External symbols from linker script
extern uint32_t __bss_start;
extern uint32_t __bss_end;
extern uint32_t __kernel_start;
extern uint32_t __kernel_end;

#endif // BOOT_INFO_X86_64_H
/*
 * x86-64 Bootloader Main
 * Multiboot2 compliant bootloader
 */

#include <stdint.h>
#include <stddef.h>
#include "boot_info_x86_64.h"

// Global variables for multiboot info
static uint32_t g_multiboot_magic = 0;
static void *g_multiboot_info = NULL;
static struct boot_info g_boot_info;
static struct boot_info_x86_64 g_x86_boot_info;

// Forward declarations
void parse_multiboot2_info(uint32_t magic, void *info, struct boot_info_x86_64 *boot_info);
void setup_boot_info(struct boot_info *boot_info, struct boot_info_x86_64 *x86_info);

// Called from assembly after entering long mode
void boot_main(uint32_t multiboot_magic, void *multiboot_info) {
    // Initialize boot info structures
    g_multiboot_magic = multiboot_magic;
    g_multiboot_info = multiboot_info;
    
    // Clear boot info structures
    for (size_t i = 0; i < sizeof(g_boot_info); i++) {
        ((char*)&g_boot_info)[i] = 0;
    }
    for (size_t i = 0; i < sizeof(g_x86_boot_info); i++) {
        ((char*)&g_x86_boot_info)[i] = 0;
    }
    
    // Set up magic numbers
    g_boot_info.magic = BOOT_PROTOCOL_MAGIC;
    g_boot_info.arch = BOOT_ARCH_X86_64;
    g_boot_info.version = BOOT_PROTOCOL_VERSION;
    
    g_x86_boot_info.magic = BOOT_PROTOCOL_MAGIC;
    g_x86_boot_info.multiboot_magic = multiboot_magic;
    g_x86_boot_info.multiboot_info = multiboot_info;
    
    // Parse Multiboot2 information
    if (multiboot_magic == 0x36d76289) {  // Multiboot2 magic
        parse_multiboot2_info(multiboot_magic, multiboot_info, &g_x86_boot_info);
    }
    
    // Set up unified boot info
    setup_boot_info(&g_boot_info, &g_x86_boot_info);
    
    // Transfer to kernel
    // For now, just demonstrate we have the boot info
    // The actual kernel would be loaded from storage in a real bootloader
    
    // Halt - in a real implementation we'd jump to loaded kernel  
    while (1) {
        __asm__ volatile("hlt");
    }
    
    // Should never reach here
    while (1) {
        __asm__ volatile("hlt");
    }
}

void parse_multiboot2_info(uint32_t magic, void *info, struct boot_info_x86_64 *boot_info) {
    if (magic != 0x36d76289 || !info) {
        return;
    }
    
    // Parse Multiboot2 tags
    struct multiboot_tag *tag = (struct multiboot_tag*)((char*)info + 8);
    
    while (tag->type != MULTIBOOT_TAG_TYPE_END) {
        switch (tag->type) {
            case MULTIBOOT_TAG_TYPE_CMDLINE: {
                struct multiboot_tag_string *cmdline_tag = (struct multiboot_tag_string*)tag;
                boot_info->cmdline = cmdline_tag->string;
                break;
            }
            
            case MULTIBOOT_TAG_TYPE_MMAP: {
                struct multiboot_tag_mmap *mmap_tag = (struct multiboot_tag_mmap*)tag;
                parse_memory_map(mmap_tag, boot_info);
                break;
            }
            
            case MULTIBOOT_TAG_TYPE_FRAMEBUFFER: {
                struct multiboot_tag_framebuffer *fb_tag = (struct multiboot_tag_framebuffer*)tag;
                parse_framebuffer(fb_tag, boot_info);
                break;
            }
        }
        
        // Move to next tag (align to 8 bytes)
        tag = (struct multiboot_tag*)((char*)tag + ((tag->size + 7) & ~7));
    }
}

void parse_memory_map(struct multiboot_tag_mmap *mmap_tag, struct boot_info_x86_64 *boot_info) {
    if (!mmap_tag) return;
    
    // For now, just count entries and store pointer
    // In a full implementation, we'd convert to our format
    uint32_t entry_count = 0;
    struct multiboot_mmap_entry *entry = mmap_tag->entries;
    
    while ((char*)entry < (char*)mmap_tag + mmap_tag->tag.size) {
        entry_count++;
        entry = (struct multiboot_mmap_entry*)((char*)entry + mmap_tag->entry_size);
    }
    
    // Store raw multiboot memory map for now
    boot_info->memory_map = (struct memory_map_entry*)mmap_tag->entries;
    boot_info->memory_map_entries = entry_count;
}

void parse_framebuffer(struct multiboot_tag_framebuffer *fb_tag, struct boot_info_x86_64 *boot_info) {
    if (!fb_tag) return;
    
    boot_info->graphics.framebuffer = fb_tag->framebuffer_addr;
    boot_info->graphics.width = fb_tag->framebuffer_width;
    boot_info->graphics.height = fb_tag->framebuffer_height;
    boot_info->graphics.pitch = fb_tag->framebuffer_pitch;
    boot_info->graphics.bpp = fb_tag->framebuffer_bpp;
    boot_info->graphics.format = (fb_tag->framebuffer_type == 1) ? FB_FORMAT_RGB : FB_FORMAT_BGR;
}

void setup_boot_info(struct boot_info *boot_info, struct boot_info_x86_64 *x86_info) {
    // Copy graphics info
    boot_info->framebuffer = x86_info->graphics;
    
    // Copy memory map (convert format if needed)
    boot_info->memory_map = x86_info->memory_map;
    boot_info->memory_map_entries = x86_info->memory_map_entries;
    boot_info->memory_map_size = x86_info->memory_map_entries * sizeof(struct memory_map_entry);
    
    // Copy command line
    if (x86_info->cmdline) {
        int i;
        for (i = 0; i < 255 && x86_info->cmdline[i]; i++) {
            boot_info->cmdline[i] = x86_info->cmdline[i];
        }
        boot_info->cmdline[i] = 0;
    } else {
        boot_info->cmdline[0] = 0;
    }
    
    // Set kernel info (will be filled in by actual kernel loading)
    boot_info->kernel_start = 0;
    boot_info->kernel_size = 0;
    
    // Architecture-specific data
    boot_info->arch_specific = x86_info;
}
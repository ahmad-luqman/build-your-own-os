/*
 * MiniOS Cross-Platform Memory Management Implementation
 * Phase 3: Memory Management & Kernel Loading
 */

#include "memory.h"
#include "kernel.h"

// Memory subsystem initialization state
static int memory_initialized = 0;

// Helper function to print single hex digit
static void print_hex_digit(int digit)
{
    if (digit < 10) {
        char ch[2] = {'0' + digit, 0};
        early_print(ch);
    } else {
        char ch[2] = {'A' + digit - 10, 0};
        early_print(ch);
    }
}

/**
 * Initialize memory management subsystem
 */
int memory_init(struct boot_info *boot_info)
{
    if (memory_initialized) {
        return 0;  // Already initialized
    }
    
    if (!boot_info || !boot_info_valid(boot_info)) {
        early_print("Memory init: Invalid boot info\n");
        return -1;
    }
    
    early_print("Initializing memory management...\n");
    
    // Initialize architecture-specific memory management
    if (arch_memory_init(boot_info) < 0) {
        early_print("Memory init: Architecture init failed\n");
        return -1;
    }
    
    // Initialize physical memory allocator
    if (boot_info->memory_map && boot_info->memory_map_entries > 0) {
        arch_memory_allocator_init(boot_info->memory_map, 
                                   boot_info->memory_map_entries);
        early_print("Memory allocator initialized\n");
    } else {
        early_print("Memory init: No memory map available\n");
        return -1;
    }
    
    // Enable memory management (MMU/paging)
    arch_memory_enable();
    early_print("Memory management enabled\n");
    
    memory_initialized = 1;
    
    // Show memory layout for debugging
    memory_show_layout(boot_info);
    
    return 0;
}

/**
 * Map physical memory to virtual address space
 */
void *memory_map(uint64_t phys_addr, size_t size, uint32_t flags)
{
    if (!memory_initialized || size == 0) {
        return NULL;
    }
    
    // For Phase 3, return identity mapped address
    // Real implementation would allocate virtual address space
    if (arch_memory_map_pages(phys_addr, phys_addr, size, flags) < 0) {
        return NULL;
    }
    
    return (void *)phys_addr;
}

/**
 * Unmap virtual memory
 */
void memory_unmap(void *virt_addr, size_t size)
{
    if (!memory_initialized || !virt_addr || size == 0) {
        return;
    }
    
    // For Phase 3, this is a no-op since we use identity mapping
    // Real implementation would update page tables and free virtual space
}

/**
 * Allocate memory
 */
void *memory_alloc(size_t size, uint32_t alignment)
{
    if (!memory_initialized) {
        return NULL;
    }
    
    // For Phase 3, ignore alignment requirements
    (void)alignment;  // Suppress unused parameter warning
    
    return memory_alloc_pages((size + PAGE_SIZE_4K - 1) / PAGE_SIZE_4K);
}

/**
 * Free memory
 */
void memory_free(void *ptr)
{
    if (!memory_initialized || !ptr) {
        return;
    }
    
    // For Phase 3, free as single page
    memory_free_pages(ptr, 1);
}

/**
 * Show memory layout for debugging
 */
void memory_show_layout(struct boot_info *boot_info)
{
    if (!boot_info) {
        return;
    }
    
    early_print("\n=== Memory Layout ===\n");
    
    // Show memory map entries
    if (boot_info->memory_map && boot_info->memory_map_entries > 0) {
        early_print("Memory map entries: ");
        
        // Convert number to string
        char num_str[16];
        uint32_t entries = boot_info->memory_map_entries;
        int pos = 0;
        
        if (entries == 0) {
            num_str[pos++] = '0';
        } else {
            char temp[16];
            int temp_pos = 0;
            while (entries > 0) {
                temp[temp_pos++] = '0' + (entries % 10);
                entries /= 10;
            }
            while (temp_pos > 0) {
                num_str[pos++] = temp[--temp_pos];
            }
        }
        num_str[pos] = 0;
        early_print(num_str);
        early_print("\n");
        
        // Show first few memory regions
        uint32_t show_count = boot_info->memory_map_entries;
        if (show_count > 5) show_count = 5;  // Show max 5 entries
        
        for (uint32_t i = 0; i < show_count; i++) {
            struct memory_map_entry *entry = &boot_info->memory_map[i];
            
            early_print("  Region ");
            if (i < 10) {
                char ch[2] = {'0' + i, 0};
                early_print(ch);
            } else {
                early_print("?");
            }
            early_print(": 0x");
            
            // Print base address (simplified)
            uint64_t addr = entry->base;
            for (int j = 7; j >= 0; j--) {
                int digit = (addr >> (j * 4)) & 0xF;
                print_hex_digit(digit);
            }
            
            early_print(" - ");
            
            // Print size (in MB for readability)
            uint64_t size_mb = entry->length / (1024 * 1024);
            if (size_mb == 0) size_mb = 1;  // At least 1MB for display
            
            char size_str[16];
            pos = 0;
            if (size_mb == 0) {
                size_str[pos++] = '0';
            } else {
                char temp[16];
                int temp_pos = 0;
                while (size_mb > 0) {
                    temp[temp_pos++] = '0' + (size_mb % 10);
                    size_mb /= 10;
                }
                while (temp_pos > 0) {
                    size_str[pos++] = temp[--temp_pos];
                }
            }
            size_str[pos] = 0;
            early_print(size_str);
            early_print("MB ");
            
            // Show type
            switch (entry->type) {
                case MEMORY_TYPE_AVAILABLE:
                    early_print("(Available)");
                    break;
                case MEMORY_TYPE_RESERVED:
                    early_print("(Reserved)");
                    break;
                case MEMORY_TYPE_ACPI_RECLAIM:
                    early_print("(ACPI Reclaimable)");
                    break;
                case MEMORY_TYPE_ACPI_NVS:
                    early_print("(ACPI NVS)");
                    break;
                case MEMORY_TYPE_BAD:
                    early_print("(Bad Memory)");
                    break;
                default:
                    early_print("(Unknown)");
                    break;
            }
            early_print("\n");
        }
        
        if (boot_info->memory_map_entries > 5) {
            early_print("  ... and ");
            uint32_t remaining = boot_info->memory_map_entries - 5;
            char rem_str[16];
            pos = 0;
            if (remaining == 0) {
                rem_str[pos++] = '0';
            } else {
                char temp[16];
                int temp_pos = 0;
                while (remaining > 0) {
                    temp[temp_pos++] = '0' + (remaining % 10);
                    remaining /= 10;
                }
                while (temp_pos > 0) {
                    rem_str[pos++] = temp[--temp_pos];
                }
            }
            rem_str[pos] = 0;
            early_print(rem_str);
            early_print(" more regions\n");
        }
    }
    
    // Show memory statistics
    struct memory_stats stats;
    memory_get_stats(&stats);
    
    early_print("Total memory: ");
    uint64_t total_mb = stats.total_memory / (1024 * 1024);
    char total_str[16];
    int pos = 0;
    if (total_mb == 0) {
        total_str[pos++] = '0';
    } else {
        char temp[16];
        int temp_pos = 0;
        while (total_mb > 0) {
            temp[temp_pos++] = '0' + (total_mb % 10);
            total_mb /= 10;
        }
        while (temp_pos > 0) {
            total_str[pos++] = temp[--temp_pos];
        }
    }
    total_str[pos] = 0;
    early_print(total_str);
    early_print("MB\n");
    
    early_print("Free memory: ");
    uint64_t free_mb = stats.free_memory / (1024 * 1024);
    char free_str[16];
    pos = 0;
    if (free_mb == 0) {
        free_str[pos++] = '0';
    } else {
        char temp[16];
        int temp_pos = 0;
        while (free_mb > 0) {
            temp[temp_pos++] = '0' + (free_mb % 10);
            free_mb /= 10;
        }
        while (temp_pos > 0) {
            free_str[pos++] = temp[--temp_pos];
        }
    }
    free_str[pos] = 0;
    early_print(free_str);
    early_print("MB\n");
    
    early_print("=====================\n\n");
}

// Simple memory allocation for shell (basic implementation)
// In a real OS, this would use a proper heap allocator

static char simple_heap[64 * 1024];  // 64KB static heap
static size_t heap_offset = 0;

void *kmalloc(size_t size) {
    // Simple bump allocator
    if (size == 0) return NULL;
    
    // Align to 8 bytes
    size = (size + 7) & ~7;
    
    if (heap_offset + size > sizeof(simple_heap)) {
        return NULL;  // Out of memory
    }
    
    void *ptr = &simple_heap[heap_offset];
    heap_offset += size;
    
    return ptr;
}

void kfree(void *ptr) {
    // Simple allocator doesn't support freeing individual blocks
    // In a real implementation, this would maintain a free list
    (void)ptr;  // Suppress warning
}
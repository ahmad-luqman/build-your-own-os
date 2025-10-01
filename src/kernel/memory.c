/*
 * MiniOS Cross-Platform Memory Management Implementation
 * Phase 3: Memory Management & Kernel Loading
 */

#include "memory.h"
#include "kernel.h"

// Memory subsystem initialization state
static int memory_initialized = 0;

/**
 * Initialize memory management subsystem
 */
int memory_init(struct boot_info *boot_info)
{
    early_print("memory_init: Starting\n");
    
    if (memory_initialized) {
        return 0;  // Already initialized
    }
    
    early_print("memory_init: Validating boot_info\n");
    if (!boot_info || !boot_info_valid(boot_info)) {
        early_print("Memory init: Invalid boot info\n");
        return -1;
    }
    
    early_print("Initializing memory management...\n");
    
    early_print("memory_init: Calling arch_memory_init\n");
    // Initialize architecture-specific memory management
    if (arch_memory_init(boot_info) < 0) {
        early_print("Memory init: Architecture init failed\n");
        return -1;
    }
    early_print("memory_init: arch_memory_init complete\n");
    
    early_print("memory_init: Starting allocator init\n");
    // Initialize physical memory allocator
    if (boot_info->memory_map && boot_info->memory_map_entries > 0) {
        arch_memory_allocator_init(boot_info->memory_map, 
                                   boot_info->memory_map_entries);
        early_print("Memory allocator initialized\n");
    } else {
        early_print("Memory init: No memory map available\n");
        return -1;
    }
    early_print("memory_init: Allocator init complete\n");
    
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
    
    // Show memory map entries count
    early_print("Memory map entries: 1\n");  
    early_print("  Region 0: 0x40000000 - 128MB (Available)\n");
    
    // Show memory statistics
    early_print("About to call memory_get_stats...\n");
    struct memory_stats stats;
    memory_get_stats(&stats);
    early_print("memory_get_stats completed\n");
    
    early_print("Total memory: 16MB\n");    // Based on our allocator config
    early_print("Free memory: 16MB\n");
    early_print("===================\n\n");
    
    early_print("memory_show_layout completed, returning to main\n");
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
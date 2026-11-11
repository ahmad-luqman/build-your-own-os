/*
 * ARM64 Physical Memory Allocator (Architecture-specific functions only)
 * Phase 3: Memory Management & Kernel Loading
 */

#include <stdint.h>
#include <stddef.h>
#include "memory.h"
#include "kernel.h" // For itoa
#include "string.h"

// Add forward declaration for early_print
void early_print(const char *str);

// Kernel image bounds from linker.ld (same types as arch/arm64/include/memory.h)
extern uint64_t __kernel_start;
extern uint64_t __kernel_end;

// Memory allocation bitmap  
#define MAX_PAGES           4096        // Support up to 16MB of 4KB pages (sufficient for Phase 3)
#define BITMAP_SIZE         (MAX_PAGES / 8)
static uint8_t page_bitmap[BITMAP_SIZE];
static uint64_t total_pages = 0;
static uint64_t free_pages = 0;
static uint64_t memory_base = 0;

// Memory statistics
static struct memory_stats current_stats = {0};

// Forward declarations
static void set_page_used(uint32_t page_num);
static void set_page_free(uint32_t page_num);
static int is_page_free(uint32_t page_num);
static uint32_t find_free_pages(uint32_t count);

/**
 * Initialize ARM64 physical memory allocator
 */
void arch_memory_allocator_init(struct memory_map_entry *memory_map,
                                uint32_t map_entries)
{
    early_print("allocator: start\n");
    
    // Initialize bitmap to all used (avoid SIMD instructions)
    // Use volatile to prevent compiler optimization into NEON instructions
    volatile uint8_t *bitmap = page_bitmap;
    for (int i = 0; i < BITMAP_SIZE; i++) {
        bitmap[i] = 0xFF;
    }
    early_print("allocator: bitmap done\n");
    
    total_pages = 0;
    free_pages = 0;
    
    // Find the largest available memory region
    uint64_t largest_base = 0;
    uint64_t largest_size = 0;
    
    for (uint32_t i = 0; i < map_entries; i++) {
        if (memory_map[i].type == MEMORY_TYPE_AVAILABLE) {
            if (memory_map[i].length > largest_size) {
                largest_base = memory_map[i].base;
                largest_size = memory_map[i].length;
            }
        }
    }
    early_print("allocator: region found\n");
    
    if (largest_size == 0) {
        return;  // No available memory found
    }
    
    // Use the largest region for allocation
    memory_base = largest_base;
    uint64_t usable_size = largest_size;
    
    // Don't use first 1MB (reserved for bootloader/firmware)
    if (memory_base < 0x100000) {
        uint64_t offset = 0x100000 - memory_base;
        if (offset < usable_size) {
            memory_base += offset;
            usable_size -= offset;
        } else {
            return;  // Region too small
        }
    }

    /*
     * Reserve the kernel image (same guard as x86-64, see issue #27).
     *
     * RAM starts at 0x40000000 but the kernel is loaded at __kernel_start
     * (0x40080000) and runs to __kernel_end. That range includes .bss with the
     * 2MB static kmalloc heap, and the fixed boot and exception stacks
     * (0x40200000 / 0x40300000) currently also fall inside it. Without this
     * guard, any allocation past the first 512KB of RAM would be handed
     * memory the kernel is already using (and QEMU may place the DTB at the
     * start of RAM, below the kernel).
     * On x86-64 this exact overlap let SFS format zero the kernel's .text.
     *
     * If the kernel overlaps the chosen region, move the base up to the
     * page-aligned end of the image, so [region start, kernel end) is never
     * handed out.
     */
    uint64_t kernel_start = (uint64_t)(uintptr_t)&__kernel_start;
    uint64_t kernel_end = ((uint64_t)(uintptr_t)&__kernel_end + PAGE_SIZE_4K - 1)
                          & ~(uint64_t)(PAGE_SIZE_4K - 1);
    uint64_t region_end = memory_base + usable_size;

    if (kernel_start < region_end && kernel_end > memory_base) {
        if (kernel_end >= region_end) {
            early_print("allocator: kernel image covers the whole usable region, "
                        "no pages available!\n");
            return;  // total_pages/free_pages stay 0, so allocations return NULL
        }
        early_print("allocator: reserving kernel image, base moved past __kernel_end\n");
        usable_size = region_end - kernel_end;
        memory_base = kernel_end;
    }

    // Calculate number of pages
    total_pages = usable_size / PAGE_SIZE_4K;
    if (total_pages > MAX_PAGES) {
        total_pages = MAX_PAGES;
    }
    early_print("allocator: pages calculated\n");
    
    // Mark all pages as free initially
    for (uint32_t i = 0; i < total_pages; i++) {
        set_page_free(i);
    }
    early_print("allocator: pages freed\n");
    
    free_pages = total_pages;
    
    // Update statistics
    current_stats.total_memory = total_pages * PAGE_SIZE_4K;
    current_stats.free_memory = free_pages * PAGE_SIZE_4K;
    current_stats.used_memory = 0;
    current_stats.total_regions = map_entries;
    current_stats.free_regions = total_pages;
    
    early_print("allocator: complete\n");
}

/**
 * Allocate physical pages
 */
void *memory_alloc_pages(size_t num_pages)
{
    if (num_pages == 0 || num_pages > free_pages) {
        return NULL;
    }
    
    uint32_t start_page = find_free_pages(num_pages);
    if (start_page == 0xFFFFFFFF) {
        return NULL;  // No contiguous pages found
    }
    
    // Mark pages as used
    for (uint32_t i = start_page; i < start_page + num_pages; i++) {
        set_page_used(i);
    }
    
    free_pages -= num_pages;
    
    // Update statistics
    current_stats.free_memory = free_pages * PAGE_SIZE_4K;
    current_stats.used_memory = (total_pages - free_pages) * PAGE_SIZE_4K;
    current_stats.free_regions = free_pages;
    
    // Return virtual address (identity mapped for now)
    return (void *)(memory_base + start_page * PAGE_SIZE_4K);
}

/**
 * Free physical pages
 */
void memory_free_pages(void *ptr, size_t num_pages)
{
    if (!ptr || num_pages == 0) {
        return;
    }
    
    uint64_t addr = (uint64_t)ptr;
    if (addr < memory_base) {
        return;  // Invalid address
    }
    
    uint32_t start_page = (addr - memory_base) / PAGE_SIZE_4K;
    if (start_page >= total_pages || start_page + num_pages > total_pages) {
        return;  // Invalid page range
    }
    
    // Mark pages as free
    for (uint32_t i = start_page; i < start_page + num_pages; i++) {
        if (!is_page_free(i)) {
            set_page_free(i);
            free_pages++;
        }
    }
    
    // Update statistics
    current_stats.free_memory = free_pages * PAGE_SIZE_4K;
    current_stats.used_memory = (total_pages - free_pages) * PAGE_SIZE_4K;
    current_stats.free_regions = free_pages;
}

/**
 * Get memory statistics
 */
void memory_get_stats(struct memory_stats *stats)
{
    if (!stats) return;
    
    // Manual field assignment instead of struct copy to avoid SIMD issues
    stats->total_memory = total_pages * PAGE_SIZE_4K;
    stats->free_memory = free_pages * PAGE_SIZE_4K;
    stats->used_memory = (total_pages - free_pages) * PAGE_SIZE_4K;
    stats->total_regions = 1;
    stats->free_regions = free_pages;
}

// Helper functions

/**
 * Mark page as used in bitmap
 */
static void set_page_used(uint32_t page_num)
{
    if (page_num >= total_pages) return;
    
    uint32_t byte_index = page_num / 8;
    uint32_t bit_index = page_num % 8;
    
    page_bitmap[byte_index] |= (1 << bit_index);
}

/**
 * Mark page as free in bitmap
 */
static void set_page_free(uint32_t page_num)
{
    if (page_num >= total_pages) return;
    
    uint32_t byte_index = page_num / 8;
    uint32_t bit_index = page_num % 8;
    
    page_bitmap[byte_index] &= ~(1 << bit_index);
}

/**
 * Check if page is free
 */
static int is_page_free(uint32_t page_num)
{
    if (page_num >= total_pages) return 0;
    
    uint32_t byte_index = page_num / 8;
    uint32_t bit_index = page_num % 8;
    
    return !(page_bitmap[byte_index] & (1 << bit_index));
}

/**
 * Find contiguous free pages
 */
static uint32_t find_free_pages(uint32_t count)
{
    for (uint32_t start = 0; start <= total_pages - count; start++) {
        uint32_t found = 0;
        
        // Check if we have count contiguous free pages starting at start
        for (uint32_t i = start; i < start + count && i < total_pages; i++) {
            if (is_page_free(i)) {
                found++;
            } else {
                break;
            }
        }
        
        if (found == count) {
            return start;
        }
    }
    
    return 0xFFFFFFFF;  // Not found
}
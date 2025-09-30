/*
 * x86-64 Physical Memory Allocator (Architecture-specific functions only)
 * Phase 3: Memory Management & Kernel Loading
 */

#include <stdint.h>
#include <stddef.h>
#include "memory.h"
#include "boot_protocol.h"

// Memory allocation bitmap
#define MAX_PAGES           32768       // Support up to 128MB of 4KB pages
#define BITMAP_SIZE         (MAX_PAGES / 8)
static uint8_t page_bitmap[BITMAP_SIZE];
static uint64_t total_pages = 0;
static uint64_t free_pages = 0;
static uint64_t memory_base = 0;

// Memory region tracking
#define MAX_MEMORY_REGIONS  32
static struct memory_region memory_regions[MAX_MEMORY_REGIONS];
static uint32_t num_memory_regions = 0;

// Memory statistics
static struct memory_stats current_stats = {0};

// Forward declarations
static void set_page_used(uint32_t page_num);
static void set_page_free(uint32_t page_num);
static int is_page_free(uint32_t page_num);
static uint32_t find_free_pages(uint32_t count);
static void add_memory_region(uint64_t base, uint64_t size, uint32_t type);

/**
 * Initialize x86-64 physical memory allocator
 */
void arch_memory_allocator_init(struct memory_map_entry *memory_map,
                                uint32_t map_entries)
{
    // Initialize bitmap to all used
    for (int i = 0; i < BITMAP_SIZE; i++) {
        page_bitmap[i] = 0xFF;
    }
    
    total_pages = 0;
    free_pages = 0;
    num_memory_regions = 0;
    
    // Process memory map
    uint64_t total_available = 0;
    uint64_t largest_base = 0;
    uint64_t largest_size = 0;
    
    for (uint32_t i = 0; i < map_entries; i++) {
        add_memory_region(memory_map[i].base, memory_map[i].length, 
                         memory_map[i].type);
        
        if (memory_map[i].type == MEMORY_TYPE_AVAILABLE) {
            total_available += memory_map[i].length;
            
            // Find largest contiguous region
            if (memory_map[i].length > largest_size) {
                largest_base = memory_map[i].base;
                largest_size = memory_map[i].length;
            }
        }
    }
    
    if (largest_size == 0) {
        return;  // No available memory
    }
    
    // Use the largest available region for page allocation
    memory_base = largest_base;
    uint64_t usable_size = largest_size;
    
    // Reserve first 1MB for bootloader and BIOS data
    if (memory_base < 0x100000) {
        uint64_t offset = 0x100000 - memory_base;
        if (offset < usable_size) {
            memory_base += offset;
            usable_size -= offset;
        } else {
            return;  // Region too small
        }
    }
    
    // Calculate number of pages we can manage
    total_pages = usable_size / PAGE_SIZE_4K;
    if (total_pages > MAX_PAGES) {
        total_pages = MAX_PAGES;
    }
    
    // Mark all pages as free initially
    for (uint32_t i = 0; i < total_pages; i++) {
        set_page_free(i);
    }
    
    free_pages = total_pages;
    
    // Update statistics
    current_stats.total_memory = total_available;
    current_stats.free_memory = free_pages * PAGE_SIZE_4K;
    current_stats.used_memory = 0;
    current_stats.total_regions = map_entries;
    current_stats.free_regions = total_pages;
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
        return NULL;
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
        return;
    }
    
    uint32_t start_page = (addr - memory_base) / PAGE_SIZE_4K;
    if (start_page >= total_pages || start_page + num_pages > total_pages) {
        return;
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
    if (stats) {
        *stats = current_stats;
    }
}

// Helper functions

/**
 * Mark page as used
 */
static void set_page_used(uint32_t page_num)
{
    if (page_num >= total_pages) return;
    
    uint32_t byte_index = page_num / 8;
    uint32_t bit_index = page_num % 8;
    
    page_bitmap[byte_index] |= (1 << bit_index);
}

/**
 * Mark page as free
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
    
    return 0xFFFFFFFF;
}

/**
 * Add memory region to tracking
 */
static void add_memory_region(uint64_t base, uint64_t size, uint32_t type)
{
    if (num_memory_regions >= MAX_MEMORY_REGIONS) {
        return;
    }
    
    memory_regions[num_memory_regions].base = base;
    memory_regions[num_memory_regions].size = size;
    memory_regions[num_memory_regions].type = type;
    memory_regions[num_memory_regions].attributes = 0;
    
    num_memory_regions++;
}
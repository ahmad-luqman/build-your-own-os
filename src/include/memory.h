/*
 * MiniOS Cross-Platform Memory Management Interface
 * Phase 3: Memory Management & Kernel Loading
 */

#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>
#include "boot_protocol.h"

// Architecture detection
#ifndef ARCH_ARM64
#ifdef __aarch64__
#define ARCH_ARM64
#elif defined(__x86_64__)
#define ARCH_X86_64
#else
#error "Unsupported architecture"
#endif
#endif

// Memory region attributes
#define MEMORY_READABLE   (1 << 0)
#define MEMORY_WRITABLE   (1 << 1)
#define MEMORY_EXECUTABLE (1 << 2)
#define MEMORY_CACHEABLE  (1 << 3)
#define MEMORY_DEVICE     (1 << 4)

// Memory allocation alignment
#define MEMORY_ALIGN_4K   0x1000
#define MEMORY_ALIGN_64K  0x10000
#define MEMORY_ALIGN_2M   0x200000
#define MEMORY_ALIGN_1G   0x40000000

// Page size constants
#define PAGE_SIZE_4K      0x1000
#define PAGE_SIZE_2M      0x200000
#define PAGE_SIZE_1G      0x40000000

// Memory region descriptor
struct memory_region {
    uint64_t base;           // Physical base address
    uint64_t size;           // Size in bytes
    uint32_t attributes;     // Memory attributes (MEMORY_*)
    uint32_t type;           // Memory type from boot protocol
};

// Memory statistics
struct memory_stats {
    uint64_t total_memory;   // Total available memory
    uint64_t used_memory;    // Currently allocated memory
    uint64_t free_memory;    // Available for allocation
    uint32_t total_regions;  // Number of memory regions
    uint32_t free_regions;   // Number of free regions
};

// Kernel load information
struct kernel_load_info {
    void *kernel_entry;      // Kernel entry point
    void *kernel_base;       // Load address
    size_t kernel_size;      // Size in memory
    void *sections;          // ELF section information
    uint32_t num_sections;   // Number of sections
};

// Cross-platform memory management API

/**
 * Initialize memory management subsystem
 * @param boot_info Boot information from bootloader
 * @return 0 on success, negative on error
 */
int memory_init(struct boot_info *boot_info);

/**
 * Map physical memory to virtual address space
 * @param phys_addr Physical address to map
 * @param size Size of mapping in bytes
 * @param flags Memory attributes (MEMORY_*)
 * @return Virtual address on success, NULL on failure
 */
void *memory_map(uint64_t phys_addr, size_t size, uint32_t flags);

/**
 * Unmap virtual memory
 * @param virt_addr Virtual address to unmap
 * @param size Size of mapping in bytes
 */
void memory_unmap(void *virt_addr, size_t size);

/**
 * Allocate physical memory
 * @param size Size in bytes
 * @param alignment Required alignment (power of 2)
 * @return Virtual address on success, NULL on failure
 */
void *memory_alloc(size_t size, uint32_t alignment);

/**
 * Free allocated memory
 * @param ptr Pointer to free
 */
void memory_free(void *ptr);

/**
 * Allocate pages
 * @param num_pages Number of pages to allocate
 * @return Virtual address on success, NULL on failure
 */
void *memory_alloc_pages(size_t num_pages);

/**
 * Free pages
 * @param ptr Pointer to pages
 * @param num_pages Number of pages to free
 */
void memory_free_pages(void *ptr, size_t num_pages);

/**
 * Get memory statistics
 * @param stats Pointer to stats structure to fill
 */
void memory_get_stats(struct memory_stats *stats);

/**
 * Show memory layout for debugging
 * @param boot_info Boot information structure
 */
void memory_show_layout(struct boot_info *boot_info);

// Architecture-specific functions (implemented per architecture)

/**
 * Initialize architecture-specific memory management
 * @param boot_info Boot information from bootloader
 * @return 0 on success, negative on error
 */
int arch_memory_init(struct boot_info *boot_info);

/**
 * Enable memory management unit / paging
 */
void arch_memory_enable(void);

/**
 * Map virtual memory with architecture-specific page tables
 * @param virt_addr Virtual address
 * @param phys_addr Physical address  
 * @param size Size of mapping
 * @param flags Architecture-specific flags
 * @return 0 on success, negative on error
 */
int arch_memory_map_pages(uint64_t virt_addr, uint64_t phys_addr, 
                          size_t size, uint32_t flags);

/**
 * Initialize physical memory allocator
 * @param memory_map Memory map from bootloader
 * @param map_entries Number of map entries
 */
void arch_memory_allocator_init(struct memory_map_entry *memory_map, 
                                uint32_t map_entries);

#endif // MEMORY_H
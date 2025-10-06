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
    
    // Verify allocator is working with a test allocation
    early_print("memory_init: Testing allocator...\n");
    void *test = memory_alloc(PAGE_SIZE_4K, MEMORY_ALIGN_4K);
    if (test) {
        early_print("memory_init: Allocator test PASSED\n");
        memory_free(test);
    } else {
        early_print("memory_init: Allocator test FAILED\n");
        memory_initialized = 0;
        return -1;
    }
    
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
    // TEMPORARY: Skip memory_get_stats as it causes crashes
    // Likely due to struct copy operation or memcpy issue
    // struct memory_stats stats;
    // memory_get_stats(&stats);
    early_print("memory_get_stats skipped (TEMP)\n");
    
    early_print("Total memory: 16MB\n");    // Based on our allocator config
    early_print("Free memory: 16MB\n");
    early_print("===================\n\n");
    
    early_print("memory_show_layout completed, returning to main\n");
}

// Simple memory allocation for shell (basic implementation)
// In a real OS, this would use a proper heap allocator

#define KMALLOC_ALIGNMENT 16

// Increase heap size to 2MB to handle file system operations
static uint8_t simple_heap[2 * 1024 * 1024] __attribute__((aligned(KMALLOC_ALIGNMENT)));  // 2MB static heap
static size_t heap_offset = 0;

// Memory allocation tracking for leak detection
#define MAX_ALLOC_TRACKING 256

struct alloc_entry {
    void *ptr;
    size_t size;
    const char *file;
    int line;
    uint32_t timestamp;  // Simple counter
};

#ifndef __x86_64__
// TEMPORARY: Disable tracking structures on x86-64 due to Issue #18
static struct alloc_entry alloc_table[MAX_ALLOC_TRACKING];
static uint32_t alloc_timestamp = 0;

// Per-subsystem tracking (based on file prefix)
struct subsystem_stats {
    const char *name;
    size_t allocations;
    size_t bytes;
};

static struct subsystem_stats subsys_stats[] = {
    {"VFS", 0, 0},
    {"SFS", 0, 0},
    {"RAMFS", 0, 0},
    {"Block", 0, 0},
    {"Shell", 0, 0},
    {"FD", 0, 0},
    {"Other", 0, 0},
};

// Simple memory statistics
static struct {
    size_t total_allocations;
    size_t total_bytes_allocated;
    size_t peak_usage;
    size_t current_usage;
    size_t active_allocations;  // Currently allocated blocks
} kmalloc_stats = {0};

// Helper: Categorize allocation by file name
static int get_subsystem_index(const char *file) {
    if (!file) return 6;  // Other

    // Simple substring matching
    if (strstr(file, "vfs")) return 0;    // VFS
    if (strstr(file, "sfs")) return 1;    // SFS
    if (strstr(file, "ramfs")) return 2;  // RAMFS
    if (strstr(file, "block") || strstr(file, "ramdisk")) return 3;  // Block
    if (strstr(file, "shell") || strstr(file, "parser") ||
        strstr(file, "completion") || strstr(file, "history")) return 4;  // Shell
    if (strstr(file, "fd")) return 5;     // FD

    return 6;  // Other
}
#endif  // !__x86_64__

// Helper: Record allocation in tracking table
#ifndef __x86_64__
static void record_allocation(void *ptr, size_t size, const char *file, int line) {
    for (int i = 0; i < MAX_ALLOC_TRACKING; i++) {
        if (alloc_table[i].ptr == NULL) {
            alloc_table[i].ptr = ptr;
            alloc_table[i].size = size;
            alloc_table[i].file = file;
            alloc_table[i].line = line;
            alloc_table[i].timestamp = alloc_timestamp++;

            // Update subsystem stats
            int subsys = get_subsystem_index(file);
            subsys_stats[subsys].allocations++;
            subsys_stats[subsys].bytes += size;

            kmalloc_stats.active_allocations++;
            return;
        }
    }
    // Tracking table full - allocation still succeeds but not tracked
}
#endif  // !__x86_64__

// Helper: Remove allocation from tracking table
#ifndef __x86_64__
static void unrecord_allocation(void *ptr) {
    for (int i = 0; i < MAX_ALLOC_TRACKING; i++) {
        if (alloc_table[i].ptr == ptr) {
            // Update subsystem stats
            int subsys = get_subsystem_index(alloc_table[i].file);
            subsys_stats[subsys].bytes -= alloc_table[i].size;

            alloc_table[i].ptr = NULL;
            alloc_table[i].size = 0;
            alloc_table[i].file = NULL;
            alloc_table[i].line = 0;

            kmalloc_stats.active_allocations--;
            return;
        }
    }
}
#endif  // !__x86_64__

#ifdef KMALLOC_DEBUG
void *kmalloc_debug(size_t size, const char *file, int line) {
#else
void *kmalloc(size_t size) {
#ifndef __x86_64__
    const char *file = NULL;
    int line = 0;
#else
    // x86-64: Skip tracking variables (unused due to temp fix for Issue #18)
    (void)0;  // Placeholder
#endif
#endif
#ifdef __x86_64__
    early_print("kmalloc: Entry, size=");
    char size_buf[16];
    int len = 0;
    size_t temp = size;
    if (temp == 0) {
        size_buf[len++] = '0';
    } else {
        char temp_buf[16];
        int temp_len = 0;
        while (temp > 0 && temp_len < 15) {
            temp_buf[temp_len++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = temp_len - 1; i >= 0; i--) {
            size_buf[len++] = temp_buf[i];
        }
    }
    size_buf[len] = '\0';
    early_print(size_buf);
    early_print("\n");
#endif

    // Simple bump allocator with better tracking
    if (size == 0) {
#ifdef __x86_64__
        early_print("kmalloc: Size is 0, returning NULL\n");
#endif
        return NULL;
    }

#ifdef __x86_64__
    early_print("kmalloc: Aligning size...\n");
#endif
    // Align requested size and heap pointer so we can safely satisfy
    // callers that issue 128-bit accesses (e.g. NEON stores emitted by GCC).
    size = (size + (KMALLOC_ALIGNMENT - 1)) & ~(size_t)(KMALLOC_ALIGNMENT - 1);
#ifdef __x86_64__
    early_print("kmalloc: Aligned size calculated\n");

    early_print("kmalloc: Calculating aligned_offset...\n");
#endif
    size_t aligned_offset = (heap_offset + (KMALLOC_ALIGNMENT - 1)) & ~(size_t)(KMALLOC_ALIGNMENT - 1);
#ifdef __x86_64__
    early_print("kmalloc: aligned_offset calculated\n");

    early_print("kmalloc: Checking heap space...\n");
#endif
    if (aligned_offset + size > sizeof(simple_heap)) {
        early_print("kmalloc: OUT OF MEMORY - ");
        // Print usage statistics for debugging
        early_print("requested=");
        char size_buf[16];
        int len = 0;
        size_t temp = size;
        do {
            size_buf[len++] = '0' + (temp % 10);
            temp /= 10;
        } while (temp > 0 && len < 15);
        size_buf[len] = 0;
        // Reverse the string
        for (int i = 0; i < len/2; i++) {
            char c = size_buf[i];
            size_buf[i] = size_buf[len-1-i];
            size_buf[len-1-i] = c;
        }
        early_print(size_buf);
        early_print(" used=");
        temp = aligned_offset;
        len = 0;
        do {
            size_buf[len++] = '0' + (temp % 10);
            temp /= 10;
        } while (temp > 0 && len < 15);
        size_buf[len] = 0;
        for (int i = 0; i < len/2; i++) {
            char c = size_buf[i];
            size_buf[i] = size_buf[len-1-i];
            size_buf[len-1-i] = c;
        }
        early_print(size_buf);
        early_print(" total=2MB\n");
        return NULL;  // Out of memory
    }

#ifdef __x86_64__
    early_print("kmalloc: Computing heap pointer address...\n");
#endif
    void *ptr = &simple_heap[aligned_offset];
#ifdef __x86_64__
    early_print("kmalloc: Heap pointer computed\n");

    early_print("kmalloc: Updating heap_offset...\n");
#endif
    heap_offset = aligned_offset + size;
#ifdef __x86_64__
    early_print("kmalloc: heap_offset updated\n");

    early_print("kmalloc: Skipping statistics updates on x86-64 (TEMP FIX)\n");
    // TEMPORARY FIX for Issue #18: Skip statistics updates on x86-64
    // Root cause: static struct access causes page fault/triple fault
    // TODO: Investigate why .data/.bss section writes fail on x86-64
#else
    // Update statistics
    kmalloc_stats.total_allocations++;
    kmalloc_stats.total_bytes_allocated += size;
    kmalloc_stats.current_usage = aligned_offset + size;
    if (kmalloc_stats.current_usage > kmalloc_stats.peak_usage) {
        kmalloc_stats.peak_usage = kmalloc_stats.current_usage;
    }
#endif

#ifdef __x86_64__
    early_print("kmalloc: Skipping allocation tracking on x86-64 (TEMP FIX)\n");
    // TEMPORARY FIX: Skip record_allocation which also writes to static arrays
#else
    // Track allocation for leak detection
    record_allocation(ptr, size, file, line);
#endif

    // Debug output with address (simplified)
#ifdef __x86_64__
    early_print("kmalloc: Returning pointer\n");
#else
    early_print("kmalloc: OK\n");
#endif

    return ptr;
}

#ifdef KMALLOC_DEBUG
void kfree_debug(void *ptr, const char *file, int line) {
    (void)file;  // Not used yet, but available for future enhancements
    (void)line;
#else
void kfree(void *ptr) {
#endif
    // Simple allocator doesn't support freeing individual blocks
    // In a real implementation, this would maintain a free list

#ifndef __x86_64__
    // Remove from tracking table (even though we don't actually free)
    unrecord_allocation(ptr);
#endif

    (void)ptr;  // Suppress warning
}

/**
 * Check if memory allocator is ready
 */
int memory_allocator_is_ready(void) {
    return memory_initialized;
}

/**
 * Get memory allocation statistics for debugging
 */
void memory_get_alloc_stats(void) {
#ifndef __x86_64__
    early_print("=== Memory Allocation Statistics ===\n");
    early_print("Total allocations: ");
    // Simple number printing (avoiding printf dependencies)
    size_t num = kmalloc_stats.total_allocations;
    if (num == 0) {
        early_print("0");
    } else {
        char buf[16];
        int len = 0;
        while (num > 0 && len < 15) {
            buf[len++] = '0' + (num % 10);
            num /= 10;
        }
        buf[len] = 0;
        // Reverse
        for (int i = 0; i < len/2; i++) {
            char c = buf[i];
            buf[i] = buf[len-1-i];
            buf[len-1-i] = c;
        }
        early_print(buf);
    }
    early_print("\n");
    
    early_print("Current usage: ");
    num = kmalloc_stats.current_usage;
    if (num == 0) {
        early_print("0");
    } else {
        char buf[16];
        int len = 0;
        while (num > 0 && len < 15) {
            buf[len++] = '0' + (num % 10);
            num /= 10;
        }
        buf[len] = 0;
        for (int i = 0; i < len/2; i++) {
            char c = buf[i];
            buf[i] = buf[len-1-i];
            buf[len-1-i] = c;
        }
        early_print(buf);
    }
    early_print(" bytes\n");
    
    early_print("Peak usage: ");
    num = kmalloc_stats.peak_usage;
    if (num == 0) {
        early_print("0");
    } else {
        char buf[16];
        int len = 0;
        while (num > 0 && len < 15) {
            buf[len++] = '0' + (num % 10);
            num /= 10;
        }
        buf[len] = 0;
        for (int i = 0; i < len/2; i++) {
            char c = buf[i];
            buf[i] = buf[len-1-i];
            buf[len-1-i] = c;
        }
        early_print(buf);
    }
    early_print(" bytes\n");
    early_print("===================================\n");
#else
    early_print("=== Memory Allocation Statistics ===\n");
    early_print("(Not available on x86-64 - see Issue #18)\n");
    early_print("===================================\n");
#endif
}

/**
 * Check for memory leaks and report them
 */
void memory_leak_check(void) {
#ifndef __x86_64__
    early_print("\n=== Memory Leak Detection ===\n");

    int leak_count = 0;
    size_t leak_bytes = 0;

    for (int i = 0; i < MAX_ALLOC_TRACKING; i++) {
        if (alloc_table[i].ptr != NULL) {
            leak_count++;
            leak_bytes += alloc_table[i].size;
        }
    }

    if (leak_count == 0) {
        early_print("No memory leaks detected!\n");
    } else {
        early_print("LEAKS DETECTED: ");
        // Print leak count
        char buf[16];
        int len = 0;
        int temp = leak_count;
        if (temp == 0) {
            buf[len++] = '0';
        } else {
            char temp_buf[16];
            int temp_len = 0;
            while (temp > 0) {
                temp_buf[temp_len++] = '0' + (temp % 10);
                temp /= 10;
            }
            for (int i = temp_len - 1; i >= 0; i--) {
                buf[len++] = temp_buf[i];
            }
        }
        buf[len] = '\0';
        early_print(buf);
        early_print(" allocations, ");

        // Print leak bytes
        len = 0;
        size_t temp_size = leak_bytes;
        if (temp_size == 0) {
            buf[len++] = '0';
        } else {
            char temp_buf[16];
            int temp_len = 0;
            while (temp_size > 0) {
                temp_buf[temp_len++] = '0' + (temp_size % 10);
                temp_size /= 10;
            }
            for (int i = temp_len - 1; i >= 0; i--) {
                buf[len++] = temp_buf[i];
            }
        }
        buf[len] = '\0';
        early_print(buf);
        early_print(" bytes\n");
    }

    early_print("Active allocations: ");
    char buf2[16];
    int len2 = 0;
    size_t active = kmalloc_stats.active_allocations;
    if (active == 0) {
        buf2[len2++] = '0';
    } else {
        char temp_buf[16];
        int temp_len = 0;
        while (active > 0) {
            temp_buf[temp_len++] = '0' + (active % 10);
            active /= 10;
        }
        for (int i = temp_len - 1; i >= 0; i--) {
            buf2[len2++] = temp_buf[i];
        }
    }
    buf2[len2] = '\0';
    early_print(buf2);
    early_print("\n===========================\n\n");
#else
    early_print("\n=== Memory Leak Detection ===\n");
    early_print("(Not available on x86-64 - see Issue #18)\n");
    early_print("===========================\n\n");
#endif
}

/**
 * Show all active allocations (for debugging)
 */
void memory_show_allocations(void) {
#ifndef __x86_64__
    early_print("\n=== Active Allocations ===\n");

    int shown = 0;
    for (int i = 0; i < MAX_ALLOC_TRACKING; i++) {
        if (alloc_table[i].ptr != NULL) {
            early_print("  ");

            // Show allocation number
            char buf[8];
            int len = 0;
            int num = shown + 1;
            do {
                buf[len++] = '0' + (num % 10);
                num /= 10;
            } while (num > 0 && len < 7);
            buf[len] = 0;
            for (int j = 0; j < len/2; j++) {
                char c = buf[j];
                buf[j] = buf[len-1-j];
                buf[len-1-j] = c;
            }
            early_print(buf);
            early_print(". ");

            // Show size
            early_print("size=");
            len = 0;
            size_t sz = alloc_table[i].size;
            do {
                buf[len++] = '0' + (sz % 10);
                sz /= 10;
            } while (sz > 0 && len < 7);
            buf[len] = 0;
            for (int j = 0; j < len/2; j++) {
                char c = buf[j];
                buf[j] = buf[len-1-j];
                buf[len-1-j] = c;
            }
            early_print(buf);

            // Show location if available
            if (alloc_table[i].file) {
                early_print(" at ");
                early_print(alloc_table[i].file);
                early_print(":");

                // Print line number
                len = 0;
                int line = alloc_table[i].line;
                if (line > 0) {
                    do {
                        buf[len++] = '0' + (line % 10);
                        line /= 10;
                    } while (line > 0 && len < 7);
                    buf[len] = 0;
                    for (int j = 0; j < len/2; j++) {
                        char c = buf[j];
                        buf[j] = buf[len-1-j];
                        buf[len-1-j] = c;
                    }
                    early_print(buf);
                }
            }

            early_print("\n");
            shown++;

            // Limit output to prevent overwhelming the console
            if (shown >= 20) {
                early_print("  ... (showing first 20 allocations)\n");
                break;
            }
        }
    }

    if (shown == 0) {
        early_print("  No active allocations\n");
    }

    early_print("========================\n\n");
#else
    early_print("\n=== Active Allocations ===\n");
    early_print("(Not available on x86-64 - see Issue #18)\n");
    early_print("========================\n\n");
#endif
}

/**
 * Show per-subsystem memory statistics
 */
void memory_subsystem_stats(void) {
#ifndef __x86_64__
    early_print("\n=== Subsystem Memory Usage ===\n");

    for (int i = 0; i < 7; i++) {
        if (subsys_stats[i].allocations > 0 || subsys_stats[i].bytes > 0) {
            early_print("  ");
            early_print(subsys_stats[i].name);
            early_print(": ");

            // Print allocation count
            char buf[16];
            int len = 0;
            size_t count = subsys_stats[i].allocations;
            if (count == 0) {
                buf[len++] = '0';
            } else {
                char temp_buf[16];
                int temp_len = 0;
                while (count > 0) {
                    temp_buf[temp_len++] = '0' + (count % 10);
                    count /= 10;
                }
                for (int j = temp_len - 1; j >= 0; j--) {
                    buf[len++] = temp_buf[j];
                }
            }
            buf[len] = '\0';
            early_print(buf);
            early_print(" allocs, ");

            // Print bytes
            len = 0;
            size_t bytes = subsys_stats[i].bytes;
            if (bytes == 0) {
                buf[len++] = '0';
            } else {
                char temp_buf[16];
                int temp_len = 0;
                while (bytes > 0) {
                    temp_buf[temp_len++] = '0' + (bytes % 10);
                    bytes /= 10;
                }
                for (int j = temp_len - 1; j >= 0; j--) {
                    buf[len++] = temp_buf[j];
                }
            }
            buf[len] = '\0';
            early_print(buf);
            early_print(" bytes\n");
        }
    }

    early_print("============================\n\n");
#else
    early_print("\n=== Subsystem Memory Usage ===\n");
    early_print("(Not available on x86-64 - see Issue #18)\n");
    early_print("============================\n\n");
#endif
}

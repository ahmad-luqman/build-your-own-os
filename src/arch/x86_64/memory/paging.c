/*
 * x86-64 Paging Implementation
 * Phase 3: Memory Management & Kernel Loading
 */

#include <stdint.h>
#include <stddef.h>
#include "memory.h"
#include "boot_protocol.h"

// x86-64 page table flags
#define PAGE_PRESENT        (1ULL << 0)
#define PAGE_WRITABLE       (1ULL << 1)
#define PAGE_USER           (1ULL << 2)
#define PAGE_WRITETHROUGH   (1ULL << 3)
#define PAGE_CACHE_DISABLE  (1ULL << 4)
#define PAGE_ACCESSED       (1ULL << 5)
#define PAGE_DIRTY          (1ULL << 6)
#define PAGE_SIZE_FLAG      (1ULL << 7)   // For PDPT and PD entries (renamed to avoid conflict)
#define PAGE_GLOBAL         (1ULL << 8)
#define PAGE_NX             (1ULL << 63)  // No execute (requires EFER.NXE)

// Virtual address space layout
#define KERNEL_VIRT_BASE    0xFFFFFFFF80000000ULL  // -2GB kernel base
#define USER_VIRT_END       0x00007FFFFFFFFFFFULL  // 128TB user space

// Page table levels
#define PML4_SHIFT          39
#define PDPT_SHIFT          30
#define PD_SHIFT            21
#define PT_SHIFT            12

#define PAGE_TABLE_ENTRIES  512
#define PAGE_MASK           (PAGE_SIZE_4K - 1)

// Page table structures (aligned to 4K)
static uint64_t pml4_table[PAGE_TABLE_ENTRIES] __attribute__((aligned(4096)));
static uint64_t pdpt_kernel[PAGE_TABLE_ENTRIES] __attribute__((aligned(4096)));
static uint64_t pd_kernel[PAGE_TABLE_ENTRIES] __attribute__((aligned(4096)));
static uint64_t pdpt_user[PAGE_TABLE_ENTRIES] __attribute__((aligned(4096)));

// Forward declarations
static void clear_page_table(uint64_t *table);
static void setup_kernel_mapping(void);
static void setup_identity_mapping(void);
static int map_2mb_page(uint64_t virt_addr, uint64_t phys_addr, uint64_t flags);
static void enable_pae_and_nx(void);
static void load_page_tables(void);

/**
 * Initialize x86-64 paging
 */
int arch_memory_init(struct boot_info *boot_info)
{
    // Suppress unused parameter warning for Phase 3
    (void)boot_info;
    // Clear all page tables
    clear_page_table(pml4_table);
    clear_page_table(pdpt_kernel);
    clear_page_table(pd_kernel);
    clear_page_table(pdpt_user);
    
    // Enable PAE and NX bit support
    enable_pae_and_nx();
    
    // Set up PML4 entries
    pml4_table[0] = (uint64_t)pdpt_user | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
    pml4_table[511] = (uint64_t)pdpt_kernel | PAGE_PRESENT | PAGE_WRITABLE;
    
    // Set up kernel PDPT (maps kernel virtual space)
    pdpt_kernel[510] = (uint64_t)pd_kernel | PAGE_PRESENT | PAGE_WRITABLE;
    
    // Set up kernel mapping (map first 1GB of physical memory to kernel space)
    setup_kernel_mapping();
    
    // Set up identity mapping (map first 1GB of physical memory to same virtual addresses)
    setup_identity_mapping();
    
    // Load page tables into CR3
    load_page_tables();
    
    return 0;
}

/**
 * Enable x86-64 paging (already enabled by bootloader, but ensure settings)
 */
void arch_memory_enable(void)
{
    // Enable PAE in CR4
    uint64_t cr4;
    __asm__ volatile ("mov %%cr4, %0" : "=r" (cr4));
    cr4 |= (1 << 5);  // PAE bit
    __asm__ volatile ("mov %0, %%cr4" : : "r" (cr4) : "memory");
    
    // Enable NX bit in EFER
    uint32_t efer_low, efer_high;
    __asm__ volatile ("rdmsr" : "=a" (efer_low), "=d" (efer_high) : "c" (0xC0000080));
    efer_low |= (1 << 11);  // NXE bit is bit 11 in the low 32 bits
    __asm__ volatile ("wrmsr" : : "a" (efer_low), "d" (efer_high), "c" (0xC0000080));
    
    // Paging is already enabled by bootloader, just ensure our page tables are loaded
    load_page_tables();
}

/**
 * Map virtual memory pages
 */
int arch_memory_map_pages(uint64_t virt_addr, uint64_t phys_addr,
                          size_t size, uint32_t flags)
{
    // Convert flags to x86-64 page table flags
    uint64_t pt_flags = PAGE_PRESENT;
    
    if (flags & MEMORY_WRITABLE) {
        pt_flags |= PAGE_WRITABLE;
    }
    
    if (!(flags & MEMORY_EXECUTABLE)) {
        pt_flags |= PAGE_NX;
    }
    
    if (!(flags & MEMORY_CACHEABLE)) {
        pt_flags |= PAGE_CACHE_DISABLE;
    }
    
    // For Phase 3, implement 2MB page mapping
    uint64_t virt_aligned = virt_addr & ~(PAGE_SIZE_2M - 1);
    uint64_t phys_aligned = phys_addr & ~(PAGE_SIZE_2M - 1);
    size_t aligned_size = (size + PAGE_SIZE_2M - 1) & ~(PAGE_SIZE_2M - 1);
    
    for (size_t offset = 0; offset < aligned_size; offset += PAGE_SIZE_2M) {
        if (map_2mb_page(virt_aligned + offset, phys_aligned + offset, pt_flags) < 0) {
            return -1;
        }
    }
    
    return 0;
}

// Helper functions

/**
 * Clear page table
 */
static void clear_page_table(uint64_t *table)
{
    for (int i = 0; i < PAGE_TABLE_ENTRIES; i++) {
        table[i] = 0;
    }
}

/**
 * Set up kernel memory mapping
 */
static void setup_kernel_mapping(void)
{
    // Map first 1GB of physical memory to kernel virtual space (-2GB)
    // Using 2MB pages for simplicity
    for (int i = 0; i < 512; i++) {
        uint64_t phys_addr = i * PAGE_SIZE_2M;
        uint64_t flags = PAGE_PRESENT | PAGE_WRITABLE | PAGE_SIZE_FLAG | PAGE_GLOBAL;
        
        pd_kernel[i] = phys_addr | flags;
    }
}

/**
 * Set up identity mapping
 */
static void setup_identity_mapping(void)
{
    // Map first 1GB using 1GB pages in user PDPT for identity mapping
    // This allows bootloader and early kernel code to continue working
    uint64_t flags = PAGE_PRESENT | PAGE_WRITABLE | PAGE_SIZE_FLAG;  // 1GB page
    
    pdpt_user[0] = 0x000000000 | flags;  // First 1GB: 0x00000000 - 0x3FFFFFFF
}

/**
 * Map 2MB page
 */
static int map_2mb_page(uint64_t virt_addr, uint64_t phys_addr, uint64_t flags)
{
    // For Phase 3, we only support mapping in existing page directories
    // A full implementation would create intermediate page tables as needed
    
    if (virt_addr >= KERNEL_VIRT_BASE) {
        // Kernel space mapping
        uint64_t pd_index = (virt_addr >> PD_SHIFT) & 0x1FF;
        if (pd_index >= PAGE_TABLE_ENTRIES) {
            return -1;
        }
        
        pd_kernel[pd_index] = (phys_addr & ~PAGE_MASK) | flags | PAGE_SIZE_FLAG;
    } else {
        // User space mapping (not implemented in Phase 3)
        return -1;
    }
    
    return 0;
}

/**
 * Enable PAE and NX bit support
 */
static void enable_pae_and_nx(void)
{
    // Check if NX bit is supported
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile ("cpuid" 
                     : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
                     : "a" (0x80000001));
    
    int nx_supported = (edx >> 20) & 1;
    
    if (nx_supported) {
        // Enable NX bit in EFER if supported
        uint32_t efer_low, efer_high;
        __asm__ volatile ("rdmsr" : "=a" (efer_low), "=d" (efer_high) : "c" (0xC0000080));
        efer_low |= (1 << 11);  // Set NXE bit (bit 11 in low 32 bits)
        __asm__ volatile ("wrmsr" : : "a" (efer_low), "d" (efer_high), "c" (0xC0000080));
    }
}

/**
 * Load page tables into CR3
 */
static void load_page_tables(void)
{
    __asm__ volatile ("mov %0, %%cr3" : : "r" ((uint64_t)pml4_table) : "memory");
    
    // Flush TLB
    __asm__ volatile ("mov %%cr3, %%rax; mov %%rax, %%cr3" : : : "rax", "memory");
}
/*
 * ARM64 Memory Management Unit (MMU) Implementation
 * Phase 3: Memory Management & Kernel Loading
 */

#include <stdint.h>
#include <stddef.h>
#include "memory.h"
#include "boot_protocol.h"

// ARM64 Translation Control Register (TCR_EL1) values
#define TCR_T0SZ_48BIT      16      // 48-bit virtual address space for TTBR0
#define TCR_T1SZ_48BIT      16      // 48-bit virtual address space for TTBR1
#define TCR_TG0_4KB         0       // 4KB granule for TTBR0
#define TCR_TG1_4KB         2       // 4KB granule for TTBR1
#define TCR_SH0_INNER       3       // Inner shareable for TTBR0
#define TCR_SH1_INNER       3       // Inner shareable for TTBR1
#define TCR_ORGN0_WB_RA_WA  1       // Write-back read-allocate write-allocate for TTBR0
#define TCR_ORGN1_WB_RA_WA  1       // Write-back read-allocate write-allocate for TTBR1
#define TCR_IRGN0_WB_RA_WA  1       // Write-back read-allocate write-allocate for TTBR0
#define TCR_IRGN1_WB_RA_WA  1       // Write-back read-allocate write-allocate for TTBR1
#define TCR_IPS_48BIT       5       // 48-bit intermediate physical address size

// Memory Attribute Indirection Register (MAIR_EL1) values
#define MAIR_DEVICE_nGnRnE  0x00    // Device memory nGnRnE
#define MAIR_DEVICE_nGnRE   0x04    // Device memory nGnRE  
#define MAIR_DEVICE_GRE     0x0C    // Device memory GRE
#define MAIR_NORMAL_NC      0x44    // Normal memory, non-cacheable
#define MAIR_NORMAL_WT      0xBB    // Normal memory, write-through cacheable
#define MAIR_NORMAL_WB      0xFF    // Normal memory, write-back cacheable

// Page table entry flags
#define PTE_VALID           (1ULL << 0)
#define PTE_TABLE           (1ULL << 1)
#define PTE_USER            (1ULL << 6)
#define PTE_RDONLY          (1ULL << 7)
#define PTE_SHARED          (1ULL << 8)
#define PTE_AF              (1ULL << 10)    // Access flag
#define PTE_NG              (1ULL << 11)    // Not global
#define PTE_PXN             (1ULL << 53)    // Privileged execute never
#define PTE_UXN             (1ULL << 54)    // User execute never

// Memory attribute index (for MAIR_EL1)
#define ATTR_DEVICE_nGnRnE  0
#define ATTR_DEVICE_nGnRE   1
#define ATTR_DEVICE_GRE     2
#define ATTR_NORMAL_NC      3
#define ATTR_NORMAL_WT      4
#define ATTR_NORMAL_WB      5

// Virtual address space layout
#define KERNEL_VA_START     0xFFFF000000000000ULL  // Kernel virtual address start
#define USER_VA_END         0x0000800000000000ULL  // User virtual address end

// Page table levels (4KB granule, 48-bit VA)
#define PAGE_SHIFT          12
#define PAGE_SIZE           (1ULL << PAGE_SHIFT)
#define PAGE_MASK           (PAGE_SIZE - 1)

// Translation table entries
static uint64_t *ttbr0_l0_table __attribute__((aligned(4096)));
static uint64_t *ttbr1_l0_table __attribute__((aligned(4096)));
static uint64_t *kernel_l1_table __attribute__((aligned(4096)));
static uint64_t *kernel_l2_table __attribute__((aligned(4096)));

// Static page table storage (temporary for Phase 3)
static uint64_t page_table_storage[4][512] __attribute__((aligned(4096)));

// Forward declarations
static void setup_mair_el1(void);
static void setup_tcr_el1(void);
static uint64_t create_pte(uint64_t phys_addr, uint64_t flags);
static int map_kernel_range(uint64_t virt_start, uint64_t phys_start, 
                           size_t size, uint64_t flags);

/**
 * Initialize ARM64 MMU
 */
int arch_memory_init(struct boot_info *boot_info)
{
    // Suppress unused parameter warning for now
    (void)boot_info;
    // Initialize static page table pointers
    ttbr0_l0_table = page_table_storage[0];
    ttbr1_l0_table = page_table_storage[1]; 
    kernel_l1_table = page_table_storage[2];
    kernel_l2_table = page_table_storage[3];
    
    // Clear page tables
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 512; j++) {
            page_table_storage[i][j] = 0;
        }
    }
    
    // Set up memory attributes
    setup_mair_el1();
    
    // Set up translation control
    setup_tcr_el1();
    
    // Set up TTBR1 (kernel) page tables
    // Map kernel at high virtual addresses
    // These will be used in a full implementation
    // uint64_t kernel_phys_start = 0x40000000;  // Typical ARM64 kernel load address
    // uint64_t kernel_size = 16 * 1024 * 1024; // 16MB kernel space
    
    // Create L1 table entry for kernel space
    ttbr1_l0_table[511] = ((uint64_t)kernel_l1_table) | PTE_TABLE | PTE_VALID;
    
    // Map first 1GB of physical memory for kernel
    if (map_kernel_range(KERNEL_VA_START, 0, 0x40000000, 
                        PTE_VALID | PTE_AF | (ATTR_NORMAL_WB << 2)) < 0) {
        return -1;
    }
    
    return 0;
}

/**
 * Enable ARM64 MMU
 */
void arch_memory_enable(void)
{
    // Set TTBR0_EL1 and TTBR1_EL1
    __asm__ volatile (
        "msr ttbr0_el1, %0\n"
        "msr ttbr1_el1, %1\n"
        "isb"
        :
        : "r" ((uint64_t)ttbr0_l0_table), "r" ((uint64_t)ttbr1_l0_table)
        : "memory"
    );
    
    // Enable MMU in SCTLR_EL1
    uint64_t sctlr;
    __asm__ volatile ("mrs %0, sctlr_el1" : "=r" (sctlr));
    sctlr |= (1 << 0);  // Enable MMU (M bit)
    sctlr |= (1 << 2);  // Enable data cache (C bit) 
    sctlr |= (1 << 12); // Enable instruction cache (I bit)
    __asm__ volatile (
        "msr sctlr_el1, %0\n"
        "isb"
        :
        : "r" (sctlr)
        : "memory"
    );
}

/**
 * Map physical memory range to virtual address
 */
int arch_memory_map_pages(uint64_t virt_addr, uint64_t phys_addr, 
                          size_t size, uint32_t flags)
{
    // Convert flags to ARM64 page table flags
    uint64_t pte_flags = PTE_VALID | PTE_AF;
    
    if (flags & MEMORY_WRITABLE) {
        // Writable (clear RO bit)
    } else {
        pte_flags |= PTE_RDONLY;
    }
    
    if (!(flags & MEMORY_EXECUTABLE)) {
        pte_flags |= PTE_PXN;
    }
    
    if (flags & MEMORY_CACHEABLE) {
        pte_flags |= (ATTR_NORMAL_WB << 2);
    } else {
        pte_flags |= (ATTR_DEVICE_nGnRE << 2);
    }
    
    // For Phase 3, implement basic 2MB block mapping
    uint64_t virt_aligned = virt_addr & ~(0x200000ULL - 1);  // 2MB alignment
    uint64_t phys_aligned = phys_addr & ~(0x200000ULL - 1);
    size_t aligned_size = (size + 0x1FFFFF) & ~(0x200000ULL - 1);
    
    return map_kernel_range(virt_aligned, phys_aligned, aligned_size, pte_flags);
}

/**
 * Set up Memory Attribute Indirection Register
 */
static void setup_mair_el1(void)
{
    uint64_t mair = 0;
    
    // Index 0: Device memory nGnRnE
    mair |= ((uint64_t)MAIR_DEVICE_nGnRnE << (ATTR_DEVICE_nGnRnE * 8));
    
    // Index 1: Device memory nGnRE
    mair |= ((uint64_t)MAIR_DEVICE_nGnRE << (ATTR_DEVICE_nGnRE * 8));
    
    // Index 2: Device memory GRE
    mair |= ((uint64_t)MAIR_DEVICE_GRE << (ATTR_DEVICE_GRE * 8));
    
    // Index 3: Normal memory, non-cacheable
    mair |= ((uint64_t)MAIR_NORMAL_NC << (ATTR_NORMAL_NC * 8));
    
    // Index 4: Normal memory, write-through
    mair |= ((uint64_t)MAIR_NORMAL_WT << (ATTR_NORMAL_WT * 8));
    
    // Index 5: Normal memory, write-back
    mair |= ((uint64_t)MAIR_NORMAL_WB << (ATTR_NORMAL_WB * 8));
    
    __asm__ volatile ("msr mair_el1, %0" : : "r" (mair));
}

/**
 * Set up Translation Control Register
 */
static void setup_tcr_el1(void)
{
    uint64_t tcr = 0;
    
    // T0SZ and T1SZ: 48-bit virtual address space
    tcr |= ((uint64_t)TCR_T0SZ_48BIT << 0);
    tcr |= ((uint64_t)TCR_T1SZ_48BIT << 16);
    
    // Granule sizes: 4KB
    tcr |= ((uint64_t)TCR_TG0_4KB << 14);
    tcr |= ((uint64_t)TCR_TG1_4KB << 30);
    
    // Shareability
    tcr |= ((uint64_t)TCR_SH0_INNER << 12);
    tcr |= ((uint64_t)TCR_SH1_INNER << 28);
    
    // Outer cacheability  
    tcr |= ((uint64_t)TCR_ORGN0_WB_RA_WA << 10);
    tcr |= ((uint64_t)TCR_ORGN1_WB_RA_WA << 26);
    
    // Inner cacheability
    tcr |= ((uint64_t)TCR_IRGN0_WB_RA_WA << 8);
    tcr |= ((uint64_t)TCR_IRGN1_WB_RA_WA << 24);
    
    // Intermediate physical address size
    tcr |= ((uint64_t)TCR_IPS_48BIT << 32);
    
    __asm__ volatile ("msr tcr_el1, %0" : : "r" (tcr));
}

/**
 * Create page table entry
 */
static uint64_t create_pte(uint64_t phys_addr, uint64_t flags)
{
    return (phys_addr & ~PAGE_MASK) | flags;
}

/**
 * Map kernel memory range (simplified for Phase 3)
 */
static int map_kernel_range(uint64_t virt_start, uint64_t phys_start, 
                           size_t size, uint64_t flags)
{
    // For Phase 3: Simple 2MB block mapping in L1 table
    uint64_t virt_addr = virt_start;
    uint64_t phys_addr = phys_start;
    size_t remaining = size;
    
    while (remaining > 0) {
        // Calculate L1 index for kernel space (TTBR1)
        uint64_t l1_index = (virt_addr >> 30) & 0x1FF;  // Bits [38:30]
        
        if (l1_index >= 512) {
            return -1;  // Out of range
        }
        
        // Create 1GB block entry (for simplicity in Phase 3)
        kernel_l1_table[l1_index] = create_pte(phys_addr, flags | (1ULL << 0)); // Block entry
        
        virt_addr += 0x40000000;  // 1GB
        phys_addr += 0x40000000;
        remaining = remaining > 0x40000000 ? remaining - 0x40000000 : 0;
    }
    
    return 0;
}
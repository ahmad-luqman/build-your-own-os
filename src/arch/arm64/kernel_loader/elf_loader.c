/*
 * ARM64 ELF Kernel Loader
 * Phase 3: Memory Management & Kernel Loading
 */

#include <stdint.h>
#include <stddef.h>
#include "kernel_loader.h"
#include "memory.h"

// UEFI includes (simplified for Phase 3)
typedef uint64_t EFI_STATUS;
typedef void* EFI_HANDLE;
typedef struct {
    // Simplified UEFI system table
    void *reserved;
} EFI_SYSTEM_TABLE;

#define EFI_SUCCESS             0
#define EFI_INVALID_PARAMETER   0x8000000000000002ULL
#define EFI_OUT_OF_RESOURCES    0x8000000000000009ULL
#define EFI_NOT_FOUND           0x800000000000000EULL

// Forward declarations
static int validate_elf_for_arm64(struct elf64_header *header);
static int load_program_segments(struct elf64_header *header, void *elf_data,
                                 struct kernel_load_info *load_info);
static void setup_kernel_memory_layout(struct kernel_load_info *load_info);

/**
 * ARM64 UEFI kernel loading
 */
int arm64_load_kernel_elf(void *image_handle, void *system_table,
                          struct kernel_load_info *load_info)
{
    // For Phase 3, we'll simulate loading a kernel ELF
    // In a real implementation, this would:
    // 1. Open kernel file from ESP or boot media
    // 2. Read ELF data into memory
    // 3. Parse and load segments
    
    // Suppress unused parameter warnings for Phase 3
    (void)image_handle;
    (void)system_table;
    
    if (!load_info) {
        return -1;
    }
    
    // Initialize load info structure
    load_info->kernel_entry = NULL;
    load_info->kernel_base = NULL;
    load_info->kernel_size = 0;
    load_info->sections = NULL;
    load_info->num_sections = 0;
    
    // For Phase 3, simulate a loaded kernel
    // In reality, the kernel would be loaded from disk/image
    
    // Set up simulated kernel information
    load_info->kernel_base = (void *)0x40000000;      // Typical ARM64 kernel base
    load_info->kernel_size = 16 * 1024 * 1024;       // 16MB kernel
    load_info->kernel_entry = (void *)0x40000000;     // Entry point = base
    
    // Set up kernel memory layout
    setup_kernel_memory_layout(load_info);
    
    return 0;
}

/**
 * Validate ELF header for ARM64
 */
static int validate_elf_for_arm64(struct elf64_header *header)
{
    if (!header) {
        return -1;
    }
    
    // Check ELF magic
    if (*(uint32_t*)header->e_ident != ELF_MAGIC) {
        return -1;
    }
    
    // Check 64-bit
    if (header->e_ident[4] != ELF_CLASS_64) {
        return -1;
    }
    
    // Check little endian
    if (header->e_ident[5] != ELF_DATA_LSB) {
        return -1;
    }
    
    // Check version
    if (header->e_ident[6] != ELF_VERSION) {
        return -1;
    }
    
    // Check machine type
    if (header->e_machine != ELF_MACHINE_ARM64) {
        return -1;
    }
    
    // Check executable or shared object
    if (header->e_type != ELF_TYPE_EXEC && header->e_type != ELF_TYPE_DYN) {
        return -1;
    }
    
    return 0;
}

/**
 * Load ELF program segments
 */
static int load_program_segments(struct elf64_header *header, void *elf_data,
                                 struct kernel_load_info *load_info)
{
    if (!header || !elf_data || !load_info) {
        return -1;
    }
    
    // Get program header table
    struct elf64_program_header *phdr = (struct elf64_program_header *)
        ((uint8_t *)elf_data + header->e_phoff);
    
    uint64_t load_base = 0xFFFFFFFFFFFFFFFFULL;  // Find minimum load address
    uint64_t load_end = 0;                       // Find maximum load address
    
    // First pass: determine memory requirements
    for (int i = 0; i < header->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            if (phdr[i].p_vaddr < load_base) {
                load_base = phdr[i].p_vaddr;
            }
            
            uint64_t seg_end = phdr[i].p_vaddr + phdr[i].p_memsz;
            if (seg_end > load_end) {
                load_end = seg_end;
            }
        }
    }
    
    if (load_base == 0xFFFFFFFFFFFFFFFFULL) {
        return -1;  // No loadable segments found
    }
    
    // Set load information
    load_info->kernel_base = (void *)load_base;
    load_info->kernel_size = load_end - load_base;
    load_info->kernel_entry = (void *)header->e_entry;
    
    // Second pass: load segments into memory
    for (int i = 0; i < header->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            uint8_t *src = (uint8_t *)elf_data + phdr[i].p_offset;
            uint8_t *dst = (uint8_t *)phdr[i].p_vaddr;
            
            // Copy file data
            for (size_t j = 0; j < phdr[i].p_filesz; j++) {
                dst[j] = src[j];
            }
            
            // Zero BSS section (memsz > filesz)
            for (size_t j = phdr[i].p_filesz; j < phdr[i].p_memsz; j++) {
                dst[j] = 0;
            }
            
            // Set up memory protection
            uint32_t flags = MEMORY_READABLE;
            if (phdr[i].p_flags & PF_W) {
                flags |= MEMORY_WRITABLE;
            }
            if (phdr[i].p_flags & PF_X) {
                flags |= MEMORY_EXECUTABLE;
            }
            
            // Map segment with appropriate permissions
            arch_memory_map_pages(phdr[i].p_vaddr, phdr[i].p_paddr,
                                  phdr[i].p_memsz, flags);
        }
    }
    
    return 0;
}

/**
 * Set up kernel memory layout (simplified for Phase 3)
 */
static void setup_kernel_memory_layout(struct kernel_load_info *load_info)
{
    if (!load_info) {
        return;
    }
    
    // For Phase 3, this is simplified
    // In a full implementation, this would set up heap and stack regions
    (void)load_info;  // Suppress unused parameter warning
}

/**
 * Generic ELF validation
 */
int elf_validate_header(struct elf64_header *header)
{
    return validate_elf_for_arm64(header);
}

/**
 * Generic kernel ELF loading
 */
int kernel_load_elf(void *elf_data, size_t elf_size, 
                    struct kernel_load_info *load_info)
{
    if (!elf_data || elf_size < sizeof(struct elf64_header) || !load_info) {
        return -1;
    }
    
    struct elf64_header *header = (struct elf64_header *)elf_data;
    
    // Validate ELF header
    if (elf_validate_header(header) < 0) {
        return -1;
    }
    
    // Load program segments
    if (load_program_segments(header, elf_data, load_info) < 0) {
        return -1;
    }
    
    // Set up memory layout
    setup_kernel_memory_layout(load_info);
    
    return 0;
}

/**
 * Set up kernel memory layout (simplified for Phase 3)
 */
int kernel_setup_memory_layout(struct kernel_load_info *load_info)
{
    if (!load_info) {
        return -1;
    }
    
    // For Phase 3, this is simplified
    // In a full implementation, this would map heap and stack regions
    (void)load_info;  // Suppress unused parameter warning
    
    return 0;
}

/**
 * Transfer control to kernel (simplified for Phase 3)
 */
void kernel_transfer_control(struct kernel_load_info *load_info,
                             struct boot_info *boot_info)
{
    if (!load_info || !load_info->kernel_entry) {
        // Cannot transfer control - halt
        while (1) {
            __asm__ volatile ("wfi");
        }
    }
    
    // For Phase 3, use current stack
    // In a full implementation, this would set up a dedicated kernel stack
    
    // Jump to kernel entry point
    void (*kernel_main)(struct boot_info *) = 
        (void (*)(struct boot_info *))load_info->kernel_entry;
    
    kernel_main(boot_info);
    
    // Should never reach here
    while (1) {
        __asm__ volatile ("wfi");
    }
}
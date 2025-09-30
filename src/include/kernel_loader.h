/*
 * MiniOS Kernel Loading Interface
 * Phase 3: Memory Management & Kernel Loading
 */

#ifndef KERNEL_LOADER_H
#define KERNEL_LOADER_H

#include <stdint.h>
#include <stddef.h>
#include "memory.h"

// ELF file header constants
#define ELF_MAGIC       0x464C457F  // "\x7FELF"
#define ELF_CLASS_64    2           // 64-bit
#define ELF_DATA_LSB    1           // Little endian
#define ELF_VERSION     1           // Current version
#define ELF_TYPE_EXEC   2           // Executable file
#define ELF_TYPE_DYN    3           // Shared object

// ELF machine types
#define ELF_MACHINE_ARM64   183     // AArch64
#define ELF_MACHINE_X86_64  62      // AMD x86-64

// Program header types
#define PT_NULL     0
#define PT_LOAD     1
#define PT_DYNAMIC  2
#define PT_INTERP   3
#define PT_NOTE     4

// Program header flags
#define PF_X        1               // Execute
#define PF_W        2               // Write
#define PF_R        4               // Read

// ELF64 header structure
struct elf64_header {
    uint8_t  e_ident[16];          // Magic number and other info
    uint16_t e_type;               // Object file type
    uint16_t e_machine;            // Architecture
    uint32_t e_version;            // Object file version
    uint64_t e_entry;              // Entry point virtual address
    uint64_t e_phoff;              // Program header table file offset
    uint64_t e_shoff;              // Section header table file offset
    uint32_t e_flags;              // Processor-specific flags
    uint16_t e_ehsize;             // ELF header size in bytes
    uint16_t e_phentsize;          // Program header table entry size
    uint16_t e_phnum;              // Program header table entry count
    uint16_t e_shentsize;          // Section header table entry size
    uint16_t e_shnum;              // Section header table entry count
    uint16_t e_shstrndx;           // Section header string table index
} __attribute__((packed));

// ELF64 program header
struct elf64_program_header {
    uint32_t p_type;               // Segment type
    uint32_t p_flags;              // Segment flags
    uint64_t p_offset;             // Segment file offset
    uint64_t p_vaddr;              // Segment virtual address
    uint64_t p_paddr;              // Segment physical address
    uint64_t p_filesz;             // Segment size in file
    uint64_t p_memsz;              // Segment size in memory
    uint64_t p_align;              // Segment alignment
} __attribute__((packed));

// Kernel section information
struct kernel_section {
    uint64_t vaddr;                // Virtual address
    uint64_t paddr;                // Physical address  
    size_t size;                   // Section size
    uint32_t flags;                // Section flags
    char name[32];                 // Section name
};

// Kernel loading API (uses kernel_load_info from memory.h)

/**
 * Validate ELF header
 * @param header Pointer to ELF header
 * @return 0 if valid, negative on error
 */
int elf_validate_header(struct elf64_header *header);

/**
 * Load kernel ELF file into memory
 * @param elf_data Pointer to ELF file data
 * @param elf_size Size of ELF file
 * @param load_info Output kernel load information
 * @return 0 on success, negative on error
 */
int kernel_load_elf(void *elf_data, size_t elf_size, 
                    struct kernel_load_info *load_info);

/**
 * Set up kernel memory layout
 * @param load_info Kernel load information
 * @return 0 on success, negative on error
 */
int kernel_setup_memory_layout(struct kernel_load_info *load_info);

/**
 * Transfer control to loaded kernel
 * @param load_info Kernel load information
 * @param boot_info Boot information to pass
 */
void kernel_transfer_control(struct kernel_load_info *load_info,
                             struct boot_info *boot_info) __attribute__((noreturn));

// Architecture-specific kernel loading

#ifdef ARCH_ARM64
/**
 * ARM64 specific kernel loading from UEFI
 * @param image_handle UEFI image handle
 * @param system_table UEFI system table
 * @param load_info Output kernel load information
 * @return EFI_STATUS
 */
int arm64_load_kernel_elf(void *image_handle, void *system_table,
                          struct kernel_load_info *load_info);
#endif

#ifdef ARCH_X86_64
/**
 * x86-64 specific kernel loading from Multiboot2 modules
 * @param modules Multiboot2 modules tag
 * @param load_info Output kernel load information
 * @return 0 on success, negative on error
 */
int x86_64_load_kernel_from_multiboot(void *modules,
                                      struct kernel_load_info *load_info);
#endif

#endif // KERNEL_LOADER_H
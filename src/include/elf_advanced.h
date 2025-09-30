#ifndef ELF_ADVANCED_H
#define ELF_ADVANCED_H

#include "elf_loader.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Extended ELF structures for full parsing

// ELF section header
struct elf64_section_header {
    uint32_t sh_name;           // Section name (string table index)
    uint32_t sh_type;           // Section type
    uint64_t sh_flags;          // Section flags
    uint64_t sh_addr;           // Section virtual address
    uint64_t sh_offset;         // Section file offset
    uint64_t sh_size;           // Section size
    uint32_t sh_link;           // Link to other section
    uint32_t sh_info;           // Additional section information
    uint64_t sh_addralign;      // Section alignment
    uint64_t sh_entsize;        // Entry size if section holds table
} __attribute__((packed));

// Section types
#define SHT_NULL        0       // Section header table entry unused
#define SHT_PROGBITS    1       // Program data
#define SHT_SYMTAB      2       // Symbol table
#define SHT_STRTAB      3       // String table
#define SHT_RELA        4       // Relocation entries with addends
#define SHT_HASH        5       // Symbol hash table
#define SHT_DYNAMIC     6       // Dynamic linking information
#define SHT_NOTE        7       // Notes
#define SHT_NOBITS      8       // Program space with no data (bss)
#define SHT_REL         9       // Relocation entries, no addends
#define SHT_DYNSYM      11      // Dynamic linker symbol table

// ELF symbol table entry
struct elf64_symbol {
    uint32_t st_name;           // Symbol name (string table index)
    uint8_t st_info;            // Symbol type and binding
    uint8_t st_other;           // Symbol visibility
    uint16_t st_shndx;          // Section index
    uint64_t st_value;          // Symbol value
    uint64_t st_size;           // Symbol size
} __attribute__((packed));

// Symbol binding
#define STB_LOCAL       0       // Local symbol
#define STB_GLOBAL      1       // Global symbol
#define STB_WEAK        2       // Weak symbol

// Symbol types
#define STT_NOTYPE      0       // Symbol type is unspecified
#define STT_OBJECT      1       // Symbol is a data object
#define STT_FUNC        2       // Symbol is a code object
#define STT_SECTION     3       // Symbol associated with a section
#define STT_FILE        4       // Symbol's name is file name

// Relocation entry
struct elf64_relocation {
    uint64_t r_offset;          // Address
    uint64_t r_info;            // Relocation type and symbol index
} __attribute__((packed));

// Relocation entry with addend
struct elf64_relocation_addend {
    uint64_t r_offset;          // Address
    uint64_t r_info;            // Relocation type and symbol index
    int64_t r_addend;           // Addend
} __attribute__((packed));

// Dynamic section entry
struct elf64_dynamic {
    int64_t d_tag;              // Dynamic array tag
    union {
        uint64_t d_val;         // Integer value
        uint64_t d_ptr;         // Address value
    } d_un;
} __attribute__((packed));

// Dynamic tags
#define DT_NULL         0       // Marks end of dynamic section
#define DT_NEEDED       1       // Name of needed library
#define DT_PLTRELSZ     2       // Size in bytes of PLT relocs
#define DT_PLTGOT       3       // Processor defined value
#define DT_HASH         4       // Address of symbol hash table
#define DT_STRTAB       5       // Address of string table
#define DT_SYMTAB       6       // Address of symbol table
#define DT_RELA         7       // Address of Rela relocs
#define DT_RELASZ       8       // Total size of Rela relocs
#define DT_RELAENT      9       // Size of one Rela reloc
#define DT_STRSZ        10      // Size of string table
#define DT_SYMENT       11      // Size of one symbol table entry

// Advanced ELF loader context
struct elf_advanced_context {
    struct elf64_header *header;
    struct elf64_program_header *program_headers;
    struct elf64_section_header *section_headers;
    
    // String tables
    char *section_names;
    char *symbol_names;
    char *dynamic_strings;
    
    // Symbol table
    struct elf64_symbol *symbols;
    uint32_t symbol_count;
    
    // Dynamic section
    struct elf64_dynamic *dynamic;
    uint32_t dynamic_count;
    
    // Relocation tables
    struct elf64_relocation *relocations;
    struct elf64_relocation_addend *rela_relocations;
    uint32_t relocation_count;
    uint32_t rela_count;
    
    // Hash table for symbol lookup
    uint32_t *hash_table;
    uint32_t hash_nbuckets;
    uint32_t hash_nchains;
    
    // Loaded segments information
    void *base_address;
    size_t total_size;
};

// Shared library structure
struct shared_library {
    char name[256];             // Library name
    char path[512];             // Full path to library
    void *base_address;         // Load address
    size_t size;                // Total size
    struct elf_advanced_context *elf_ctx;
    struct shared_library *next; // Linked list
    int ref_count;              // Reference count
};

// Symbol resolution context
struct symbol_resolution {
    const char *name;           // Symbol name
    uint64_t value;             // Symbol value
    struct shared_library *library; // Defining library
    uint8_t type;               // Symbol type
    uint8_t binding;            // Symbol binding
};

// Advanced ELF loading functions
int elf_advanced_parse(const uint8_t *elf_data, size_t size, struct elf_advanced_context *ctx);
int elf_advanced_load(struct elf_advanced_context *ctx, struct user_program *program);
int elf_advanced_relocate(struct elf_advanced_context *ctx, struct user_program *program);
void elf_advanced_cleanup(struct elf_advanced_context *ctx);

// Dynamic linking functions
int dynamic_load_library(const char *path, struct shared_library **lib);
int dynamic_unload_library(struct shared_library *lib);
int dynamic_resolve_symbol(const char *name, struct symbol_resolution *resolution);
int dynamic_link_program(struct user_program *program, struct shared_library **libraries);

// Symbol table functions
int elf_build_symbol_table(struct elf_advanced_context *ctx);
int elf_find_symbol(struct elf_advanced_context *ctx, const char *name, struct elf64_symbol **symbol);
int elf_resolve_relocations(struct elf_advanced_context *ctx);

// Library management
int library_manager_init(void);
void library_manager_cleanup(void);
struct shared_library *library_find(const char *name);
int library_add_search_path(const char *path);

// Advanced program execution
int program_load_advanced(const char *path, struct user_program *program);
int program_execute_with_libs(struct user_program *program, char **library_names);

// Debugging and introspection
void elf_dump_advanced_info(struct elf_advanced_context *ctx);
void elf_dump_symbol_table(struct elf_advanced_context *ctx);
void elf_dump_dynamic_section(struct elf_advanced_context *ctx);

// Error codes for advanced ELF loading
#define ELF_ADV_SUCCESS             0
#define ELF_ADV_ERROR_INVALID_ELF   -1
#define ELF_ADV_ERROR_UNSUPPORTED   -2
#define ELF_ADV_ERROR_NO_MEMORY     -3
#define ELF_ADV_ERROR_SYMBOL_NOT_FOUND  -4
#define ELF_ADV_ERROR_RELOC_FAILED  -5
#define ELF_ADV_ERROR_LIBRARY_NOT_FOUND -6

const char *elf_advanced_error_string(int error);

#ifdef __cplusplus
}
#endif

#endif /* ELF_ADVANCED_H */
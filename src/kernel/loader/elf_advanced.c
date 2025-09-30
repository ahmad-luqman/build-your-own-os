#include "elf_advanced.h"
#include "memory.h"
#include "kernel.h"
#include "vfs.h"

// Forward declarations for string functions 
extern void *memset(void *s, int c, size_t n);
extern void *memcpy(void *dest, const void *src, size_t n);
extern int memcmp(const void *s1, const void *s2, size_t n);
extern char *strcpy(char *dest, const char *src);
extern char *strncpy(char *dest, const char *src, size_t n);
extern int strcmp(const char *s1, const char *s2);
extern int strlen(const char *s);

// Global shared library management
static struct shared_library *library_list = NULL;
static char *library_search_paths[16];
static int num_search_paths = 0;

// Enhanced ELF parsing function
int elf_advanced_parse(const uint8_t *elf_data, size_t size, struct elf_advanced_context *ctx) {
    if (!elf_data || !ctx || size < sizeof(struct elf64_header)) {
        return ELF_ADV_ERROR_INVALID_ELF;
    }

    // Clear the context
    memset(ctx, 0, sizeof(struct elf_advanced_context));
    
    // Parse ELF header
    ctx->header = (struct elf64_header *)elf_data;
    
    // Validate ELF magic
    const char elf_magic[4] = {0x7f, 'E', 'L', 'F'};
    if (memcmp(ctx->header->e_ident, elf_magic, 4) != 0) {
        return ELF_ADV_ERROR_INVALID_ELF;
    }
    
    // Check for 64-bit ELF
    if (ctx->header->e_ident[4] != 2) {  // ELFCLASS64
        return ELF_ADV_ERROR_UNSUPPORTED;
    }
    
    // Check architecture compatibility
    if (ctx->header->e_machine != EM_AARCH64 && ctx->header->e_machine != EM_X86_64) {
        return ELF_ADV_ERROR_UNSUPPORTED;
    }
    
    // Parse program headers
    if (ctx->header->e_phoff != 0 && ctx->header->e_phnum > 0) {
        ctx->program_headers = (struct elf64_program_header *)(elf_data + ctx->header->e_phoff);
    }
    
    // Parse section headers  
    if (ctx->header->e_shoff != 0 && ctx->header->e_shnum > 0) {
        ctx->section_headers = (struct elf64_section_header *)(elf_data + ctx->header->e_shoff);
        
        // Get section header string table
        if (ctx->header->e_shstrndx != SHN_UNDEF) {
            struct elf64_section_header *shstrtab = &ctx->section_headers[ctx->header->e_shstrndx];
            ctx->section_names = (char *)(elf_data + shstrtab->sh_offset);
        }
    }
    
    return ELF_ADV_SUCCESS;
}

// Enhanced ELF loading function
int elf_advanced_load(struct elf_advanced_context *ctx, struct user_program *program) {
    if (!ctx || !ctx->header || !program) {
        return ELF_ADV_ERROR_INVALID_ELF;
    }
    
    // Calculate total memory needed
    uint64_t min_addr = UINT64_MAX;
    uint64_t max_addr = 0;
    size_t total_size = 0;
    
    for (int i = 0; i < ctx->header->e_phnum; i++) {
        struct elf64_program_header *phdr = &ctx->program_headers[i];
        
        if (phdr->p_type == PT_LOAD) {
            uint64_t segment_start = phdr->p_vaddr;
            uint64_t segment_end = phdr->p_vaddr + phdr->p_memsz;
            
            if (segment_start < min_addr) {
                min_addr = segment_start;
            }
            if (segment_end > max_addr) {
                max_addr = segment_end;
            }
        }
    }
    
    if (min_addr == UINT64_MAX) {
        return ELF_ADV_ERROR_INVALID_ELF;  // No loadable segments
    }
    
    total_size = max_addr - min_addr;
    
    // Allocate memory for the program
    void *base_address = memory_alloc_pages((total_size + 0xFFF) / 0x1000);
    if (!base_address) {
        return ELF_ADV_ERROR_NO_MEMORY;
    }
    
    ctx->base_address = base_address;
    ctx->total_size = total_size;
    
    // Load segments
    for (int i = 0; i < ctx->header->e_phnum; i++) {
        struct elf64_program_header *phdr = &ctx->program_headers[i];
        
        if (phdr->p_type == PT_LOAD) {
            void *segment_addr = (uint8_t *)base_address + (phdr->p_vaddr - min_addr);
            
            // Copy file data
            if (phdr->p_filesz > 0) {
                memcpy(segment_addr, (uint8_t *)ctx->header + phdr->p_offset, phdr->p_filesz);
            }
            
            // Zero BSS section
            if (phdr->p_memsz > phdr->p_filesz) {
                memset((uint8_t *)segment_addr + phdr->p_filesz, 0, 
                       phdr->p_memsz - phdr->p_filesz);
            }
        }
    }
    
    // Set up program structure
    memset(program, 0, sizeof(struct user_program));
    program->entry_point = (void *)((uint8_t *)base_address + (ctx->header->e_entry - min_addr));
    program->load_base = base_address;
    program->image_size = total_size;
    program->status = PROGRAM_STATUS_READY;
    
    return ELF_ADV_SUCCESS;
}

// Build symbol table from ELF sections
int elf_build_symbol_table(struct elf_advanced_context *ctx) {
    if (!ctx || !ctx->section_headers) {
        return ELF_ADV_ERROR_INVALID_ELF;
    }
    
    // Find symbol table and string table sections
    for (int i = 0; i < ctx->header->e_shnum; i++) {
        struct elf64_section_header *shdr = &ctx->section_headers[i];
        
        if (shdr->sh_type == SHT_SYMTAB || shdr->sh_type == SHT_DYNSYM) {
            ctx->symbols = (struct elf64_symbol *)((uint8_t *)ctx->header + shdr->sh_offset);
            ctx->symbol_count = shdr->sh_size / sizeof(struct elf64_symbol);
            
            // Find associated string table
            if (shdr->sh_link < ctx->header->e_shnum) {
                struct elf64_section_header *strtab = &ctx->section_headers[shdr->sh_link];
                ctx->symbol_names = (char *)((uint8_t *)ctx->header + strtab->sh_offset);
            }
        } else if (shdr->sh_type == SHT_DYNAMIC) {
            ctx->dynamic = (struct elf64_dynamic *)((uint8_t *)ctx->header + shdr->sh_offset);
            ctx->dynamic_count = shdr->sh_size / sizeof(struct elf64_dynamic);
        }
    }
    
    return ELF_ADV_SUCCESS;
}

// Find a symbol by name
int elf_find_symbol(struct elf_advanced_context *ctx, const char *name, struct elf64_symbol **symbol) {
    if (!ctx || !ctx->symbols || !ctx->symbol_names || !name) {
        return ELF_ADV_ERROR_SYMBOL_NOT_FOUND;
    }
    
    for (uint32_t i = 0; i < ctx->symbol_count; i++) {
        struct elf64_symbol *sym = &ctx->symbols[i];
        const char *sym_name = &ctx->symbol_names[sym->st_name];
        
        if (strcmp(sym_name, name) == 0) {
            *symbol = sym;
            return ELF_ADV_SUCCESS;
        }
    }
    
    return ELF_ADV_ERROR_SYMBOL_NOT_FOUND;
}

// Perform basic relocations (simplified)
int elf_advanced_relocate(struct elf_advanced_context *ctx, struct user_program *program) {
    if (!ctx || !program) {
        return ELF_ADV_ERROR_INVALID_ELF;
    }
    
    // For now, we'll assume relocations are minimal
    // In a full implementation, this would process relocation sections
    (void)ctx;    // Suppress unused parameter warning
    (void)program;
    
    return ELF_ADV_SUCCESS;
}

// Cleanup ELF context
void elf_advanced_cleanup(struct elf_advanced_context *ctx) {
    if (!ctx) {
        return;
    }
    
    if (ctx->base_address) {
        memory_free_pages(ctx->base_address, (ctx->total_size + 0xFFF) / 0x1000);
    }
    
    memset(ctx, 0, sizeof(struct elf_advanced_context));
}

// Library management initialization
int library_manager_init(void) {
    library_list = NULL;
    num_search_paths = 0;
    
    // Add default search paths
    library_add_search_path("/lib");
    library_add_search_path("/usr/lib");
    
    return 0;
}

// Add a library search path
int library_add_search_path(const char *path) {
    if (num_search_paths >= 16 || !path) {
        return -1;
    }
    
    int len = strlen(path) + 1;
    library_search_paths[num_search_paths] = memory_alloc(len, 8);
    if (!library_search_paths[num_search_paths]) {
        return ELF_ADV_ERROR_NO_MEMORY;
    }
    
    strcpy(library_search_paths[num_search_paths], path);
    num_search_paths++;
    
    return 0;
}

// Find a loaded library by name
struct shared_library *library_find(const char *name) {
    struct shared_library *lib = library_list;
    
    while (lib) {
        if (strcmp(lib->name, name) == 0) {
            return lib;
        }
        lib = lib->next;
    }
    
    return NULL;
}

// Load a shared library (simplified implementation)
int dynamic_load_library(const char *path, struct shared_library **lib) {
    if (!path || !lib) {
        return ELF_ADV_ERROR_LIBRARY_NOT_FOUND;
    }
    
    // Check if already loaded
    const char *name = path;  // Simplified - should extract filename
    struct shared_library *existing = library_find(name);
    if (existing) {
        existing->ref_count++;
        *lib = existing;
        return ELF_ADV_SUCCESS;
    }
    
    // Allocate new library structure
    struct shared_library *new_lib = memory_alloc(sizeof(struct shared_library), 8);
    if (!new_lib) {
        return ELF_ADV_ERROR_NO_MEMORY;
    }
    
    memset(new_lib, 0, sizeof(struct shared_library));
    strncpy(new_lib->name, name, sizeof(new_lib->name) - 1);
    strncpy(new_lib->path, path, sizeof(new_lib->path) - 1);
    new_lib->ref_count = 1;
    
    // Add to library list
    new_lib->next = library_list;
    library_list = new_lib;
    
    *lib = new_lib;
    return ELF_ADV_SUCCESS;
}

// Unload a shared library
int dynamic_unload_library(struct shared_library *lib) {
    if (!lib) {
        return -1;
    }
    
    lib->ref_count--;
    if (lib->ref_count <= 0) {
        // Remove from list and free memory
        // This is simplified - should properly unmap memory
        if (lib->elf_ctx) {
            elf_advanced_cleanup(lib->elf_ctx);
            memory_free(lib->elf_ctx);
        }
        
        // Remove from linked list
        struct shared_library **current = &library_list;
        while (*current && *current != lib) {
            current = &(*current)->next;
        }
        if (*current) {
            *current = lib->next;
        }
        
        memory_free(lib);
    }
    
    return 0;
}

// Resolve a symbol across all loaded libraries
int dynamic_resolve_symbol(const char *name, struct symbol_resolution *resolution) {
    if (!name || !resolution) {
        return ELF_ADV_ERROR_SYMBOL_NOT_FOUND;
    }
    
    struct shared_library *lib = library_list;
    
    while (lib) {
        if (lib->elf_ctx) {
            struct elf64_symbol *symbol;
            if (elf_find_symbol(lib->elf_ctx, name, &symbol) == ELF_ADV_SUCCESS) {
                resolution->name = name;
                resolution->value = symbol->st_value;
                resolution->library = lib;
                resolution->type = symbol->st_info & 0xF;
                resolution->binding = (symbol->st_info >> 4) & 0xF;
                return ELF_ADV_SUCCESS;
            }
        }
        lib = lib->next;
    }
    
    return ELF_ADV_ERROR_SYMBOL_NOT_FOUND;
}

// Advanced program loading with full ELF support
int program_load_advanced(const char *path, struct user_program *program) {
    if (!path || !program) {
        return ELF_ADV_ERROR_INVALID_ELF;
    }
    
    // Open the ELF file
    int fd = vfs_open(path, 0, 0);
    if (fd < 0) {
        return ELF_ADV_ERROR_LIBRARY_NOT_FOUND;
    }
    
    // Get file size (simplified - would use fstat in full implementation)
    size_t file_size = 64 * 1024;  // Assume max 64KB for now
    uint8_t *elf_data = memory_alloc(file_size, 8);
    if (!elf_data) {
        vfs_close(fd);
        return ELF_ADV_ERROR_NO_MEMORY;
    }
    
    // Read ELF file
    ssize_t bytes_read = vfs_read(fd, elf_data, file_size);
    vfs_close(fd);
    
    if (bytes_read <= 0) {
        memory_free(elf_data);
        return ELF_ADV_ERROR_LIBRARY_NOT_FOUND;
    }
    
    // Parse and load ELF
    struct elf_advanced_context *ctx = memory_alloc(sizeof(struct elf_advanced_context), 8);
    if (!ctx) {
        memory_free(elf_data);
        return ELF_ADV_ERROR_NO_MEMORY;
    }
    
    int result = elf_advanced_parse(elf_data, bytes_read, ctx);
    if (result != ELF_ADV_SUCCESS) {
        memory_free(ctx);
        memory_free(elf_data);
        return result;
    }
    
    result = elf_advanced_load(ctx, program);
    if (result != ELF_ADV_SUCCESS) {
        elf_advanced_cleanup(ctx);
        memory_free(ctx);
        memory_free(elf_data);
        return result;
    }
    
    // Build symbol table
    elf_build_symbol_table(ctx);
    
    // Perform relocations
    result = elf_advanced_relocate(ctx, program);
    if (result != ELF_ADV_SUCCESS) {
        elf_advanced_cleanup(ctx);
        memory_free(ctx);
        memory_free(elf_data);
        return result;
    }
    
    // Clean up temporary data
    memory_free(elf_data);
    
    // Store context for later use
    program->elf_context = ctx;
    
    return ELF_ADV_SUCCESS;
}

// Library management cleanup
void library_manager_cleanup(void) {
    struct shared_library *lib = library_list;
    
    while (lib) {
        struct shared_library *next = lib->next;
        dynamic_unload_library(lib);
        lib = next;
    }
    
    for (int i = 0; i < num_search_paths; i++) {
        if (library_search_paths[i]) {
            memory_free(library_search_paths[i]);
        }
    }
    
    num_search_paths = 0;
    library_list = NULL;
}

// Debugging functions
void elf_dump_advanced_info(struct elf_advanced_context *ctx) {
    if (!ctx || !ctx->header) {
        early_print("ELF: Invalid context\n");
        return;
    }
    
    early_print("ELF Advanced Info:\n");
    // Note: early_print only supports strings, not formatted output
    early_print("  Entry point, headers, and size information available\n");
}

void elf_dump_symbol_table(struct elf_advanced_context *ctx) {
    if (!ctx || !ctx->symbols || !ctx->symbol_names) {
        early_print("ELF: No symbol table\n");
        return;
    }
    
    early_print("Symbol Table: (limited output)\n");
    // Note: Can only print limited info without formatted printing
}

// Error string conversion
const char *elf_advanced_error_string(int error) {
    switch (error) {
        case ELF_ADV_SUCCESS: return "Success";
        case ELF_ADV_ERROR_INVALID_ELF: return "Invalid ELF format";
        case ELF_ADV_ERROR_UNSUPPORTED: return "Unsupported ELF feature";
        case ELF_ADV_ERROR_NO_MEMORY: return "Out of memory";
        case ELF_ADV_ERROR_SYMBOL_NOT_FOUND: return "Symbol not found";
        case ELF_ADV_ERROR_RELOC_FAILED: return "Relocation failed";
        case ELF_ADV_ERROR_LIBRARY_NOT_FOUND: return "Library not found";
        default: return "Unknown error";
    }
}
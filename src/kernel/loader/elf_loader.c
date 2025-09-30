#include "elf_loader.h"
#include "elf_advanced.h"
#include "memory.h"
#include "vfs.h"
#include "process.h"
#include "kernel.h"

// Forward declarations for string functions 
extern void *memset(void *s, int c, size_t n);
extern void *memcpy(void *dest, const void *src, size_t n);
extern char *strcpy(char *dest, const char *src);
extern char *strncpy(char *dest, const char *src, size_t n);
extern int strcmp(const char *s1, const char *s2);

// Global program table to track loaded programs
static struct user_program program_table[MAX_TASKS];
static int program_count = 0;

// Load user program from filesystem (simplified)
int user_program_load(const char *path, struct user_program *program) {
    if (!path || !program) {
        return USER_PROGRAM_ERROR_INVALID_FORMAT;
    }
    
    // Open the program file
    int fd = vfs_open(path, 0, 0);
    if (fd < 0) {
        return USER_PROGRAM_ERROR_FILE_NOT_FOUND;
    }
    
    // For Phase 7, we'll do a simplified load
    // In a full implementation, this would parse ELF format
    
    // Initialize program structure
    memset(program, 0, sizeof(struct user_program));
    strncpy(program->name, path, sizeof(program->name) - 1);
    
    // Allocate basic memory regions
    program->stack_size = USER_STACK_SIZE;
    program->stack_base = user_alloc_pages(program->stack_size);
    if (!program->stack_base) {
        vfs_close(fd);
        return USER_PROGRAM_ERROR_NO_MEMORY;
    }
    
    program->heap_size = USER_HEAP_SIZE;
    program->heap_base = user_alloc_pages(program->heap_size);
    if (!program->heap_base) {
        user_free_pages(program->stack_base, program->stack_size);
        vfs_close(fd);
        return USER_PROGRAM_ERROR_NO_MEMORY;
    }
    
    // Set up a dummy entry point for demonstration
    program->entry_point = (void *)USER_PROGRAM_BASE;
    program->load_base = (void *)USER_PROGRAM_BASE;
    program->image_size = 4096;  // 4KB program
    program->status = PROGRAM_STATUS_READY;
    
    vfs_close(fd);
    return 0;
}

// Execute a loaded user program
int user_program_execute(struct user_program *program) {
    if (!program || program->status != PROGRAM_STATUS_READY) {
        return -1;
    }
    
    // Create a new task for the program
    // For now, we'll use a dummy task entry point
    program->pid = process_create((task_entry_t)program->entry_point, 
                                  program, program->name, PRIORITY_NORMAL);
    
    if ((int)program->pid < 0) {
        return -1;
    }
    
    program->status = PROGRAM_STATUS_RUNNING;
    return (int)program->pid;
}

// Clean up program resources
void user_program_cleanup(struct user_program *program) {
    if (!program) {
        return;
    }
    
    // Free memory regions
    if (program->stack_base) {
        user_free_pages(program->stack_base, program->stack_size);
    }
    
    if (program->heap_base) {
        user_free_pages(program->heap_base, program->heap_size);
    }
    
    // Clean up arguments
    cleanup_program_arguments(program);
    
    program->status = PROGRAM_STATUS_TERMINATED;
}

// Wait for user program completion
int user_program_wait(uint32_t pid, int *status) {
    // For Phase 7, this is a stub
    // In a full implementation, this would wait for the process to exit
    (void)pid;  // Suppress unused parameter warning
    if (status) {
        *status = 0;
    }
    return 0;
}

// Create a new user program
int program_create(const char *path, int argc, char *argv[], struct user_program *program) {
    // Load the program
    int result = user_program_load(path, program);
    if (result != 0) {
        return result;
    }
    
    // Set up program arguments
    result = setup_program_arguments(program, argc, argv);
    if (result != 0) {
        user_program_cleanup(program);
        return result;
    }
    
    // Add to program table
    if (program_count < MAX_TASKS) {
        program_table[program_count] = *program;
        program_count++;
    }
    
    return 0;
}

// Execute a loaded program
int program_execute(struct user_program *program) {
    return user_program_execute(program);
}

// Clean up program resources
void program_cleanup(struct user_program *program) {
    user_program_cleanup(program);
}

// Set up program arguments
int setup_program_arguments(struct user_program *program, int argc, char *argv[]) {
    if (!program) {
        return -1;
    }
    
    program->argc = argc;
    
    if (argc == 0 || !argv) {
        program->argv = NULL;
        return 0;
    }
    
    // Allocate argv array
    program->argv = memory_alloc(sizeof(char *) * (argc + 1), 8);
    if (!program->argv) {
        return USER_PROGRAM_ERROR_NO_MEMORY;
    }
    
    // Copy argument strings
    for (int i = 0; i < argc; i++) {
        int len = strlen(argv[i]) + 1;
        program->argv[i] = memory_alloc(len, 8);
        if (!program->argv[i]) {
            // Clean up on failure
            for (int j = 0; j < i; j++) {
                memory_free(program->argv[j]);
            }
            memory_free(program->argv);
            program->argv = NULL;
            return USER_PROGRAM_ERROR_NO_MEMORY;
        }
        strcpy(program->argv[i], argv[i]);
    }
    program->argv[argc] = NULL;
    
    return 0;
}

// Clean up program arguments
void cleanup_program_arguments(struct user_program *program) {
    if (!program || !program->argv) {
        return;
    }
    
    for (int i = 0; i < program->argc; i++) {
        if (program->argv[i]) {
            memory_free(program->argv[i]);
        }
    }
    
    memory_free(program->argv);
    program->argv = NULL;
    program->argc = 0;
}

// Enhanced program loading with advanced ELF support
int program_load_enhanced(const char *path, struct user_program *program) {
    // Try advanced ELF loading first
    int result = program_load_advanced(path, program);
    if (result == ELF_ADV_SUCCESS) {
        return 0;  // Success
    }
    
    // Fall back to basic loading
    return user_program_load(path, program);
}

// Utility functions for memory management
void *user_alloc_pages(size_t size) {
    // Align size to page boundary
    size = (size + 0xFFF) & ~0xFFF;
    return memory_alloc(size, 0x1000);  // Page-aligned allocation
}

void user_free_pages(void *addr, size_t size) {
    (void)size;  // Suppress unused parameter warning
    if (addr) {
        memory_free(addr);
    }
}

int user_map_memory(void *vaddr, size_t size, uint32_t flags) {
    // For this simple implementation, we assume memory is already mapped
    (void)vaddr;   // Suppress unused parameter warnings
    (void)size;
    (void)flags;
    return 0;
}

void user_unmap_memory(void *vaddr, size_t size) {
    // For this simple implementation, we don't need to unmap
    (void)vaddr;   // Suppress unused parameter warnings
    (void)size;
}

// Process creation functions
int process_exec(const char *path, int argc, char *argv[]) {
    struct user_program program;
    
    int result = program_create(path, argc, argv, &program);
    if (result != 0) {
        return result;
    }
    
    return program_execute(&program);
}

int process_spawn(const char *path, int argc, char *argv[]) {
    return process_exec(path, argc, argv);
}

// Stub for process_fork - not implemented in Phase 7
int process_fork(void) {
    return -1;  // Not implemented
}

// Error handling
const char *user_program_error_string(int error_code) {
    switch (error_code) {
        case 0: return "Success";
        case USER_PROGRAM_ERROR_INVALID_FORMAT: return "Invalid program format";
        case USER_PROGRAM_ERROR_FILE_NOT_FOUND: return "Program file not found";
        case USER_PROGRAM_ERROR_NO_MEMORY: return "Out of memory";
        case USER_PROGRAM_ERROR_LOAD_FAILED: return "Failed to load program";
        case USER_PROGRAM_ERROR_EXEC_FAILED: return "Failed to execute program";
        default: return "Unknown error";
    }
}
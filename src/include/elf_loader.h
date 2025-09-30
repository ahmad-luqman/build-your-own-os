#ifndef ELF_LOADER_H
#define ELF_LOADER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Use existing ELF definitions from kernel_loader.h
#include "kernel_loader.h"

// User program structure
struct user_program {
    char name[64];                  // Program name
    uint32_t pid;                   // Process ID
    void *entry_point;              // Program entry point
    void *load_base;                // Base load address
    size_t image_size;              // Total image size
    
    // Memory layout
    void *text_base;                // Text segment base
    size_t text_size;               // Text segment size
    void *data_base;                // Data segment base
    size_t data_size;               // Data segment size
    void *bss_base;                 // BSS segment base
    size_t bss_size;                // BSS segment size
    
    // Stack and heap
    void *stack_base;               // Stack base address
    size_t stack_size;              // Stack size
    void *heap_base;                // Heap base address
    size_t heap_size;               // Heap size
    
    // Arguments
    int argc;                       // Argument count
    char **argv;                    // Argument vector
    char **envp;                    // Environment vector
    
    // Process state
    int exit_code;                  // Exit code
    int status;                     // Process status
};

// Program status values
#define PROGRAM_STATUS_LOADING      0
#define PROGRAM_STATUS_READY        1
#define PROGRAM_STATUS_RUNNING      2
#define PROGRAM_STATUS_TERMINATED   3
#define PROGRAM_STATUS_ERROR        -1

// Memory regions for user programs
#define USER_PROGRAM_BASE           0x400000    // 4MB base address
#define USER_STACK_SIZE             (64 * 1024) // 64KB stack
#define USER_HEAP_SIZE              (1024 * 1024) // 1MB heap

// User program loading functions
int user_program_load(const char *path, struct user_program *program);
int user_program_execute(struct user_program *program);
void user_program_cleanup(struct user_program *program);
int user_program_wait(uint32_t pid, int *status);

// Program execution functions
int program_create(const char *path, int argc, char *argv[], struct user_program *program);
int program_execute(struct user_program *program);
void program_cleanup(struct user_program *program);

// Memory management for user programs
void *user_alloc_pages(size_t size);
void user_free_pages(void *addr, size_t size);
int user_map_memory(void *vaddr, size_t size, uint32_t flags);
void user_unmap_memory(void *vaddr, size_t size);

// Process creation utilities
int process_fork(void);
int process_exec(const char *path, int argc, char *argv[]);
int process_spawn(const char *path, int argc, char *argv[]);

// Argument handling
int setup_program_arguments(struct user_program *program, int argc, char *argv[]);
void cleanup_program_arguments(struct user_program *program);

// Error codes
#define USER_PROGRAM_ERROR_INVALID_FORMAT   -1
#define USER_PROGRAM_ERROR_FILE_NOT_FOUND   -2
#define USER_PROGRAM_ERROR_NO_MEMORY        -3
#define USER_PROGRAM_ERROR_LOAD_FAILED      -4
#define USER_PROGRAM_ERROR_EXEC_FAILED      -5

const char *user_program_error_string(int error_code);

#ifdef __cplusplus
}
#endif

#endif /* ELF_LOADER_H */
#include "../stdlib.h"
#include "../string.h"

// System call interface
extern void sys_exit(int status);
extern void *sys_brk(void *addr);

// Simple heap allocator
static void *heap_start = NULL;
static void *heap_end = NULL;
static size_t heap_size = 0;

// Simple free block list
struct free_block {
    size_t size;
    struct free_block *next;
};

static struct free_block *free_list = NULL;

// Initialize heap 
static void _heap_init(void) {
    if (!heap_start) {
        // Request initial heap from kernel (simplified)
        heap_size = 1024 * 1024;  // 1MB initial heap
        heap_start = sys_brk(NULL);
        heap_end = sys_brk((char *)heap_start + heap_size);
        
        // Initialize free list with entire heap
        free_list = (struct free_block *)heap_start;
        free_list->size = heap_size - sizeof(struct free_block);
        free_list->next = NULL;
    }
}

// Simple malloc implementation
void *malloc(size_t size) {
    if (size == 0) return NULL;
    
    _heap_init();
    
    // Align to 8-byte boundary
    size = (size + 7) & ~7;
    
    // Find suitable free block
    struct free_block **current = &free_list;
    while (*current) {
        if ((*current)->size >= size) {
            struct free_block *block = *current;
            
            // If block is much larger, split it
            if (block->size > size + sizeof(struct free_block)) {
                struct free_block *new_block = 
                    (struct free_block *)((char *)block + sizeof(struct free_block) + size);
                new_block->size = block->size - size - sizeof(struct free_block);
                new_block->next = block->next;
                block->next = new_block;
                block->size = size;
            }
            
            // Remove block from free list
            *current = block->next;
            
            // Return data portion (after header)
            return (char *)block + sizeof(struct free_block);
        }
        current = &(*current)->next;
    }
    
    return NULL;  // No suitable block found
}

// Simple free implementation
void free(void *ptr) {
    if (!ptr) return;
    
    // Get block header
    struct free_block *block = 
        (struct free_block *)((char *)ptr - sizeof(struct free_block));
    
    // Add to free list
    block->next = free_list;
    free_list = block;
}

// Calloc implementation
void *calloc(size_t num, size_t size) {
    size_t total_size = num * size;
    void *ptr = malloc(total_size);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

// Simple realloc (not optimized)
void *realloc(void *ptr, size_t size) {
    if (!ptr) {
        return malloc(size);
    }
    
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    
    void *new_ptr = malloc(size);
    if (new_ptr) {
        // Copy old data (we don't know original size, so copy conservatively)
        memcpy(new_ptr, ptr, size);  // This is not correct - should track original size
        free(ptr);
    }
    
    return new_ptr;
}

// Process control
void exit(int status) {
    sys_exit(status);
    // This should not return
    while (1) {}
}

void abort(void) {
    exit(EXIT_FAILURE);
}

// Simple atexit implementation (limited)
static void (*exit_functions[32])(void);
static int num_exit_functions = 0;

int atexit(void (*func)(void)) {
    if (num_exit_functions >= 32 || !func) {
        return -1;
    }
    
    exit_functions[num_exit_functions++] = func;
    return 0;
}

// String to number conversions
int atoi(const char *str) {
    if (!str) return 0;
    
    int result = 0;
    int sign = 1;
    
    // Skip whitespace
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') {
        str++;
    }
    
    // Handle sign
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }
    
    // Convert digits
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return result * sign;
}

long atol(const char *str) {
    if (!str) return 0;
    
    long result = 0;
    int sign = 1;
    
    // Skip whitespace
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') {
        str++;
    }
    
    // Handle sign
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }
    
    // Convert digits
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return result * sign;
}

// Pseudo-random number generator (simple linear congruential generator)
static unsigned long next_rand = 1;

int rand(void) {
    next_rand = next_rand * 1103515245 + 12345;
    return (unsigned int)(next_rand / 65536) % 32768;
}

void srand(unsigned int seed) {
    next_rand = seed;
}

// Arithmetic functions
int abs(int x) {
    return x < 0 ? -x : x;
}

long labs(long x) {
    return x < 0 ? -x : x;
}

div_t div(int numer, int denom) {
    div_t result;
    result.quot = numer / denom;
    result.rem = numer % denom;
    return result;
}

ldiv_t ldiv(long numer, long denom) {
    ldiv_t result;
    result.quot = numer / denom;
    result.rem = numer % denom;
    return result;
}

// Simple environment support (stub)
char *getenv(const char *name) {
    (void)name;  // Suppress unused parameter warning
    return NULL; // No environment variables in this simple implementation
}

// System command execution (stub)
int system(const char *command) {
    (void)command;  // Suppress unused parameter warning
    return -1;      // Not implemented
}
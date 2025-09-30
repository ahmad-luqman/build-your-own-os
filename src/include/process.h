#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Task states
#define TASK_STATE_READY        0
#define TASK_STATE_RUNNING      1
#define TASK_STATE_BLOCKED      2
#define TASK_STATE_TERMINATED   3

// Process priorities
#define PRIORITY_HIGH           0
#define PRIORITY_NORMAL         1
#define PRIORITY_LOW            2
#define PRIORITY_IDLE           3

// Maximum number of tasks
#define MAX_TASKS               32

// CPU context structure (architecture-specific parts defined in arch headers)
struct cpu_context {
#ifdef __aarch64__
    // ARM64 context - EL0/EL1 registers
    uint64_t x0, x1, x2, x3, x4, x5, x6, x7;
    uint64_t x8, x9, x10, x11, x12, x13, x14, x15;
    uint64_t x16, x17, x18, x19, x20, x21, x22, x23;
    uint64_t x24, x25, x26, x27, x28, x29, x30;
    uint64_t sp_el0, sp_el1;
    uint64_t elr_el1;
    uint64_t spsr_el1;
    uint64_t ttbr0_el1;
    uint64_t ttbr1_el1;
#elif defined(__x86_64__)
    // x86-64 context - Ring 0/Ring 3 registers
    uint64_t rax, rbx, rcx, rdx;
    uint64_t rsi, rdi, rbp, rsp;
    uint64_t r8, r9, r10, r11;
    uint64_t r12, r13, r14, r15;
    uint64_t rip, rflags;
    uint64_t cs, ss, ds, es, fs, gs;
    uint64_t cr3;  // Page directory base
#endif
};

// Task control block
struct task {
    uint32_t pid;                       // Process ID
    uint32_t state;                     // Task state
    uint32_t priority;                  // Task priority
    char name[32];                      // Task name
    
    // Memory management
    void *stack_base;                   // Stack base address
    size_t stack_size;                  // Stack size
    void *heap_base;                    // Heap base address (for future use)
    size_t heap_size;                   // Heap size (for future use)
    
    // CPU context
    struct cpu_context context;         // Saved CPU state
    
    // Scheduling
    uint64_t time_slice;               // Remaining time slice
    uint64_t total_runtime;            // Total runtime
    uint64_t last_scheduled;           // Last schedule time
    
    // Task list management
    struct task *next;                 // Next task in queue
    struct task *prev;                 // Previous task in queue
    
    // Exit status
    int exit_code;                     // Exit code when terminated
};

// Scheduler structure
struct scheduler {
    struct task *current_task;         // Currently running task
    struct task *ready_queue;          // Ready task queue
    struct task *blocked_queue;        // Blocked task queue
    
    uint32_t num_tasks;               // Total number of tasks
    uint32_t next_pid;                // Next available PID
    
    uint64_t tick_count;              // Scheduler tick counter
    uint32_t time_slice_quantum;      // Time slice length (in timer ticks)
    
    // Statistics
    uint64_t context_switches;        // Number of context switches
    uint64_t total_ticks;             // Total scheduler ticks
};

// Function pointers for task entry points
typedef void (*task_entry_t)(void *arg);

// Process management functions
int process_init(void);
int process_create(task_entry_t entry, void *arg, const char *name, uint32_t priority);
void process_yield(void);
void process_sleep(uint64_t ticks);
void process_exit(int exit_code);
int process_kill(uint32_t pid);

// Scheduler functions
void scheduler_init(void);
void scheduler_tick(void);
void scheduler_start(void);
struct task *scheduler_get_current_task(void);
struct task *scheduler_pick_next_task(void);
void scheduler_switch_to_task(struct task *task);
void scheduler_add_task(struct task *task);
void scheduler_remove_task(struct task *task);

// Context switching (architecture-specific)
void context_switch(struct cpu_context *old_ctx, struct cpu_context *new_ctx);
void arch_setup_task_context(struct cpu_context *ctx, task_entry_t entry, void *arg, void *stack_top);

// Task management utilities
struct task *task_find_by_pid(uint32_t pid);
void task_dump_info(struct task *task);
void scheduler_dump_info(void);

// Built-in tasks
void idle_task(void *arg);
void init_task(void *arg);

// Stack management
#define TASK_STACK_SIZE    8192        // Default task stack size (8KB)
void *allocate_task_stack(size_t size);
void free_task_stack(void *stack_base, size_t size);

// Process statistics
struct process_stats {
    uint32_t total_tasks;
    uint32_t ready_tasks;
    uint32_t blocked_tasks;
    uint64_t context_switches;
    uint64_t scheduler_ticks;
    uint32_t current_pid;
};

int process_get_stats(struct process_stats *stats);

#ifdef __cplusplus
}
#endif

#endif /* PROCESS_H */
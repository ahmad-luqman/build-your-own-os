/**
 * Process Management Implementation
 * 
 * Cross-platform process management for MiniOS
 * Provides task creation, scheduling, and context switching
 */

#include "process.h"
#include "memory.h"
#include "timer.h"
#include "kernel.h"

// External scheduler reference (defined in scheduler.c)
extern struct scheduler g_scheduler;

// Task pool for static allocation
static struct task g_task_pool[MAX_TASKS];
static int g_task_pool_bitmap = 0;

// Stack memory pool (simple implementation)
static char g_stack_pool[MAX_TASKS * TASK_STACK_SIZE] __attribute__((aligned(16)));
static int g_stack_pool_bitmap = 0;

// Process management initialization
int process_init(void) {
    early_print("Initializing process management...\n");
    
    // Initialize scheduler
    memset(&g_scheduler, 0, sizeof(g_scheduler));
    g_scheduler.time_slice_quantum = 10;  // 10 timer ticks per slice
    g_scheduler.next_pid = 1;  // PID 0 reserved for kernel
    
    // Initialize task pool
    memset(g_task_pool, 0, sizeof(g_task_pool));
    g_task_pool_bitmap = 0;
    
    // Initialize stack pool
    g_stack_pool_bitmap = 0;
    
    early_print("Process management initialized\n");
    return 0;
}

// Allocate task from pool
static struct task *allocate_task(void) {
    for (int i = 0; i < MAX_TASKS; i++) {
        if (!(g_task_pool_bitmap & (1 << i))) {
            g_task_pool_bitmap |= (1 << i);
            return &g_task_pool[i];
        }
    }
    return NULL;  // No free tasks
}

// Free task to pool
static void free_task(struct task *task) {
    if (!task) return;
    
    int index = task - g_task_pool;
    if (index >= 0 && index < MAX_TASKS) {
        g_task_pool_bitmap &= ~(1 << index);
        memset(task, 0, sizeof(struct task));
    }
}

// Allocate task stack
void *allocate_task_stack(size_t size) {
    if (size != TASK_STACK_SIZE) {
        return NULL;  // Only support fixed size for now
    }
    
    for (int i = 0; i < MAX_TASKS; i++) {
        if (!(g_stack_pool_bitmap & (1 << i))) {
            g_stack_pool_bitmap |= (1 << i);
            return &g_stack_pool[i * TASK_STACK_SIZE];
        }
    }
    return NULL;  // No free stacks
}

// Free task stack
void free_task_stack(void *stack_base, size_t size) {
    if (!stack_base || size != TASK_STACK_SIZE) return;
    
    char *stack_ptr = (char *)stack_base;
    if (stack_ptr < g_stack_pool || stack_ptr >= g_stack_pool + sizeof(g_stack_pool)) {
        return;  // Not from our pool
    }
    
    int index = (stack_ptr - g_stack_pool) / TASK_STACK_SIZE;
    if (index >= 0 && index < MAX_TASKS) {
        g_stack_pool_bitmap &= ~(1 << index);
    }
}

// Create a new process
int process_create(task_entry_t entry, void *arg, const char *name, uint32_t priority) {
    if (!entry || !name) return -1;
    
    // Allocate task structure
    struct task *task = allocate_task();
    if (!task) {
        early_print("ERROR: No free task slots available\n");
        return -1;
    }
    
    // Allocate stack
    void *stack_base = allocate_task_stack(TASK_STACK_SIZE);
    if (!stack_base) {
        free_task(task);
        early_print("ERROR: No free stack slots available\n");
        return -1;
    }
    
    // Initialize task structure
    task->pid = g_scheduler.next_pid++;
    task->state = TASK_STATE_READY;
    task->priority = priority;
    strncpy(task->name, name, sizeof(task->name) - 1);
    task->name[sizeof(task->name) - 1] = '\0';
    
    task->stack_base = stack_base;
    task->stack_size = TASK_STACK_SIZE;
    task->time_slice = g_scheduler.time_slice_quantum;
    task->total_runtime = 0;
    task->last_scheduled = 0;
    
    // Setup initial context (architecture-specific)
    void *stack_top = (char *)stack_base + TASK_STACK_SIZE;
    arch_setup_task_context(&task->context, entry, arg, stack_top);
    
    // Add to scheduler
    scheduler_add_task(task);
    
    early_print("Created process: ");
    early_print(name);
    early_print(" (PID ");
    char pid_str[16];
    early_print(itoa(task->pid, pid_str, 10));
    early_print(")\n");
    
    return task->pid;
}

// Yield CPU to other processes
void process_yield(void) {
    if (g_scheduler.current_task) {
        g_scheduler.current_task->state = TASK_STATE_READY;
        scheduler_tick();  // Force reschedule
    }
}

// Sleep for specified number of ticks
void process_sleep(uint64_t ticks) {
    if (g_scheduler.current_task && ticks > 0) {
        g_scheduler.current_task->state = TASK_STATE_BLOCKED;
        // In a full implementation, we'd add to a sleep queue with wake time
        // For now, just yield
        scheduler_tick();
    }
}

// Exit current process
void process_exit(int exit_code) {
    if (g_scheduler.current_task) {
        g_scheduler.current_task->state = TASK_STATE_TERMINATED;
        g_scheduler.current_task->exit_code = exit_code;
        
        early_print("Process ");
        early_print(g_scheduler.current_task->name);
        early_print(" exiting with code ");
        char code_str[16];
        early_print(itoa(exit_code, code_str, 10));
        early_print("\n");
        
        scheduler_tick();  // Force reschedule
    }
}

// Kill process by PID
int process_kill(uint32_t pid) {
    struct task *task = task_find_by_pid(pid);
    if (!task) return -1;
    
    task->state = TASK_STATE_TERMINATED;
    task->exit_code = -1;  // Killed
    
    if (task == g_scheduler.current_task) {
        scheduler_tick();  // Force reschedule if killing current task
    }
    
    return 0;
}

// Find task by PID
struct task *task_find_by_pid(uint32_t pid) {
    for (int i = 0; i < MAX_TASKS; i++) {
        if ((g_task_pool_bitmap & (1 << i)) && g_task_pool[i].pid == pid) {
            return &g_task_pool[i];
        }
    }
    return NULL;
}

// Dump task information
void task_dump_info(struct task *task) {
    if (!task) return;
    
    early_print("Task: ");
    early_print(task->name);
    early_print(" PID=");
    char str[16];
    early_print(itoa(task->pid, str, 10));
    early_print(" State=");
    early_print(itoa(task->state, str, 10));
    early_print(" Priority=");
    early_print(itoa(task->priority, str, 10));
    early_print("\n");
}

// Built-in idle task
void idle_task(void *arg) {
    (void)arg;  // Unused
    
    early_print("Idle task started\n");
    
    while (1) {
        // In a real implementation, this would halt CPU until interrupt
        // For now, just yield frequently
        process_yield();
        
        // Simple delay to avoid overwhelming the system
        for (volatile int i = 0; i < 1000000; i++);
    }
}

// Built-in init task
void init_task(void *arg) {
    (void)arg;  // Unused
    
    early_print("Init task started - system is ready!\n");
    
    // Demonstrate task creation
    process_create(idle_task, NULL, "idle", PRIORITY_IDLE);
    
    // Main init loop
    for (int i = 0; i < 5; i++) {
        early_print("Init task iteration ");
        char str[16];
        early_print(itoa(i + 1, str, 10));
        early_print("\n");
        
        process_sleep(50);  // Sleep for 50 ticks
    }
    
    early_print("Init task completed\n");
    process_exit(0);
}

// Get process statistics
int process_get_stats(struct process_stats *stats) {
    if (!stats) return -1;
    
    stats->total_tasks = 0;
    stats->ready_tasks = 0;
    stats->blocked_tasks = 0;
    
    for (int i = 0; i < MAX_TASKS; i++) {
        if (g_task_pool_bitmap & (1 << i)) {
            stats->total_tasks++;
            switch (g_task_pool[i].state) {
                case TASK_STATE_READY:
                case TASK_STATE_RUNNING:
                    stats->ready_tasks++;
                    break;
                case TASK_STATE_BLOCKED:
                    stats->blocked_tasks++;
                    break;
            }
        }
    }
    
    stats->context_switches = g_scheduler.context_switches;
    stats->scheduler_ticks = g_scheduler.total_ticks;
    stats->current_pid = g_scheduler.current_task ? g_scheduler.current_task->pid : 0;
    
    return 0;
}

// Simple string to integer conversion for early printing
char *itoa(int value, char *str, int base) {
    char *ptr = str, *ptr1 = str, tmp_char;
    int tmp_value;
    
    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmp_value - value * base)];
    } while (value);
    
    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';
    
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
    
    return str;
}
/**
 * Process Scheduler Implementation
 * 
 * Round-robin scheduler with priority support for MiniOS
 */

#include "process.h"
#include "timer.h"
#include "kernel.h"

// Simple string functions (replacing string.h)
static void *memset(void *s, int c, size_t n) {
    unsigned char *p = s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

// External scheduler reference (defined in process.c)
struct scheduler g_scheduler;

// Forward declarations
static void cleanup_terminated_tasks(void);

// Initialize scheduler
void scheduler_init(void) {
    early_print("Initializing scheduler...\n");
    
    memset(&g_scheduler, 0, sizeof(g_scheduler));
    g_scheduler.time_slice_quantum = 10;  // 10 timer ticks
    g_scheduler.next_pid = 1;
    
    early_print("Scheduler initialized\n");
}

// Add task to scheduler
void scheduler_add_task(struct task *task) {
    if (!task) return;
    
    task->next = NULL;
    task->prev = NULL;
    
    if (!g_scheduler.ready_queue) {
        // First task
        g_scheduler.ready_queue = task;
        task->next = task;
        task->prev = task;
    } else {
        // Insert at end of circular list
        struct task *tail = g_scheduler.ready_queue->prev;
        task->next = g_scheduler.ready_queue;
        task->prev = tail;
        tail->next = task;
        g_scheduler.ready_queue->prev = task;
    }
    
    g_scheduler.num_tasks++;
}

// Remove task from scheduler
void scheduler_remove_task(struct task *task) {
    if (!task || !g_scheduler.ready_queue) return;
    
    if (task->next == task) {
        // Only task in queue
        g_scheduler.ready_queue = NULL;
    } else {
        // Remove from circular list
        task->prev->next = task->next;
        task->next->prev = task->prev;
        
        if (g_scheduler.ready_queue == task) {
            g_scheduler.ready_queue = task->next;
        }
    }
    
    task->next = NULL;
    task->prev = NULL;
    g_scheduler.num_tasks--;
}

// Pick next task to run (round-robin with priority)
struct task *scheduler_pick_next_task(void) {
    cleanup_terminated_tasks();
    
    if (!g_scheduler.ready_queue) {
        return NULL;  // No tasks available
    }
    
    struct task *best_task = NULL;
    struct task *current = g_scheduler.ready_queue;
    uint32_t best_priority = PRIORITY_IDLE + 1;  // Lower number = higher priority
    
    do {
        if (current->state == TASK_STATE_READY && current->priority < best_priority) {
            best_task = current;
            best_priority = current->priority;
            
            // If we find highest priority, stop searching
            if (best_priority == PRIORITY_HIGH) {
                break;
            }
        }
        current = current->next;
    } while (current != g_scheduler.ready_queue);
    
    if (best_task) {
        // Move ready queue pointer to next task for fairness
        g_scheduler.ready_queue = best_task->next;
    }
    
    return best_task;
}

// Switch to specified task
void scheduler_switch_to_task(struct task *task) {
    if (!task) return;
    
    struct task *old_task = g_scheduler.current_task;
    
    if (old_task == task) {
        return;  // Already running this task
    }
    
    // Update task states
    if (old_task && old_task->state == TASK_STATE_RUNNING) {
        old_task->state = TASK_STATE_READY;
    }
    
    task->state = TASK_STATE_RUNNING;
    task->time_slice = g_scheduler.time_slice_quantum;
    task->last_scheduled = timer_get_ticks();
    
    g_scheduler.current_task = task;
    g_scheduler.context_switches++;
    
    // Perform context switch if we have a previous task
    if (old_task) {
        context_switch(&old_task->context, &task->context);
    } else {
        // First task - just load the context
        // This is architecture-specific and would be implemented
        // in the architecture-specific context switch code
    }
}

// Scheduler tick (called from timer interrupt)
void scheduler_tick(void) {
    g_scheduler.tick_count++;
    g_scheduler.total_ticks++;
    
    struct task *current = g_scheduler.current_task;
    
    if (current) {
        current->total_runtime++;
        
        // Decrement time slice
        if (current->time_slice > 0) {
            current->time_slice--;
        }
        
        // Check if time slice expired or task is no longer runnable
        if (current->time_slice == 0 || current->state != TASK_STATE_RUNNING) {
            // Need to schedule next task
            struct task *next_task = scheduler_pick_next_task();
            
            if (next_task && next_task != current) {
                scheduler_switch_to_task(next_task);
            } else if (current->state == TASK_STATE_RUNNING) {
                // Reset time slice for current task if it's still the only runnable task
                current->time_slice = g_scheduler.time_slice_quantum;
            }
        }
    } else {
        // No current task - pick one
        struct task *next_task = scheduler_pick_next_task();
        if (next_task) {
            scheduler_switch_to_task(next_task);
        }
    }
}

// Start the scheduler
void scheduler_start(void) {
    early_print("Starting scheduler...\n");
    
    // Pick first task to run
    struct task *first_task = scheduler_pick_next_task();
    if (first_task) {
        early_print("Starting first task: ");
        early_print(first_task->name);
        early_print("\n");
        
        scheduler_switch_to_task(first_task);
    } else {
        early_print("ERROR: No tasks to schedule!\n");
    }
}

// Get current running task
struct task *scheduler_get_current_task(void) {
    return g_scheduler.current_task;
}

// Clean up terminated tasks
static void cleanup_terminated_tasks(void) {
    if (!g_scheduler.ready_queue) return;
    
    struct task *current = g_scheduler.ready_queue;
    struct task *start = current;
    
    do {
        struct task *next = current->next;
        
        if (current->state == TASK_STATE_TERMINATED) {
            early_print("Cleaning up terminated task: ");
            early_print(current->name);
            early_print("\n");
            
            // Remove from scheduler
            scheduler_remove_task(current);
            
            // Free resources
            if (current->stack_base) {
                free_task_stack(current->stack_base, current->stack_size);
            }
            
            // Free task structure (implemented in process.c)
            // free_task(current);  // This would be called here in full implementation
        }
        
        current = next;
    } while (current != start && g_scheduler.ready_queue);
}

// Dump scheduler information
void scheduler_dump_info(void) {
    early_print("=== Scheduler Information ===\n");
    
    char str[16];
    early_print("Total tasks: ");
    early_print(itoa(g_scheduler.num_tasks, str, 10));
    early_print("\n");
    
    early_print("Context switches: ");
    // Note: Using simple conversion for 64-bit values
    early_print(itoa((int)g_scheduler.context_switches, str, 10));
    early_print("\n");
    
    early_print("Scheduler ticks: ");
    early_print(itoa((int)g_scheduler.total_ticks, str, 10));
    early_print("\n");
    
    if (g_scheduler.current_task) {
        early_print("Current task: ");
        early_print(g_scheduler.current_task->name);
        early_print(" (PID ");
        early_print(itoa(g_scheduler.current_task->pid, str, 10));
        early_print(")\n");
    } else {
        early_print("No current task\n");
    }
    
    early_print("=== End Scheduler Info ===\n");
}
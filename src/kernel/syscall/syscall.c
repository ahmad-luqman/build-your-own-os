/**
 * System Call Implementation
 * 
 * Cross-platform system call interface for MiniOS
 */

#include "syscall.h"
#include "process.h"
#include "timer.h"
#include "kernel.h"

// System call handler table
static syscall_handler_t syscall_table[MAX_SYSCALLS];

// System call statistics
static struct syscall_stats g_syscall_stats;

// Tracing enabled flag
static int g_tracing_enabled = 0;

// Initialize system call interface
int syscall_init(void) {
    early_print("Initializing system call interface...\n");
    
    // Clear handler table
    memset(syscall_table, 0, sizeof(syscall_table));
    
    // Clear statistics
    memset(&g_syscall_stats, 0, sizeof(g_syscall_stats));
    
    // Register built-in system calls
    syscall_register(SYSCALL_EXIT, syscall_exit);
    syscall_register(SYSCALL_PRINT, syscall_print);
    syscall_register(SYSCALL_READ, syscall_read);
    syscall_register(SYSCALL_WRITE, syscall_write);
    syscall_register(SYSCALL_GETPID, syscall_getpid);
    syscall_register(SYSCALL_SLEEP, syscall_sleep);
    syscall_register(SYSCALL_YIELD, syscall_yield);
    syscall_register(SYSCALL_GETTIME, syscall_gettime);
    
    early_print("System call interface initialized\n");
    return 0;
}

// Register system call handler
int syscall_register(uint32_t syscall_num, syscall_handler_t handler) {
    if (syscall_num >= MAX_SYSCALLS || !handler) {
        return -1;
    }
    
    syscall_table[syscall_num] = handler;
    return 0;
}

// Unregister system call handler
void syscall_unregister(uint32_t syscall_num) {
    if (syscall_num < MAX_SYSCALLS) {
        syscall_table[syscall_num] = NULL;
    }
}

// System call dispatcher
long syscall_dispatch(uint32_t syscall_num, long arg0, long arg1, long arg2, long arg3, long arg4, long arg5) {
    // Validate system call number
    if (syscall_num >= MAX_SYSCALLS) {
        g_syscall_stats.errors++;
        return SYSCALL_EINVAL;
    }
    
    // Get handler
    syscall_handler_t handler = syscall_table[syscall_num];
    if (!handler) {
        g_syscall_stats.errors++;
        return SYSCALL_ENOENT;
    }
    
    // Update statistics
    g_syscall_stats.total_calls++;
    g_syscall_stats.calls_by_num[syscall_num]++;
    g_syscall_stats.last_call_time = timer_get_ticks();
    
    // Trace if enabled
    if (g_tracing_enabled) {
        struct syscall_context ctx;
        ctx.syscall_num = syscall_num;
        ctx.args[0] = arg0;
        ctx.args[1] = arg1;
        ctx.args[2] = arg2;
        ctx.args[3] = arg3;
        ctx.args[4] = arg4;
        ctx.args[5] = arg5;
        ctx.caller_pid = scheduler_get_current_task() ? scheduler_get_current_task()->pid : 0;
        ctx.timestamp = timer_get_ticks();
        
        syscall_trace(&ctx);
    }
    
    // Call handler
    long result = handler(arg0, arg1, arg2, arg3, arg4, arg5);
    
    return result;
}

// Validate system call parameters
int syscall_validate_params(uint32_t syscall_num, long arg0, long arg1, long arg2, long arg3, long arg4, long arg5) {
    (void)arg1; (void)arg2; (void)arg3; (void)arg4; (void)arg5;  // Unused for now
    
    switch (syscall_num) {
        case SYSCALL_PRINT:
            // Validate string pointer and length
            if (arg0 == 0 || arg1 < 0) {
                return SYSCALL_EINVAL;
            }
            break;
            
        case SYSCALL_READ:
        case SYSCALL_WRITE:
            // Validate file descriptor, buffer, and count
            if (arg0 < 0 || arg1 == 0 || arg2 < 0) {
                return SYSCALL_EINVAL;
            }
            break;
            
        case SYSCALL_SLEEP:
            // Validate sleep duration
            if (arg0 < 0) {
                return SYSCALL_EINVAL;
            }
            break;
    }
    
    return SYSCALL_SUCCESS;
}

// System call tracing
void syscall_trace(struct syscall_context *ctx) {
    if (!ctx) return;
    
    early_print("SYSCALL: ");
    char str[16];
    early_print(itoa(ctx->syscall_num, str, 10));
    early_print(" from PID ");
    early_print(itoa(ctx->caller_pid, str, 10));
    early_print(" args=[");
    early_print(itoa((int)ctx->args[0], str, 10));
    early_print(",");
    early_print(itoa((int)ctx->args[1], str, 10));
    early_print("...]\n");
}

// Built-in system call handlers

// Exit system call
long syscall_exit(long exit_code, long unused1, long unused2, long unused3, long unused4, long unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    
    early_print("SYSCALL: exit(");
    char str[16];
    early_print(itoa((int)exit_code, str, 10));
    early_print(")\n");
    
    process_exit((int)exit_code);
    
    // Should not return
    return SYSCALL_SUCCESS;
}

// Print system call
long syscall_print(long str_ptr, long len, long unused2, long unused3, long unused4, long unused5) {
    (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    
    if (str_ptr == 0 || len <= 0) {
        return SYSCALL_EINVAL;
    }
    
    const char *str = (const char *)str_ptr;
    
    // Simple implementation - just print each character
    for (long i = 0; i < len; i++) {
        if (str[i] == '\0') break;  // Stop at null terminator
        // In a real implementation, we'd use UART driver here
        // For now, use early_print character by character
        char c[2] = {str[i], '\0'};
        early_print(c);
    }
    
    return len;
}

// Read system call (stub)
long syscall_read(long fd, long buf_ptr, long count, long unused3, long unused4, long unused5) {
    (void)fd; (void)buf_ptr; (void)count;
    (void)unused3; (void)unused4; (void)unused5;
    
    early_print("SYSCALL: read() - not implemented\n");
    return SYSCALL_ENOENT;  // Not implemented
}

// Write system call (stub)
long syscall_write(long fd, long buf_ptr, long count, long unused3, long unused4, long unused5) {
    (void)fd; (void)buf_ptr; (void)count;
    (void)unused3; (void)unused4; (void)unused5;
    
    early_print("SYSCALL: write() - not implemented\n");
    return SYSCALL_ENOENT;  // Not implemented
}

// Get process ID system call
long syscall_getpid(long unused0, long unused1, long unused2, long unused3, long unused4, long unused5) {
    (void)unused0; (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    
    struct task *current = scheduler_get_current_task();
    if (current) {
        return (long)current->pid;
    }
    
    return 0;  // Kernel PID
}

// Sleep system call
long syscall_sleep(long ticks, long unused1, long unused2, long unused3, long unused4, long unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    
    if (ticks < 0) {
        return SYSCALL_EINVAL;
    }
    
    early_print("SYSCALL: sleep(");
    char str[16];
    early_print(itoa((int)ticks, str, 10));
    early_print(")\n");
    
    process_sleep((uint64_t)ticks);
    
    return SYSCALL_SUCCESS;
}

// Yield system call
long syscall_yield(long unused0, long unused1, long unused2, long unused3, long unused4, long unused5) {
    (void)unused0; (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    
    early_print("SYSCALL: yield()\n");
    process_yield();
    
    return SYSCALL_SUCCESS;
}

// Get time system call
long syscall_gettime(long time_ptr, long unused1, long unused2, long unused3, long unused4, long unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    
    uint64_t current_time = timer_get_ticks();
    
    if (time_ptr != 0) {
        // Copy time to user buffer (in a real implementation, we'd validate the pointer)
        uint64_t *user_time = (uint64_t *)time_ptr;
        *user_time = current_time;
    }
    
    return (long)current_time;
}

// Get system call statistics
int syscall_get_stats(struct syscall_stats *stats) {
    if (!stats) return -1;
    
    memcpy(stats, &g_syscall_stats, sizeof(struct syscall_stats));
    return 0;
}

// Dump system call statistics
void syscall_dump_stats(void) {
    early_print("=== System Call Statistics ===\n");
    
    char str[16];
    early_print("Total calls: ");
    early_print(itoa((int)g_syscall_stats.total_calls, str, 10));
    early_print("\n");
    
    early_print("Total errors: ");
    early_print(itoa((int)g_syscall_stats.errors, str, 10));
    early_print("\n");
    
    early_print("Last call time: ");
    early_print(itoa((int)g_syscall_stats.last_call_time, str, 10));
    early_print("\n");
    
    // Show call counts for implemented syscalls
    const char *syscall_names[] = {
        "exit", "print", "read", "write", "getpid", "sleep", "yield", "gettime"
    };
    
    for (int i = 0; i < 8; i++) {
        if (g_syscall_stats.calls_by_num[i] > 0) {
            early_print(syscall_names[i]);
            early_print(": ");
            early_print(itoa((int)g_syscall_stats.calls_by_num[i], str, 10));
            early_print("\n");
        }
    }
    
    early_print("=== End Syscall Stats ===\n");
}

// Enable/disable system call tracing
void syscall_enable_tracing(int enable) {
    g_tracing_enabled = enable;
    
    early_print("System call tracing ");
    early_print(enable ? "enabled" : "disabled");
    early_print("\n");
}
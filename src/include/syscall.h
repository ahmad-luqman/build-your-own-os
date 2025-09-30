#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// System call numbers
#define SYSCALL_EXIT        0
#define SYSCALL_PRINT       1
#define SYSCALL_READ        2
#define SYSCALL_WRITE       3
#define SYSCALL_GETPID      4
#define SYSCALL_SLEEP       5
#define SYSCALL_YIELD       6
#define SYSCALL_GETTIME     7
#define SYSCALL_OPEN        8
#define SYSCALL_CLOSE       9
#define SYSCALL_SEEK        10

#define MAX_SYSCALLS        64

// System call error codes
#define SYSCALL_SUCCESS     0
#define SYSCALL_ERROR      -1
#define SYSCALL_EINVAL     -2
#define SYSCALL_ENOENT     -3
#define SYSCALL_EPERM      -4
#define SYSCALL_ENOMEM     -5

// System call handler function type
typedef long (*syscall_handler_t)(long arg0, long arg1, long arg2, long arg3, long arg4, long arg5);

// System call context (for debugging and tracing)
struct syscall_context {
    uint32_t syscall_num;              // System call number
    long args[6];                      // System call arguments
    long result;                       // System call result
    uint32_t caller_pid;               // Calling process PID
    uint64_t timestamp;                // Call timestamp
};

// System call statistics
struct syscall_stats {
    uint64_t total_calls;              // Total system calls
    uint64_t calls_by_num[MAX_SYSCALLS]; // Calls per syscall number
    uint64_t errors;                   // Total errors
    uint64_t last_call_time;           // Last call timestamp
};

// System call initialization and management
int syscall_init(void);
int syscall_register(uint32_t syscall_num, syscall_handler_t handler);
void syscall_unregister(uint32_t syscall_num);

// System call dispatcher (called from architecture-specific entry points)
long syscall_dispatch(uint32_t syscall_num, long arg0, long arg1, long arg2, long arg3, long arg4, long arg5);

// Built-in system call handlers
long syscall_exit(long exit_code, long unused1, long unused2, long unused3, long unused4, long unused5);
long syscall_print(long str_ptr, long len, long unused2, long unused3, long unused4, long unused5);
long syscall_read(long fd, long buf_ptr, long count, long unused3, long unused4, long unused5);
long syscall_write(long fd, long buf_ptr, long count, long unused3, long unused4, long unused5);
long syscall_getpid(long unused0, long unused1, long unused2, long unused3, long unused4, long unused5);
long syscall_sleep(long ticks, long unused1, long unused2, long unused3, long unused4, long unused5);
long syscall_yield(long unused0, long unused1, long unused2, long unused3, long unused4, long unused5);
long syscall_gettime(long time_ptr, long unused1, long unused2, long unused3, long unused4, long unused5);

// Architecture-specific system call entry points
#ifdef __aarch64__
// ARM64 uses SVC (Supervisor Call) instruction
// System call number in x8, arguments in x0-x5, result in x0
void syscall_entry_svc(void);
void syscall_return_to_user(long result);
#elif defined(__x86_64__)
// x86-64 uses SYSCALL instruction
// System call number in rax, arguments in rdi,rsi,rdx,r10,r8,r9, result in rax
void syscall_entry_syscall(void);
void syscall_return_to_user(long result);
#endif

// System call validation and tracing
int syscall_validate_params(uint32_t syscall_num, long arg0, long arg1, long arg2, long arg3, long arg4, long arg5);
void syscall_trace(struct syscall_context *ctx);

// User-space system call wrappers (for testing)
static inline long sys_exit(int exit_code) {
#ifdef __aarch64__
    register long result;
    register long syscall_num = SYSCALL_EXIT;
    register long arg0 = exit_code;
    
    __asm__ volatile("mov x8, %1\n\t"
                     "mov x0, %2\n\t"
                     "svc #0\n\t"
                     "mov %0, x0"
                     : "=r"(result)
                     : "r"(syscall_num), "r"(arg0)
                     : "x8", "x0", "memory");
    return result;
#elif defined(__x86_64__)
    register long result;
    register long syscall_num = SYSCALL_EXIT;
    register long arg0 = exit_code;
    
    __asm__ volatile("syscall"
                     : "=a"(result)
                     : "a"(syscall_num), "D"(arg0)
                     : "rcx", "r11", "memory");
    return result;
#endif
}

static inline long sys_print(const char *str, size_t len) {
#ifdef __aarch64__
    register long result;
    register long syscall_num = SYSCALL_PRINT;
    register long arg0 = (long)str;
    register long arg1 = len;
    
    __asm__ volatile("mov x8, %1\n\t"
                     "mov x0, %2\n\t"
                     "mov x1, %3\n\t"
                     "svc #0\n\t"
                     "mov %0, x0"
                     : "=r"(result)
                     : "r"(syscall_num), "r"(arg0), "r"(arg1)
                     : "x8", "x0", "x1", "memory");
    return result;
#elif defined(__x86_64__)
    register long result;
    register long syscall_num = SYSCALL_PRINT;
    register long arg0 = (long)str;
    register long arg1 = len;
    
    __asm__ volatile("syscall"
                     : "=a"(result)
                     : "a"(syscall_num), "D"(arg0), "S"(arg1)
                     : "rcx", "r11", "memory");
    return result;
#endif
}

static inline long sys_getpid(void) {
#ifdef __aarch64__
    register long result;
    register long syscall_num = SYSCALL_GETPID;
    
    __asm__ volatile("mov x8, %1\n\t"
                     "svc #0\n\t"
                     "mov %0, x0"
                     : "=r"(result)
                     : "r"(syscall_num)
                     : "x8", "x0", "memory");
    return result;
#elif defined(__x86_64__)
    register long result;
    register long syscall_num = SYSCALL_GETPID;
    
    __asm__ volatile("syscall"
                     : "=a"(result)
                     : "a"(syscall_num)
                     : "rcx", "r11", "memory");
    return result;
#endif
}

static inline long sys_yield(void) {
#ifdef __aarch64__
    register long result;
    register long syscall_num = SYSCALL_YIELD;
    
    __asm__ volatile("mov x8, %1\n\t"
                     "svc #0\n\t"
                     "mov %0, x0"
                     : "=r"(result)
                     : "r"(syscall_num)
                     : "x8", "x0", "memory");
    return result;
#elif defined(__x86_64__)
    register long result;
    register long syscall_num = SYSCALL_YIELD;
    
    __asm__ volatile("syscall"
                     : "=a"(result)
                     : "a"(syscall_num)
                     : "rcx", "r11", "memory");
    return result;
#endif
}

// System call statistics and debugging
int syscall_get_stats(struct syscall_stats *stats);
void syscall_dump_stats(void);
void syscall_enable_tracing(int enable);

#ifdef __cplusplus
}
#endif

#endif /* SYSCALL_H */
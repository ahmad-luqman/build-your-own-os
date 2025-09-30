#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>
#include <stddef.h>

// Additional types needed by file system
typedef long ssize_t;
typedef long off_t;

// Memory allocation functions (declared here, implemented in memory.c)
void *kmalloc(size_t size);
void kfree(void *ptr);
int strlen(const char *str);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
char *strcat(char *dest, const char *src);
char *strchr(const char *str, int c);
char *strrchr(const char *str, int c);
void *memset(void *s, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

// Kernel configuration
#define KERNEL_VERSION "0.5.0-dev"
#define KERNEL_NAME "MiniOS"

// Log levels
#define LOG_DEBUG 0
#define LOG_INFO  1
#define LOG_WARN  2
#define LOG_ERROR 3
#define LOG_FATAL 4

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_INFO
#endif

// Architecture detection
#ifdef __aarch64__
#define ARCH_ARM64
#elif defined(__x86_64__)
#define ARCH_X86_64
#else
#error "Unsupported architecture"
#endif

// Include boot protocol
#include "boot_protocol.h"

// Phase 3 includes
#include "memory.h"
#include "exceptions.h"
#include "kernel_loader.h"

// Phase 4 includes
#include "device.h"
#include "driver.h"
#include "timer.h"
#include "uart.h"
#include "interrupt.h"

// Function declarations
void kernel_main(struct boot_info *boot_info);
void kernel_panic(const char *message);
void early_print(const char *str);

// Utility functions
char *itoa(int value, char *str, int base);

// Phase 3 function declarations
void test_memory_allocation(void);

// Phase 4 function declarations (from uart.c)
void uart_test_output(void);
void uart_show_devices(void);

// Phase 4 function declarations (from interrupt.c)
void show_interrupt_stats(void);
void show_interrupt_controllers(void);

// Memory allocation functions (simple implementations for shell)
void *kmalloc(size_t size);
void kfree(void *ptr);

// Architecture-specific functions (implemented in arch/)
void arch_init(void);
void arch_early_print(const char *str);
void arch_halt(void) __attribute__((noreturn));

#endif // KERNEL_H
#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>
#include <stddef.h>

// Kernel configuration
#define KERNEL_VERSION "0.1.0"
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

// Function declarations
void kernel_main(struct boot_info *boot_info);
void kernel_panic(const char *message);
void early_print(const char *str);

// Architecture-specific functions (implemented in arch/)
void arch_init(void);
void arch_early_print(const char *str);
void arch_halt(void) __attribute__((noreturn));

#endif // KERNEL_H
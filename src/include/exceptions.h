/*
 * MiniOS Cross-Platform Exception Handling Interface
 * Phase 3: Memory Management & Kernel Loading
 */

#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <stdint.h>

// Architecture detection
#ifndef ARCH_ARM64
#ifdef __aarch64__
#define ARCH_ARM64
#elif defined(__x86_64__)
#define ARCH_X86_64
#else
#error "Unsupported architecture"
#endif
#endif

// Exception types (cross-platform)
#define EXCEPTION_DIVIDE_BY_ZERO    0x00
#define EXCEPTION_DEBUG             0x01
#define EXCEPTION_NMI               0x02
#define EXCEPTION_BREAKPOINT        0x03
#define EXCEPTION_OVERFLOW          0x04
#define EXCEPTION_BOUND_RANGE       0x05
#define EXCEPTION_INVALID_OPCODE    0x06
#define EXCEPTION_DEVICE_NA         0x07
#define EXCEPTION_DOUBLE_FAULT      0x08
#define EXCEPTION_INVALID_TSS       0x0A
#define EXCEPTION_SEGMENT_NP        0x0B
#define EXCEPTION_STACK_FAULT       0x0C
#define EXCEPTION_GENERAL_PROTECTION 0x0D
#define EXCEPTION_PAGE_FAULT        0x0E
#define EXCEPTION_FPU_ERROR         0x10
#define EXCEPTION_ALIGNMENT_CHECK   0x11
#define EXCEPTION_MACHINE_CHECK     0x12
#define EXCEPTION_SIMD_ERROR        0x13

// ARM64-specific exceptions
#define EXCEPTION_SYNC              0x20
#define EXCEPTION_IRQ               0x21
#define EXCEPTION_FIQ               0x22
#define EXCEPTION_SERROR            0x23

// Exception context structure
struct exception_context {
    // Architecture-specific register state
#ifdef ARCH_ARM64
    uint64_t x[31];          // General purpose registers x0-x30
    uint64_t sp;             // Stack pointer
    uint64_t elr;            // Exception link register (PC)
    uint64_t spsr;           // Saved program status register
    uint64_t esr;            // Exception syndrome register
    uint64_t far;            // Fault address register
#elif defined(ARCH_X86_64)
    uint64_t rax, rbx, rcx, rdx;
    uint64_t rsi, rdi, rbp, rsp;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
    uint64_t rip;            // Instruction pointer
    uint64_t rflags;         // Flags register
    uint64_t cr2;            // Page fault address (if applicable)
    uint64_t error_code;     // Exception error code
#endif
};

// Exception handler function type
typedef void (*exception_handler_t)(uint32_t exception_num, 
                                   struct exception_context *ctx);

// Cross-platform exception handling API

/**
 * Initialize exception handling system
 * @return 0 on success, negative on error
 */
int exception_init(void);

/**
 * Register exception handler
 * @param exception_num Exception number
 * @param handler Handler function
 * @return 0 on success, negative on error
 */
int exception_register_handler(uint32_t exception_num, exception_handler_t handler);

/**
 * Default exception handler (prints info and halts)
 * @param exception_num Exception number
 * @param ctx Exception context
 */
void exception_default_handler(uint32_t exception_num, 
                               struct exception_context *ctx);

/**
 * Page fault handler
 * @param exception_num Exception number (should be EXCEPTION_PAGE_FAULT)
 * @param ctx Exception context
 */
void exception_page_fault_handler(uint32_t exception_num,
                                  struct exception_context *ctx);

/**
 * Print exception information
 * @param exception_num Exception number
 * @param ctx Exception context
 */
void exception_print_info(uint32_t exception_num, struct exception_context *ctx);

// Architecture-specific functions (implemented per architecture)

/**
 * Initialize architecture-specific exception handling
 * @return 0 on success, negative on error
 */
int arch_exception_init(void);

/**
 * Set up exception vectors/tables
 */
void arch_exception_setup_vectors(void);

/**
 * Enable/disable interrupts
 * @param enable True to enable, false to disable
 */
void arch_interrupts_enable(int enable);

/**
 * Check if interrupts are enabled
 * @return True if enabled, false if disabled
 */
int arch_interrupts_enabled(void);

#endif // EXCEPTIONS_H
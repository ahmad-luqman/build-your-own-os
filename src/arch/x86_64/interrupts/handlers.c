/*
 * x86-64 Exception Handlers (Architecture-specific functions only)
 * Phase 3: Memory Management & Kernel Loading
 */

#include <stdint.h>
#include "exceptions.h"
#include "kernel.h"

// Exception handler table
static exception_handler_t exception_handlers[16] = {0};

/**
 * Initialize x86-64 exception handling
 */
int arch_exception_init(void)
{
    // For Phase 3, simplified implementation
    // Real implementation would set up IDT
    
    // Initialize all handlers to default
    for (int i = 0; i < 16; i++) {
        exception_handlers[i] = exception_default_handler;
    }
    
    return 0;
}

/**
 * Register exception handler
 */
int exception_register_handler(uint32_t exception_num, exception_handler_t handler)
{
    if ((exception_num & 0xF) >= 16) {
        return -1;
    }
    
    exception_handlers[exception_num & 0xF] = handler ? handler : exception_default_handler;
    return 0;
}

/**
 * Enable/disable interrupts
 */
void arch_interrupts_enable(int enable)
{
    if (enable) {
        __asm__ volatile ("sti");
    } else {
        __asm__ volatile ("cli");
    }
}

/**
 * Check if interrupts are enabled
 */
int arch_interrupts_enabled(void)
{
    uint64_t flags;
    __asm__ volatile ("pushfq; popq %0" : "=r" (flags));
    return (flags & (1 << 9)) != 0;  // IF flag
}
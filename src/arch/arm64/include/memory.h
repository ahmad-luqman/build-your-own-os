#ifndef ARCH_MEMORY_H
#define ARCH_MEMORY_H

#include <stdint.h>

extern uint64_t __kernel_end;

// Stack is at fixed location 0x40200000 with 64KB size (0x10000)
#define KERNEL_STACK_START   0x40200000
#define KERNEL_STACK_END     (KERNEL_STACK_START + 0x10000)  // 64KB stack

// Stack guard/canary area
#define KERNEL_STACK_GUARD   (KERNEL_STACK_START - 0x10000)  // 64KB guard area

// Exception stack for handling critical operations
#define EXCEPTION_STACK_START  0x40300000
#define EXCEPTION_STACK_END   (EXCEPTION_STACK_START + 0x10000)  // 64KB exception stack

// Heap stays below stack guard, limit to 4MB to be safe
#define KERNEL_HEAP_START ((uint64_t)&__kernel_end)
#define KERNEL_HEAP_END   (KERNEL_STACK_GUARD)                // Stop at guard area

// Runtime stack check
static inline int check_stack_overflow(void)
{
    uint64_t sp;
    __asm__ volatile("mov %0, sp" : "=r"(sp));

    if (sp < KERNEL_STACK_GUARD) {
        // Stack overflow detected!
        return 1;
    }
    return 0;
}

#endif // ARCH_MEMORY_H

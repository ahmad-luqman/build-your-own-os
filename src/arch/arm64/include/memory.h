#ifndef ARCH_MEMORY_H
#define ARCH_MEMORY_H

#include <stdint.h>

extern uint64_t __kernel_end;

#define KERNEL_HEAP_START ((uint64_t)&__kernel_end)
#define KERNEL_HEAP_END (KERNEL_HEAP_START + 0x200000) // 2MB heap

#endif // ARCH_MEMORY_H

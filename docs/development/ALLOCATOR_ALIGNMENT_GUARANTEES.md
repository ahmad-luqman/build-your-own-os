# Allocator Alignment Guarantees in MiniOS

## Overview

This document describes the alignment guarantees provided by MiniOS memory allocators and the critical importance of proper alignment for ARM64 SIMD operations.

## Problem Context

### The SIMD Vectorization Bug
- **Issue**: GCC -O2 optimization generates SIMD instructions (e.g., `ldr q31`, `str q31`) for structure copying
- **Impact**: Misaligned memory access causes stack corruption and crashes
- **Root Cause**: Insufficient alignment guarantees from memory allocators
- **Affected Platform**: ARM64 only (NEON SIMD instructions require 16-byte alignment)

### Critical Failure Example
```c
// Problem: Structure assignment with GCC -O2
struct inode dest = src;  // Generates SIMD if misaligned

// SIMD instructions generated:
// ldr q31, [x0]      // Load 16 bytes - requires 16-byte alignment
// str q31, [x1]      // Store 16 bytes - requires 16-byte alignment
```

## Allocator Implementation

### kmalloc Alignment

#### Location: `src/kernel/memory.c`

```c
#define KMALLOC_ALIGNMENT 16

static uint8_t simple_heap[256 * 1024] __attribute__((aligned(KMALLOC_ALIGNMENT)));

void *kmalloc(size_t size) {
    // Align requested size for NEON operations
    size = (size + (KMALLOC_ALIGNMENT - 1)) & ~(size_t)(KMALLOC_ALIGNMENT - 1);

    // Align heap offset for 16-byte boundary
    size_t aligned_offset = (heap_offset + (KMALLOC_ALIGNMENT - 1)) & ~(size_t)(KMALLOC_ALIGNMENT - 1);

    void *ptr = &simple_heap[aligned_offset];
    heap_offset = aligned_offset + size;

    return ptr;  // Guaranteed 16-byte aligned
}
```

#### Guarantees:
1. **16-byte alignment** for all allocations
2. **Heap base** aligned to 16 bytes
3. **All returned pointers** aligned to 16 bytes
4. **Size rounding** to 16-byte multiples
5. **No fragmentation** due to alignment (bump allocator)

### Page Allocator Alignment

#### Location: `src/arch/arm64/memory/allocator.c`

```c
#define PAGE_SIZE_4K 4096

void *memory_alloc_pages(size_t num_pages) {
    // Returns page-aligned addresses (4KB alignment)
    return (void *)(memory_base + start_page * PAGE_SIZE_4K);
}
```

#### Guarantees:
1. **4KB alignment** for all page allocations
2. **Physical page boundaries** respected
3. **Contiguous pages** for multi-page allocations
4. **Identity mapping** for direct access

## Memory Subsystems and Their Alignment

### 1. Kernel Heap (kmalloc/kfree)
- **Alignment**: 16 bytes (KMALLOC_ALIGNMENT)
- **Purpose**: General kernel allocations
- **Usage**: Structures, buffers, small objects
- **Critical for**: SFS inodes, VFS structures, process contexts

### 2. Page Allocator (memory_alloc_pages)
- **Alignment**: 4KB (PAGE_SIZE_4K)
- **Purpose**: Large memory regions
- **Usage**: Page tables, process memory, large buffers
- **Properties**: Physical page aligned

### 3. Exception Stack
- **Location**: `src/arch/arm64/interrupts/vectors.S`
- **Alignment**: 16 bytes (implicit from page allocation)
- **Purpose**: Exception handling context
- **Critical for**: Save/restore operations

### 4. Kernel Stack
- **Alignment**: 16 bytes (defined in process creation)
- **Purpose**: Process execution context
- **Properties**: Per-process stack

## Why 16-byte Alignment?

### ARM64 ABI Requirements
The ARM64 Procedure Call Standard (AAPCS64) requires:
- **Stack pointer**: 16-byte aligned at function entry
- **NEON loads/stores**: 16-byte aligned addresses
- **SIMD registers**: Operate on 128-bit (16-byte) data

### Compiler Optimizations
```c
// GCC -O2 may transform:
memcpy(dest, src, sizeof(struct inode));

// Into SIMD instructions:
ldr q0, [x0]    // Loads 16 bytes - requires alignment
str q0, [x1]    // Stores 16 bytes - requires alignment
```

### Consequences of Misalignment
1. **Alignment Faults**: CPU generates exceptions
2. **Data Corruption**: Partial reads/writes
3. **Stack Corruption**: Overwrites adjacent data
4. **Security Issues**: Information leakage

## Implementation Details

### Alignment Macro
```c
#define ALIGN_UP(addr, align) (((addr) + (align) - 1) & ~((align) - 1))
#define ALIGN_DOWN(addr, align) ((addr) & ~((align) - 1))
#define IS_ALIGNED(addr, align) (((addr) & ((align) - 1)) == 0)
```

### Heap Initialization
```c
// Static heap with compiler-enforced alignment
static uint8_t simple_heap[256 * 1024] __attribute__((aligned(16)));
```

### Runtime Verification
```c
// Debug check for alignment (in debug builds)
#ifdef DEBUG
void *kmalloc_debug(size_t size, const char *file, int line) {
    void *ptr = kmalloc(size);
    if ((uintptr_t)ptr % 16 != 0) {
        early_print("ALIGNMENT ERROR: ptr not 16-byte aligned!\n");
    }
    return ptr;
}
#define kmalloc(size) kmalloc_debug(size, __FILE__, __LINE__)
#endif
```

## Testing and Validation

### Alignment Test
```c
void test_alignment_guarantees(void) {
    // Test various sizes
    for (size_t i = 1; i <= 128; i++) {
        void *ptr = kmalloc(i);
        assert(((uintptr_t)ptr % 16) == 0);
        assert(IS_ALIGNED(ptr, 16));
        kfree(ptr);
    }

    // Test large allocations
    void *large = kmalloc(4096);
    assert(((uintptr_t)large % 16) == 0);
    kfree(large);
}
```

### SIMD Safety Test
```c
// Verify SIMD operations work on allocated memory
void test_simd_safety(void) {
    struct test_struct {
        uint64_t a, b;  // 16 bytes total
    } *ptr1, *ptr2;

    ptr1 = kmalloc(sizeof(*ptr1));
    ptr2 = kmalloc(sizeof(*ptr2));

    // Initialize
    ptr1->a = 0x1111111111111111;
    ptr1->b = 0x2222222222222222;

    // This should generate SIMD instructions with GCC -O2
    *ptr2 = *ptr1;

    // Verify correctness
    assert(ptr2->a == 0x1111111111111111);
    assert(ptr2->b == 0x2222222222222222);

    kfree(ptr1);
    kfree(ptr2);
}
```

## Best Practices

### For Kernel Developers
1. **Always use kmalloc()** for kernel allocations
2. **Never assume alignment** - verify if critical
3. **Avoid direct memory access** to unaligned addresses
4. **Use structure copying carefully** - may generate SIMD

### For Memory Management
1. **Maintain 16-byte alignment** minimum
2. **Document alignment guarantees** for allocators
3. **Test with optimizations enabled** (-O2)
4. **Validate SIMD operations** work correctly

### For Portability
1. **Check platform requirements** - x86_64 is more lenient
2. **Use standard alignment macros**
3. **Consider future SIMD extensions** (AVX-512 needs 64-byte)

## Related Files

### Core Implementation
- `src/kernel/memory.c` - kmalloc/kfree implementation
- `src/arch/arm64/memory/allocator.c` - Page allocator
- `src/arch/arm64/interrupts/vectors.S` - Exception stack

### Build System
- `tools/build/arch-arm64.mk` - Compiler flags (anti-vectorization)
- `Makefile` - Build configuration

### Documentation
- `docs/development/SIMD_VECTORIZATION_FIX.md` - SIMD issue details
- `docs/development/SFS_CRASH_ANALYSIS.md` - Crash investigation
- `docs/development/SFS_FILE_CREATION_CRASH_HANDOVER.md` - Development status

## Future Considerations

### Potential Improvements
1. **Dynamic alignment** - parameterized alignment
2. **Cache-line alignment** - 64-byte for performance
3. **NUMA awareness** - per-node allocators
4. **Debug instrumentation** - alignment tracking

### Compiler Flags
Currently using anti-vectorization flags as workaround:
```makefile
CFLAGS += -fno-tree-vectorize -fno-slp-vectorize
```

These can be removed once alignment guarantees are fully validated with regression tests.

## Summary

MiniOS provides robust 16-byte alignment guarantees through:
1. **kmalloc()**: Always returns 16-byte aligned pointers
2. **Page allocator**: Provides 4KB aligned pages
3. **Static heaps**: Compiler-enforced alignment
4. **Exception handling**: Properly aligned stacks

This ensures safe SIMD operations and prevents the stack corruption issues that caused the SFS crashes. The alignment fix is critical for system stability on ARM64 platforms.
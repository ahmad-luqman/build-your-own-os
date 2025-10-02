# QUICK REFERENCE: Memory Allocator Fix

## 🎯 THE ISSUE
Exception at PC `0x00000000600003C5` when calling `kmalloc()` during Phase 5 init.

## 🔍 WHERE TO START

### 1. Check Allocator Status (1 min)
```bash
grep -A5 "memory_init complete" src/kernel/main.c
grep -A10 "allocator_init\|allocator_complete" src/arch/arm64/memory/allocator.c
```

### 2. Add Debug Test (2 min)
```c
// In src/kernel/main.c after line 175 (after memory_init)
early_print("=== Testing allocator ===\n");
void *test = memory_alloc(4096, MEMORY_ALIGN_4K);
early_print(test ? "✓ Works\n" : "✗ FAILS\n");
if (test) memory_free(test);
```

### 3. Build & Test (1 min)
```bash
make clean && make ARCH=arm64
qemu-system-aarch64 -machine virt -cpu cortex-a72 -m 512M \
  -kernel build/arm64/kernel.elf -nographic -serial mon:stdio
```

## 🔧 LIKELY FIXES (Try in order)

### Fix #1: Add Ready Flag (EASIEST - 5 min)
```c
// In src/arch/arm64/memory/allocator.c
static int allocator_ready = 0;

// At end of allocator_init():
allocator_ready = 1;

// Add function:
int memory_allocator_is_ready(void) {
    return allocator_ready;
}

// In src/kernel/main.c before Phase 5:
if (!memory_allocator_is_ready()) {
    kernel_panic("Allocator not ready for Phase 5");
}
```

### Fix #2: Move Phase 5 Later (5 min)
```c
// In src/kernel/main.c
// Cut Phase 5 block (lines 280-320)
// Paste after Phase 4 device_list_all() completes
// Add: early_print("Waiting for allocator stability...\n");
```

### Fix #3: BSS Initialization (10 min)
```c
// Check if BSS is cleared properly in bootloader
// src/arch/arm64/boot.S or src/arch/arm64/kernel_start.S
// Look for BSS clearing code
```

## 📁 FILES TO TOUCH

### Must Edit
1. `src/kernel/main.c` (line 280-320) - Re-enable Phase 5 features
2. `src/arch/arm64/memory/allocator.c` - Add ready flag or fix init

### Must Revert After Fix
1. `src/fs/ramfs/ramfs_core.c` (line 258-290) - Remove static workaround
2. `src/kernel/main.c` (line 300-320) - Uncomment fd_init()

## ✅ SUCCESS TEST

```bash
# After fix, this should work:
/MiniOS> mkdir test
Directory created: /test

/MiniOS> cd test
/MiniOS/test> echo "it works!" > success.txt
/MiniOS/test> cat success.txt
it works!

✅ If you see this, you're done!
```

## 🆘 IF STUCK

### Still Getting Exception?
1. Check if allocator bitmap is initialized: `grep bitmap_done src/arch/arm64/memory/allocator.c`
2. Verify memory regions: `grep "Region 0:" output` should show valid address
3. Try smaller allocation first: `memory_alloc(16, MEMORY_ALIGN_4K)`

### Different Exception?
- New PC value? Look it up: `addr2line -e build/arm64/kernel.elf 0xXXXXXXXX`
- Null pointer? Check if allocator returned NULL
- Data abort? Check alignment requirements

### Need More Info?
Read full handover: `cat HANDOVER_FIX_MEMORY_ALLOCATOR.md`

## 📊 EXPECTED TIME
- **Quick fix**: 10-15 minutes (if it's just a ready flag)
- **Medium fix**: 30-45 minutes (if ordering issue)
- **Complex fix**: 1-2 hours (if allocator redesign needed)

Most likely it's the quick fix! 🎯

## 💾 BACKUP PLAN
If all else fails, keep static allocation and document limitation:
```c
// Phase 5 works but limited to static structures
// TODO: Fix allocator timing for dynamic FS operations
```
Shell will still work, just with limited file operations.

---
**TIP**: The PC value `0x00000000600003C5` suggests function pointer issue.
Check if memory_alloc function pointer is initialized correctly.

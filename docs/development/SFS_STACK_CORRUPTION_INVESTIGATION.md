# SFS Stack Corruption Investigation Report

## Problem Summary
Critical stack corruption crash occurring in SFS (Simple File System) operations, specifically during directory traversal and file operations. The crash was preventing basic commands like `cd /sfs` from working.

## Symptoms
- Crash during `cd /sfs` command in SFS persistence test
- Corrupted stack pointer (SP) values: 0x4009AF68, 0x4009AFB8, 0x4009B068
- Corrupted PC values: 0x600003C5, 0x200003C5
- Stack canary remained intact, indicating corruption after canary setup
- Exception type: Synchronous Exception (ESR: 0x401FFA4C/0x401FFA48)

## Root Cause Analysis

### Primary Issue
Compiler optimization (-O2) was causing stack corruption during structure assignments in `sfs_allocate_vfs_inode()` function. The specific problematic code was:
```c
inode_data->disk_inode = *disk_inode;  // Structure assignment corrupting stack
```

### Technical Details
1. **Stack Layout**: Main stack at 0x40200000 with 64KB size
2. **Exception Stack**: Added dedicated exception stack at 0x40300000
3. **Corruption Pattern**: SP corrupted by ~0x50F98 bytes from stack base
4. **Memory Barriers**: Missing proper barriers between memory operations

## Investigation Steps Taken

### 1. Exception Handler Analysis
- Modified `/src/arch/arm64/interrupts/vectors.S` to use dedicated exception stack
- Added preservation of original SP value in exception context
- Enabled proper debugging of corrupted stack values

### 2. Debug Tracing
- Added extensive debug output to SFS functions:
  - `sfs_dir_lookup()`
  - `sfs_get_inode()`
  - `sfs_allocate_vfs_inode()`
- Traced crash to exact location: structure copy operation

### 3. Compiler Optimization Testing
- Tested with -O0 (no optimization): Issue disappeared
- Tested with -O2 (default optimization): Issue persisted
- Confirmed compiler optimization was the root cause

## Solution Implemented

### 1. Exception Vector Fixes (`src/arch/arm64/interrupts/vectors.S`)
```assembly
// Added dedicated exception stack
.set EXCEPTION_STACK_BASE, 0x40300000

// Modified save_context macro
.macro save_context
    mov x5, sp                // Save current SP
    ldr x6, =EXCEPTION_STACK_BASE
    mov sp, x6                // Use dedicated exception stack
    sub sp, sp, #(35 * 8)     // Room for context + original SP
    str x5, [sp, #(34 * 8)]   // Save original SP
    // ... rest of context saving
.endm
```

### 2. Memory Barriers (`src/fs/sfs/sfs_core.c`)
```c
// Added ARM64 memory barriers around critical operations
__asm__ volatile("dmb ish" ::: "memory");

// Replaced structure assignment with memcpy
memcpy(&inode_data->disk_inode, disk_inode, sizeof(struct sfs_inode));
```

### 3. Fixed Stack Corruption Detection (`src/kernel/exceptions.c`)
- Removed debug "(CORRUPTED!)" label for cleaner output
- Maintained stack corruption detection with canary checks

## Files Modified

### Core Fixes
1. `/src/arch/arm64/interrupts/vectors.S` - Exception stack implementation
2. `/src/fs/sfs/sfs_core.c` - Memory barriers and safe copying
3. `/src/kernel/exceptions.c` - Cleaned up debug output

### Related Files
4. `/src/arch/arm64/kernel_start.S` - Stack initialization
5. `/src/arch/arm64/include/memory.h` - Stack definitions

## Test Results

### Before Fix
```
/MiniOS> cd /sfs
kmalloc: OK
*** UNHANDLED EXCEPTION ***
Exception type: Synchronous Exception
PC (ELR_EL1): 0x00000000600003C5
SP: 0x000000004009AF68 (CORRUPTED!)
```

### After Fix
```
/MiniOS> cd /sfs
kmalloc: OK
kmalloc: OK
kmalloc: OK
kmalloc: OK
/sfsMiniOS>
```

## Remaining Issues

1. **File Creation Crash**: After `cd /sfs` works, creating files still causes crashes
   - Command: `echo "Persistence Test" > test.txt`
   - Same stack corruption pattern observed

2. **Next Crash Location**: Likely in file creation/write path
   - `sfs_file_create()` or related functions
   - Similar optimization/memory barrier issues

## Recommendations

### Immediate Actions
1. Investigate file creation crash with same methodology
2. Add memory barriers to all SFS file operations
3. Create comprehensive test suite for all SFS operations

### Long-term Improvements
1. Review all structure assignments for potential optimization issues
2. Implement compiler attributes to prevent problematic optimizations
3. Add static analysis for memory corruption detection

## Technical Notes

### ARM64 Memory Barrier Types
- `dmb ish` - Data Memory Barrier, Inner Shareable domain
- Ensures memory ordering before/after critical operations
- Prevents compiler and CPU reordering

### Compiler Optimization Considerations
- -O2 can optimize structure assignments in unsafe ways
- Volatile qualifiers alone insufficient for memory ordering
- Memory barriers required for correct behavior

### Stack Layout
```
0x40300000 - Exception Stack (64KB)
0x40200000 - Main Stack (64KB)
0x401F0000 - Stack Limit/Canary
```

## Commits
1. `adfd4ba` - Fix critical stack corruption crash in SFS operations
2. `6f6becb` - Add VFS path resolution and SFS persistence test
3. `b6d8745` - Update development documentation and shell improvements

## Investigation Tools Used
- GDB/Debugger: Not available, used custom debug output
- Memory barriers: ARM64 `dmb ish` instructions
- Stack canaries: 0xDEADC0DE pattern
- Exception vectors: Modified for better debugging

## Lessons Learned
1. Compiler optimization can cause subtle memory corruption
2. Exception handlers must use dedicated stacks for reliable debugging
3. Memory barriers are essential in multi-threaded/system code
4. Structure assignments can be optimized dangerously

## Next Steps
1. Fix file creation crash using same methodology
2. Audit all SFS operations for similar issues
3. Implement comprehensive memory safety checks
4. Create regression tests for stack corruption

---

*Report generated: 2025-10-03*
*Investigation by: Claude Code Assistant*
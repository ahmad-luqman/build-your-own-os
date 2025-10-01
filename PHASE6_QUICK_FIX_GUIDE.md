# Phase 6 Shell - Quick Fix Guide

**Problem**: Unhandled exception when accessing global shell_context  
**Status**: Ready for debugging  
**Priority**: HIGH - Blocking Phase 6 completion

---

## 🚀 Quick Start - Copy/Paste Commands

### Option 1: Check BSS Initialization (FASTEST)

```bash
cd /Users/ahmadluqman/src/build-your-own-os

# Check if BSS is being cleared in boot code
echo "=== Checking ARM64 boot.S ==="
grep -n "bss" src/arch/arm64/boot.S

echo "=== Checking kernel_start.S ==="
grep -n "bss" src/arch/arm64/kernel_start.S

echo "=== Checking linker script ==="
grep -n "bss\|\.bss" src/arch/arm64/kernel.ld

# If no BSS clearing found, that's the problem!
```

### Option 2: Quick Test with Small Structure

Add this to `src/shell/core/shell_core.c`:

```c
// Replace line with global_shell_context
// OLD: static struct shell_context global_shell_context = {0};
// NEW:
static struct {
    char current_directory[256];
    char command_buffer[1024];
    int exit_requested;
    int stdin_fd, stdout_fd, stderr_fd;
} minimal_shell_context = {0};

// Then modify shell_main_task to use minimal_shell_context
// Cast it to shell_context* for now (unsafe but for testing)
```

### Option 3: Use Heap Allocation

Replace in `src/shell/core/shell_core.c`:

```c
// Remove global declaration
// static struct shell_context global_shell_context = {0};

// In shell_main_task():
void shell_main_task(void *arg)
{
    (void)arg;
    
    early_print("Allocating shell context from heap...\n");
    struct shell_context *ctx = (struct shell_context *)kmalloc(sizeof(struct shell_context));
    
    if (!ctx) {
        early_print("Failed to allocate shell context\n");
        return;
    }
    
    early_print("Shell context allocated successfully\n");
    
    // Initialize to zero manually
    for (size_t i = 0; i < sizeof(struct shell_context); i++) {
        ((char*)ctx)[i] = 0;
    }
    
    // Rest of existing code...
    
    kfree(ctx);  // At the end
}
```

---

## 🔍 Detailed Diagnostic Steps

### Step 1: Verify BSS Symbols Exist

```bash
cd /Users/ahmadluqman/src/build-your-own-os
aarch64-elf-nm build/arm64/kernel.elf | grep bss
```

**Expected output:**
```
00000000XXXXXXXX B __bss_end
00000000XXXXXXXX B __bss_start
```

If missing, linker script needs fixing.

### Step 2: Check Global Context Address

```bash
aarch64-elf-nm build/arm64/kernel.elf | grep global_shell_context
```

**Check:**
- Is it in BSS section (symbol type 'B')?
- Is the address reasonable?
- Is it within kernel memory range?

### Step 3: Examine Boot Code

```bash
# Look for BSS clearing loop
aarch64-elf-objdump -d build/arm64/kernel.elf | grep -A 30 "_start:" | head -50
```

**Look for pattern like:**
```asm
ldr x0, =__bss_start
ldr x1, =__bss_end
# Loop to clear memory
```

### Step 4: Check Memory Mapping

Add to `src/kernel/main.c` before shell initialization:

```c
early_print("=== Memory Diagnostic ===\n");
extern char __bss_start[], __bss_end[];
early_print("BSS start: 0x");
print_hex((uint64_t)__bss_start);
early_print("\nBSS end: 0x");
print_hex((uint64_t)__bss_end);
early_print("\nBSS size: ");
print_decimal((uint64_t)(__bss_end - __bss_start));
early_print(" bytes\n");
```

---

## 🛠️ Fix Templates

### Fix 1: Add BSS Clearing to boot.S

If BSS clearing is missing, add to `src/arch/arm64/boot.S` or `kernel_start.S`:

```asm
// Clear BSS section
clear_bss:
    // Load BSS range
    ldr x0, =__bss_start
    ldr x1, =__bss_end
    
    // Check if already clear (debug)
    cmp x0, x1
    b.eq bss_done
    
clear_bss_loop:
    // Write zero and advance
    str xzr, [x0], #8
    cmp x0, x1
    b.lo clear_bss_loop
    
bss_done:
    // Continue to kernel_main
```

### Fix 2: Reduce Structure Size

In `src/include/shell.h`:

```c
// Change this:
#define SHELL_HISTORY_SIZE  16  // Was 16 * 1024 bytes = 16KB

// To this:
#define SHELL_HISTORY_SIZE  4   // Only 4 * 1024 bytes = 4KB
```

Then rebuild:
```bash
make clean && make ARCH=arm64 all
```

### Fix 3: Split Structure

Create new file `src/include/shell_types.h`:

```c
#ifndef SHELL_TYPES_H
#define SHELL_TYPES_H

// Core context (small, can be stack or global)
struct shell_context_core {
    char current_directory[256];
    char command_buffer[1024];
    int argc;
    int exit_requested;
    int stdin_fd, stdout_fd, stderr_fd;
    struct fd_table *fd_table;
};

// History context (large, allocated separately)
struct shell_history {
    char command_history[16][1024];
    int history_count;
    int history_index;
};

#endif
```

---

## ✅ Verification Tests

After applying any fix, run these tests:

### Test 1: Basic Boot
```bash
cd /Users/ahmadluqman/src/build-your-own-os
timeout 8 qemu-system-aarch64 -machine virt -cpu cortex-a72 -m 512M \
    -kernel build/arm64/kernel.elf -nographic -serial mon:stdio 2>&1 | \
    grep -E "(shell_main_task|Shell|MiniOS>|EXCEPTION)"
```

**Success**: Should see "shell_main_task" messages without "EXCEPTION"

### Test 2: Shell Prompt
```bash
# Run and send command
(sleep 2; echo "help"; sleep 2; echo "exit"; sleep 1) | \
timeout 10 qemu-system-aarch64 -machine virt -cpu cortex-a72 -m 512M \
    -kernel build/arm64/kernel.elf -nographic -serial mon:stdio 2>&1 | \
    tail -30
```

**Success**: Should see:
- "MiniOS Shell v1.0"
- "MiniOS>" prompt  
- "Available commands" list
- No exceptions

### Test 3: Command Execution
```bash
./test_phase6_incremental.sh 2>&1 | grep -E "(PASSED|FAILED|Test)"
```

**Success**: All tests should PASS

---

## 📊 Success Indicators

| Indicator | Before Fix | After Fix |
|-----------|------------|-----------|
| Boot to shell | ❌ Exception | ✅ Prompt appears |
| Memory access | ❌ Crashes | ✅ Works |
| Commands | ❌ Not reached | ✅ Execute |
| Test suite | ❌ 0/7 pass | ✅ 7/7 pass |

---

## 🆘 If Still Failing

### Debug Output Template

Add to `shell_main_task()`:

```c
void shell_main_task(void *arg)
{
    (void)arg;
    
    // Checkpoint 1
    early_print("[1] Entered shell_main_task\n");
    
    // Checkpoint 2  
    early_print("[2] About to get context address\n");
    struct shell_context *ctx = &global_shell_context;
    
    // Checkpoint 3
    early_print("[3] Got context at: 0x");
    print_hex((uint64_t)ctx);
    early_print("\n");
    
    // Checkpoint 4
    early_print("[4] About to read from context\n");
    volatile int test_read = ctx->exit_requested;
    
    // Checkpoint 5
    early_print("[5] Read successful, value: ");
    print_decimal(test_read);
    early_print("\n");
    
    // Checkpoint 6
    early_print("[6] About to write to context\n");
    ctx->exit_requested = 0;
    
    // Checkpoint 7
    early_print("[7] Write successful\n");
    
    // Continue...
}
```

**Note which checkpoint fails** - this tells us exactly where the problem is.

### Common Checkpoint Failures

- **Fails at [2]**: Function entry issue (unlikely)
- **Fails at [3]**: Symbol resolution issue
- **Fails at [4]**: Address calculation issue
- **Fails at [5]**: Memory read fault (unmapped/invalid address)
- **Fails at [6]**: Still reading
- **Fails at [7]**: Memory write fault (read-only or protected memory)

---

## 📞 Emergency Workaround

If nothing works, use this minimal shell (no global context):

```c
// In shell_core.c
void shell_main_task_minimal(void *arg)
{
    (void)arg;
    
    shell_print("MiniOS Shell v1.0 (minimal mode)\n");
    shell_print("Type 'help' for commands, 'exit' to quit\n\n");
    
    char cmd_buf[256];
    
    while (1) {
        shell_print("MiniOS> ");
        
        // Simple read
        int pos = 0;
        while (pos < 255) {
            int ch = shell_getc();
            if (ch < 0) continue;
            
            if (ch == '\n' || ch == '\r') {
                cmd_buf[pos] = '\0';
                shell_putc('\n');
                break;
            }
            
            cmd_buf[pos++] = ch;
            shell_putc(ch);
        }
        
        // Simple command handling
        if (strcmp(cmd_buf, "help") == 0) {
            shell_print("Available commands: help, exit\n");
        } else if (strcmp(cmd_buf, "exit") == 0) {
            break;
        } else if (cmd_buf[0] != '\0') {
            shell_print("Unknown command\n");
        }
    }
    
    shell_print("Shell exited\n");
}
```

This proves the I/O works and isolates the problem to the context structure.

---

## 📋 Checklist

Before asking for help, verify:

- [ ] Checked BSS symbols exist (`aarch64-elf-nm`)
- [ ] Checked BSS clearing code exists in boot.S
- [ ] Tried smaller structure size
- [ ] Tried heap allocation
- [ ] Added debug checkpoints
- [ ] Verified boot.elf is valid
- [ ] Tried minimal shell workaround
- [ ] Documented which checkpoint fails
- [ ] Saved error messages and PC address

---

*This guide is designed for quick copy-paste fixes. Choose the option that seems most likely and try it!*

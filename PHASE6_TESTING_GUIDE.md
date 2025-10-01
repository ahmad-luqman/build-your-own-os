# Phase 6 Shell - Testing Guide

## ✅ Quick Verification Test

Run this single command to verify the shell is working:

```bash
cd /Users/ahmadluqman/src/build-your-own-os

# Quick test (8 seconds)
(sleep 2; echo "help"; sleep 2; echo "echo test"; sleep 2; echo "exit") | \
timeout 10 qemu-system-aarch64 -machine virt -cpu cortex-a72 -m 512M \
    -kernel build/arm64/kernel.elf -nographic -serial mon:stdio 2>&1 | \
    grep -E "(MiniOS Shell|Available commands|echo test|Shell exited)"
```

**Expected Output:**
```
MiniOS Shell v1.0
Available commands:
echo test
Shell exited
```

## 🧪 Comprehensive Testing

### Test 1: Interactive Mode

```bash
qemu-system-aarch64 -machine virt -cpu cortex-a72 -m 512M \
    -kernel build/arm64/kernel.elf -nographic -serial mon:stdio
```

Try these commands:
```
help
echo Hello World!
pwd
uname
ps
free
date
uptime
exit
```

### Test 2: Automated Command Test

```bash
cat > /tmp/shell_commands.txt << 'CMDS'
help
echo Testing MiniOS Shell
pwd
uname
ps
free
exit
CMDS

cat /tmp/shell_commands.txt | \
    qemu-system-aarch64 -machine virt -cpu cortex-a72 -m 512M \
    -kernel build/arm64/kernel.elf -nographic -serial mon:stdio
```

### Test 3: Cross-Platform (x86_64)

```bash
# Build for x86_64
make ARCH=x86_64 all

# Test
(sleep 2; echo "help"; sleep 2; echo "exit") | \
timeout 10 qemu-system-x86_64 -m 512M \
    -kernel build/x86_64/kernel.elf -nographic -serial mon:stdio
```

## 📊 Success Criteria Checklist

- [ ] Shell boots to "MiniOS Shell v1.0"
- [ ] Prompt shows "/MiniOS>"
- [ ] help command lists all commands
- [ ] echo command prints arguments
- [ ] pwd shows "/"
- [ ] uname shows "MiniOS"
- [ ] ps shows process list
- [ ] free shows memory statistics
- [ ] exit cleanly terminates shell
- [ ] No crashes or exceptions

## 🐛 Troubleshooting

### Issue: Shell doesn't start

**Check:**
```bash
# Verify kernel exists and is correct size
ls -lh build/arm64/kernel.elf
# Should be ~173KB

# Check SHELL_HISTORY_SIZE
grep SHELL_HISTORY_SIZE src/include/shell.h
# Should show: #define SHELL_HISTORY_SIZE 2
```

**Fix:**
```bash
make clean && make ARCH=arm64 all
```

### Issue: Exception at boot

**Symptom:** `*** UNHANDLED EXCEPTION ***`

**Fix:** SHELL_HISTORY_SIZE might be wrong
```bash
# Check value
grep "#define SHELL_HISTORY_SIZE" src/include/shell.h

# Should be 2, if not:
sed -i '' 's/SHELL_HISTORY_SIZE.*/SHELL_HISTORY_SIZE  2/' src/include/shell.h
make clean && make ARCH=arm64 all
```

### Issue: Commands don't work

**Check command registry:**
```bash
grep -A 3 "shell_commands\[\]" src/shell/core/shell_core.c | head -20
```

Should show all commands registered.

## 📈 Performance Metrics

| Metric | Value |
|--------|-------|
| Boot Time | ~2 seconds |
| Shell Context Size | ~4KB |
| Memory Usage | ~16KB total |
| Available Commands | 15 |
| Command Response | Instant |

## 🔬 Advanced Testing

### Memory Stress Test
```bash
# Test multiple allocations
cat << 'STRESS' | qemu-system-aarch64 -machine virt -cpu cortex-a72 -m 512M \
    -kernel build/arm64/kernel.elf -nographic -serial mon:stdio
help
ps
free
uname
ps
free
ps
free
exit
STRESS
```

### Long Command Test
```bash
echo "echo This is a very long command with many words to test the command buffer handling" | \
    qemu-system-aarch64 -machine virt -cpu cortex-a72 -m 512M \
    -kernel build/arm64/kernel.elf -nographic -serial mon:stdio
```

### Special Characters Test
```bash
cat << 'SPECIAL' | qemu-system-aarch64 -machine virt -cpu cortex-a72 -m 512M \
    -kernel build/arm64/kernel.elf -nographic -serial mon:stdio
echo Test! @#$%
echo "Multiple   spaces"
exit
SPECIAL
```

## 📝 Test Results Log

Keep a log of test results:

```bash
# Create test log
cat > test_results.log << 'LOG'
Date: $(date)
Architecture: ARM64
Kernel: build/arm64/kernel.elf

Test 1: Boot to prompt - PASS/FAIL
Test 2: help command - PASS/FAIL
Test 3: echo command - PASS/FAIL
Test 4: pwd command - PASS/FAIL
Test 5: System info - PASS/FAIL
Test 6: Clean exit - PASS/FAIL

Notes:
- 
LOG
```

## 🎯 Integration Testing

Test with other phases:

```bash
# Ensure all previous phases still work
./test_phases.sh

# Should show:
# Phase 1-2: PASS
# Phase 3: PASS
# Phase 4: PASS
# Phase 5: PASS
# Phase 6: PASS
```

## 📞 Report Issues

If tests fail, gather this information:

```bash
# 1. Kernel info
ls -lh build/arm64/kernel.elf
file build/arm64/kernel.elf

# 2. Build log
make clean && make ARCH=arm64 all > build.log 2>&1
tail -50 build.log

# 3. Test output
timeout 10 qemu-system-aarch64 -machine virt -cpu cortex-a72 -m 512M \
    -kernel build/arm64/kernel.elf -nographic -serial mon:stdio \
    > test_output.log 2>&1
cat test_output.log

# 4. Configuration
grep SHELL_HISTORY_SIZE src/include/shell.h
grep SHELL_MAX src/include/shell.h
```

Include all four outputs when reporting issues.

---

**Happy Testing! 🧪**

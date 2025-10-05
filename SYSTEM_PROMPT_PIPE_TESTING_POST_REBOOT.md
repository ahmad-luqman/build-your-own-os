# MiniOS Pipe Testing - Post-Reboot System Prompt

## 🎯 IMMEDIATE POST-REBOOT TASKS

### **1. Verify QEMU Functionality (First 5 minutes)**
```bash
cd /Users/ahmadluqman/src/build-your-own-os

# Test basic QEMU operation
echo "help\nexit" | timeout 30s qemu-system-aarch64 -machine virt -cpu cortex-a72 -m 512M -nographic -kernel build/arm64/kernel.elf -serial mon:stdio
```

**If QEMU still fails with SIGTERM:**
- Try alternative QEMU flags: `-serial stdio` instead of `-serial mon:stdio`
- Check for security software interfering
- Try running from different terminal

**If QEMU works:** Proceed to step 2

---

## 🧪 PIPE TESTING CHECKLIST

### **2. Test RAMFS Pipes (Should Work)**
```bash
# Quick RAMFS pipe test
echo -e "echo 'RAMFS test' | cat\nls | cat\nexit" | timeout 45s qemu-system-aarch64 -machine virt -cpu cortex-a72 -m 512M -nographic -kernel build/arm64/kernel.elf -serial mon:stdio
```

**Expected Output:**
- "RAMFS test" should appear
- Directory listing should show files
- No crashes or errors

### **3. Test SFS Pipes (Main Goal)**
```bash
# Run comprehensive SFS pipe test
./scripts/testing/test_pipes_sfs_simple.sh
```

**Or run manually:**
```bash
cat > /tmp/sfs_test.txt << 'EOF'
help
echo "=== SFS Pipe Testing ==="
echo "RAMFS pipe:"
echo "hello RAMFS" | cat

echo "Setting up SFS:"
mkdir /sfs
mkfs ramdisk0
mount ramdisk0 /sfs sfs

echo "SFS pipe:"
cd /sfs
echo "hello SFS" | cat
ls /sfs | cat

echo "Cross-FS pipe:"
echo "cross-FS content" | cat

echo "Cleanup:"
cd /
umount /sfs
exit
EOF

timeout 60s qemu-system-aarch64 -machine virt -cpu cortex-a72 -m 512M -nographic -kernel build/arm64/kernel.elf -serial mon:stdio < /tmp/sfs_test.txt
```

---

## 📁 KEY FILES TO REFERENCE

### **Core Implementation**
- **`src/shell/parser/parser.c`** (lines 11-46, 151-207)
  - `build_pipe_temp_path()` - Creates temp files
  - `execute_command()` - Pipe execution logic
  - Pipe detection and command splitting

### **Test Scripts Created**
- **`scripts/testing/test_pipes_sfs_simple.sh`** - Simple SFS pipe test
- **`scripts/testing/test_pipes_sfs_ramfs.sh`** - Comprehensive RAMFS+SFS test
- **`scripts/testing/test_pipe_functionality.sh`** - Basic pipe test

### **Documentation**
- **`docs/development/TODO.md`** - Updated with pipe status
- **`docs/development/SFS_NEXT_STEPS.md`** - Phase 6 completion status
- **`NEXT_SESSION_PROMPT.md`** - Current implementation status

---

## 🎯 SUCCESS CRITERIA

### **RAMFS Pipes ✅**
- [ ] `echo "text" | cat` displays "text"
- [ ] `ls | cat` shows directory listing
- [ ] `pwd | cat` shows current directory
- [ ] No temp files left in `/tmp`

### **SFS Pipes 🎯**
- [ ] SFS mounts successfully: `mount ramdisk0 /sfs sfs`
- [ ] `cd /sfs && echo "test" | cat` works
- [ ] `ls /sfs | cat` shows SFS directory
- [ ] Cross-FS operations work
- [ ] SFS unmounts cleanly

### **Expected Pipe Behavior**
1. **Temp Files**: Created in `/tmp/pipe_0`, `/tmp/pipe_1`, etc.
2. **Cleanup**: Temp files automatically deleted after pipe
3. **Error Handling**: Invalid syntax shows helpful messages
4. **Performance**: Fast execution with no hangs

---

## 🔧 DEBUGGING IF TESTS FAIL

### **If QEMU Still Terminates:**
```bash
# Check what's killing QEMU
sudo dmesg | tail -20  # Look for QEMU-related messages
ps aux | grep qemu     # Check for hanging processes

# Try different QEMU approach
qemu-system-aarch64 -M virt -cpu cortex-a72 -m 512M -kernel build/arm64/kernel.elf -nographic -serial stdio
```

### **If Pipes Don't Work:**
1. **Check temp file creation**: Look for `/tmp/pipe_*` messages in output
2. **Check parser output**: Should show pipe detection
3. **Check I/O redirection**: Verify temp files are created/read correctly
4. **Check cleanup**: `vfs_unlink()` should remove temp files

### **Common Issues & Solutions:**
- **SFS mount fails**: Check if ramdisk0 is properly initialized
- **Temp file issues**: Verify RAMFS has space in `/tmp`
- **Command parsing**: Check pipe syntax `cmd1 | cmd2` format

---

## 📊 TESTING MATRIX TO VERIFY

| Test Command | Expected Result | FS Location |
|--------------|-----------------|-------------|
| `echo "hello" | cat` | Shows "hello" | RAMFS |
| `ls / | cat` | Directory listing | RAMFS |
| `cd /sfs && pwd | cat` | Shows "/sfs" | SFS |
| `cd /sfs && echo "test" | cat` | Shows "test" | SFS |
| `ls /sfs | cat` | SFS contents | Cross-FS |
| `echo "test" > file.txt && cat file.txt | cat` | Shows "test" | SFS |

---

## 🚀 NEXT STEPS AFTER SUCCESS

1. **Document Results**: Update TODO.md with confirmed SFS pipe functionality
2. **Multiple Pipes**: Test `cmd1 | cmd2 | cmd3` (not implemented yet)
3. **Performance**: Test with large data through pipes
4. **Edge Cases**: Test error handling, empty pipes, etc.

---

## 📞 QUICK REFERENCE COMMANDS

```bash
# Build MiniOS
make clean && make all ARCH=arm64

# Quick pipe test
echo "hello world" | cat | ./tools/test-vm.sh arm64 45

# SFS comprehensive test
./scripts/testing/test_pipes_sfs_simple.sh

# Check QEMU processes
ps aux | grep qemu

# Kill hanging QEMU
pkill qemu-system-aarch64
```

---

**Remember**: The pipe implementation uses temporary files in `/tmp/pipe_<n>` and should work on both RAMFS and SFS. The testing will confirm the real-world functionality.
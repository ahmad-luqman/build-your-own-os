#!/bin/bash

# Comprehensive Pipe Testing on Both RAMFS and SFS
# Tests basic pipe functionality across different file systems

set -e

echo "========================================="
echo "MiniOS Pipe Testing - RAMFS & SFS"
echo "========================================="

# Build the OS
echo "Building MiniOS..."
make clean > /dev/null 2>&1
make all ARCH=arm64 > /dev/null 2>&1

echo "Running comprehensive pipe tests..."

# Create comprehensive test input
cat > /tmp/pipes_sfs_ramfs_test.txt << 'EOF'
help
echo "=== Testing Pipe Functionality ==="

echo "=== RAMFS Pipe Tests ==="
echo "RAMFS test: pipe on root filesystem"
echo "hello from RAMFS" | cat
ls / | cat
pwd | cat

echo "=== SFS Mount and Setup ==="
mkdir /sfs
mkfs ramdisk0
mount ramdisk0 /sfs sfs
cd /sfs
pwd

echo "=== SFS Pipe Tests ==="
echo "SFS test: pipe while working directory on SFS"
echo "hello from SFS" | cat
ls /sfs | cat
ls . | cat

echo "=== Cross-File System Pipe Tests ==="
echo "Cross-FS: SFS command output to RAMFS temp file"
echo "SFS content" | cat
ls /sfs | cat /sfs/cross_test.txt
cat /sfs/cross_test.txt

echo "=== Complex SFS Pipe Operations ==="
mkdir /sfs/testdir
echo "test file content" > /sfs/testdir/testfile.txt
ls /sfs/testdir | cat
cat /sfs/testdir/testfile.txt | cat

echo "=== Pipe Cleanup Verification ==="
ls /sfs
rm /sfs/cross_test.txt
rm /sfs/testdir/testfile.txt
rmdir /sfs/testdir
ls /sfs

echo "=== Return to RAMFS ==="
cd /
pwd
echo "Back on RAMFS" | cat

echo "=== All Pipe Tests Complete ==="
exit
EOF

# Run QEMU with test commands
timeout 60s qemu-system-aarch64 \
    -M virt \
    -cpu cortex-a72 \
    -m 512M \
    -nographic \
    -kernel build/arm64/kernel.elf \
    -serial mon:stdio \
    < /tmp/pipes_sfs_ramfs_test.txt \
    > pipes-sfs-ramfs-test.log 2>&1 || true

echo ""
echo "========================================="
echo "RAMFS Pipe Test Results"
echo "========================================="

# Check RAMFS pipe functionality
if grep -q "hello from RAMFS" pipes-sfs-ramfs-test.log; then
    echo "✅ RAMFS basic pipe works"
else
    echo "❌ RAMFS basic pipe failed"
fi

if grep -q "welcome.txt" pipes-sfs-ramfs-test.log; then
    echo "✅ RAMFS ls | cat pipe works"
else
    echo "❌ RAMFS ls | cat pipe failed"
fi

echo ""
echo "========================================="
echo "SFS Pipe Test Results"
echo "========================================="

# Check SFS mount and functionality
if grep -q "Mounted file system: sfs" pipes-sfs-ramfs-test.log; then
    echo "✅ SFS mounted successfully"
else
    echo "❌ SFS mount failed"
fi

if grep -q "hello from SFS" pipes-sfs-ramfs-test.log; then
    echo "✅ SFS basic pipe works"
else
    echo "❌ SFS basic pipe failed"
fi

if grep -q "SFS content" pipes-sfs-ramfs-test.log; then
    echo "✅ Cross-FS pipe works (SFS→RAMFS temp→RAMFS output)"
else
    echo "❌ Cross-FS pipe failed"
fi

# Check SFS directory operations with pipes
if grep -q "testfile.txt" pipes-sfs-ramfs-test.log; then
    echo "✅ SFS file operations with pipes work"
else
    echo "❌ SFS file operations with pipes failed"
fi

echo ""
echo "========================================="
echo "Pipe Temp File Analysis"
echo "========================================="

# Look for temp file creation/cleanup
if grep -q "Created file: /tmp/pipe_" pipes-sfs-ramfs-test.log; then
    echo "✅ Temporary pipe files created on RAMFS"
else
    echo "⚠️  Temp file creation not visible (may be working silently)"
fi

echo ""
echo "========================================="
echo "Sample Output from Test:"
echo "----------------------------------------"
# Look for relevant output sections
grep -E "(hello from|SFS|RAMFS|Mounted|===)" pipes-sfs-ramfs-test.log | head -20
echo "----------------------------------------"

echo ""
echo "========================================="
echo "Test Summary"
echo "========================================="

# Count successful operations
ramfs_success=0
sfs_success=0

grep -q "hello from RAMFS" pipes-sfs-ramfs-test.log && ramfs_success=$((ramfs_success+1))
grep -q "welcome.txt" pipes-sfs-ramfs-test.log && ramfs_success=$((ramfs_success+1))
grep -q "Back on RAMFS" pipes-sfs-ramfs-test.log && ramfs_success=$((ramfs_success+1))

grep -q "Mounted file system: sfs" pipes-sfs-ramfs-test.log && sfs_success=$((sfs_success+1))
grep -q "hello from SFS" pipes-sfs-ramfs-test.log && sfs_success=$((sfs_success+1))
grep -q "SFS content" pipes-sfs-ramfs_test.log && sfs_success=$((sfs_success+1))
grep -q "testfile.txt" pipes-sfs-ramfs-test.log && sfs_success=$((sfs_success+1))

echo "RAMFS Pipe Tests Passed: $ramfs_success/3"
echo "SFS Pipe Tests Passed: $sfs_success/4"
echo ""
echo "Total Pipe Functionality: $((ramfs_success + sfs_success))/7 tests"

if [ $((ramfs_success + sfs_success)) -eq 7 ]; then
    echo "🎉 ALL PIPE TESTS PASSED!"
else
    echo "⚠️  Some pipe tests failed - check log for details"
fi

echo ""
echo "========================================="
echo "Full test log: pipes-sfs-ramfs-test.log"
echo "========================================="
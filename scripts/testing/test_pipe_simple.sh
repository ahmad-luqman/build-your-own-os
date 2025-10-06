#!/bin/bash

# Simple test for basic pipe functionality on both RAMFS and SFS

set -e

echo "========================================="
echo "MiniOS Simple Pipe Test"
echo "========================================="

# Build the OS
echo "Building MiniOS..."
make kernel ARCH=arm64 > /dev/null 2>&1

# Create disk image if it doesn't exist
if [ ! -f disk.img ]; then
    echo "Creating disk image..."
    dd if=/dev/zero of=disk.img bs=1M count=10 > /dev/null 2>&1
fi

echo "Running pipe test in QEMU VM..."

# Create test input with pipe commands for both RAMFS and SFS
cat > /tmp/simple_pipe_test.txt << 'EOF'
echo "=== RAMFS Pipe Tests ==="
echo hello | cat
echo "testing pipes" | cat
ls /tmp | cat

echo ""
echo "=== SFS Pipe Tests ==="
mkfs sfs /dev/vda 1024
mount sfs /dev/vda /sfs
echo "sfs test" > /sfs/test.txt
cat /sfs/test.txt | cat
ls /sfs | cat

echo ""
echo "=== Pipe Chain Test ==="
echo "multi stage" | cat | cat

exit
EOF

# Run QEMU with commands
timeout 45s qemu-system-aarch64 \
    -M virt \
    -cpu cortex-a72 \
    -m 512M \
    -nographic \
    -kernel build/arm64/kernel.elf \
    -drive if=none,format=raw,file=disk.img,id=hd0 \
    -device virtio-blk-device,drive=hd0 \
    < /tmp/simple_pipe_test.txt \
    > simple-pipe-test.log 2>&1 || true

echo ""
echo "Test output:"
cat simple-pipe-test.log

echo ""
echo "========================================="
echo "Checking results..."
echo "========================================="

# Track test results
TESTS_PASSED=0
TESTS_FAILED=0

# Check for expected outputs
echo "Checking RAMFS pipe tests..."
if grep -q "^hello$" simple-pipe-test.log; then
    echo "✅ RAMFS: echo hello | cat - PASSED"
    ((TESTS_PASSED++))
else
    echo "❌ RAMFS: echo hello | cat - FAILED"
    ((TESTS_FAILED++))
fi

if grep -q "testing pipes" simple-pipe-test.log; then
    echo "✅ RAMFS: echo with text | cat - PASSED"
    ((TESTS_PASSED++))
else
    echo "❌ RAMFS: echo with text | cat - FAILED"
    ((TESTS_FAILED++))
fi

# Check SFS pipe tests
echo ""
echo "Checking SFS pipe tests..."
if grep -q "sfs test" simple-pipe-test.log; then
    echo "✅ SFS: cat file | cat - PASSED"
    ((TESTS_PASSED++))
else
    echo "❌ SFS: cat file | cat - FAILED"
    ((TESTS_FAILED++))
fi

# Check for errors
echo ""
echo "Checking for errors..."
if grep -q "Invalid pipe syntax" simple-pipe-test.log; then
    echo "❌ Pipe syntax errors detected"
    ((TESTS_FAILED++))
fi

if grep -q "Missing command after pipe" simple-pipe-test.log; then
    echo "❌ Pipe parsing errors detected"
    ((TESTS_FAILED++))
fi

# Check that old detection message is gone
if grep -q "Basic pipe detected:" simple-pipe-test.log; then
    echo "❌ Old pipe detection message still present"
    ((TESTS_FAILED++))
else
    echo "✅ No detection messages (pipes executing properly)"
    ((TESTS_PASSED++))
fi

# Check shell started
if grep -q "MiniOS Shell" simple-pipe-test.log; then
    echo "✅ Shell started successfully"
    ((TESTS_PASSED++))
else
    echo "❌ Shell did not start"
    ((TESTS_FAILED++))
fi

echo ""
echo "========================================="
echo "Test Summary"
echo "========================================="
echo "Tests Passed: $TESTS_PASSED"
echo "Tests Failed: $TESTS_FAILED"
echo ""
echo "Full log: simple-pipe-test.log"

if [ $TESTS_FAILED -eq 0 ]; then
    echo ""
    echo "🎉 All pipe tests PASSED!"
    exit 0
else
    echo ""
    echo "⚠️  Some tests failed. Check simple-pipe-test.log for details."
    exit 1
fi

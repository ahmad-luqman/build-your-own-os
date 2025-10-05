#!/bin/bash

# Simple test for basic pipe functionality

set -e

echo "========================================="
echo "MiniOS Simple Pipe Test"
echo "========================================="

# Build the OS
echo "Building MiniOS..."
make kernel ARCH=arm64 > /dev/null 2>&1

echo "Running simple pipe test..."

# Create test input with basic commands
cat > /tmp/simple_pipe_test.txt << 'EOF'
echo "test pipe" | cat
exit
EOF

# Run QEMU with commands
timeout 35s qemu-system-aarch64 \
    -M virt \
    -cpu cortex-a72 \
    -m 512M \
    -nographic \
    -kernel build/arm64/kernel.elf \
    < /tmp/simple_pipe_test.txt \
    > simple-pipe-test.log 2>&1 || true

echo ""
echo "Test output:"
cat simple-pipe-test.log

echo ""
echo "========================================="
echo "Checking results..."
echo "========================================="

# Check for expected outputs
if grep -q "test pipe" simple-pipe-test.log; then
    echo "✅ Basic pipe works!"
else
    echo "❌ Basic pipe failed"
fi

if grep -q "MiniOS Shell" simple-pipe-test.log; then
    echo "✅ Shell started"
else
    echo "❌ Shell not started"
fi

echo ""
echo "Full log: simple-pipe-test.log"
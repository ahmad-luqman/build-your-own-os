#!/bin/bash
# Test script for Bug #1: Echo output redirection
# This will run MiniOS in QEMU and test if echo redirection works

set -e

ARCH="${1:-arm64}"
TEST_OUTPUT="/tmp/minios_echo_test_output.txt"

echo "========================================="
echo "Testing Bug #1: Echo Output Redirection"
echo "========================================="
echo ""

# Build the system
echo "Building MiniOS for $ARCH..."
make ARCH=$ARCH > /dev/null 2>&1 || {
    echo "ERROR: Build failed"
    exit 1
}
echo "✓ Build successful"
echo ""

# Create test commands
cat > /tmp/minios_test_input.txt << 'EOF'
ls
echo "Testing output redirection bug..."
echo "This should go to file" > testfile.txt
ls
cat testfile.txt
echo "---TEST COMPLETE---"
EOF

echo "Test commands:"
cat /tmp/minios_test_input.txt
echo ""
echo "========================================="
echo "Running MiniOS in QEMU..."
echo "========================================="
echo ""

# Run in QEMU with timeout
timeout 30 qemu-system-aarch64 \
    -M virt \
    -cpu cortex-a72 \
    -m 512M \
    -nographic \
    -kernel build/arm64/kernel.elf \
    -serial mon:stdio \
    < /tmp/minios_test_input.txt 2>&1 | tee "$TEST_OUTPUT" || true

echo ""
echo "========================================="
echo "Analyzing Results..."
echo "========================================="
echo ""

# Check if the bug is present
if grep -q "This should go to file" "$TEST_OUTPUT" | grep -v "cat testfile.txt"; then
    echo "🔴 BUG CONFIRMED: Text appeared on console (should be in file only)"
    echo ""
    echo "Looking for evidence..."
    grep -A 2 -B 2 "This should go to file" "$TEST_OUTPUT" | head -10
else
    echo "🟢 BUG MIGHT BE FIXED: Text did not appear on console"
fi

echo ""
echo "Full output saved to: $TEST_OUTPUT"
echo ""

# Cleanup
rm -f /tmp/minios_test_input.txt

echo "Review the output above to confirm the bug behavior."

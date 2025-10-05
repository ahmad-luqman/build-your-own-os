#!/bin/bash

# Test script for basic pipe functionality in MiniOS shell

set -e

echo "========================================="
echo "MiniOS Pipe Functionality Test"
echo "========================================="

# Build the OS
echo "Building MiniOS..."
make kernel ARCH=arm64 > /dev/null 2>&1

echo "Running pipe functionality test..."

# Create test input
cat > /tmp/pipe_test.txt << 'EOF'
help
echo "=== Testing Basic Pipes ==="
echo "hello world" | cat
ls / | cat
echo "line1" | cat
echo "Testing multiple commands:" | cat
mkdir /tmp/pipe_test
echo "test content" > /tmp/pipe_test/test.txt
cat /tmp/pipe_test/test.txt | cat
ls /tmp/pipe_test | cat
rm /tmp/pipe_test/test.txt
rmdir /tmp/pipe_test
echo "=== Pipe Tests Complete ==="
exit
EOF

# Run QEMU with test commands
timeout 35s qemu-system-aarch64 \
    -M virt \
    -cpu cortex-a72 \
    -m 512M \
    -nographic \
    -kernel build/arm64/kernel.elf \
    < /tmp/pipe_test.txt \
    > serial-pipe-test.log 2>&1 || true

echo ""
echo "========================================="
echo "Pipe Test Results"
echo "========================================="

# Check if pipes are working
if grep -q "hello world" serial-pipe-test.log; then
    echo "✅ Basic echo | cat pipe works"
else
    echo "❌ Basic echo | cat pipe failed"
fi

if grep -q "welcome.txt" serial-pipe-test.log && grep -q "tmp" serial-pipe-test.log; then
    echo "✅ ls | cat pipe works"
else
    echo "❌ ls | cat pipe failed"
fi

if grep -q "line1" serial-pipe-test.log; then
    echo "✅ Simple string piping works"
else
    echo "❌ Simple string piping failed"
fi

if grep -q "test content" serial-pipe-test.log; then
    echo "✅ File content piping works"
else
    echo "❌ File content piping failed"
fi

# Check for temporary file creation and cleanup
if grep -q "Created file: /tmp/pipe_0" serial-pipe-test.log; then
    echo "✅ Temporary pipe file created"
else
    echo "⚠️  Temporary pipe file creation not verified"
fi

echo ""
echo "========================================="
echo "Sample output from test:"
echo "----------------------------------------"
# Look for shell prompts and commands
grep -E "(MiniOS>|hello world|welcome.txt|line1|test content)" serial-pipe-test.log | head -20
echo "----------------------------------------"

echo ""
echo "========================================="
echo "Full test log available at: serial-pipe-test.log"
echo "========================================="
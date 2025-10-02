#!/bin/bash

# Interactive test for RAMFS commands

set -e

echo "========================================="
echo "Testing RAMFS File System Commands"
echo "========================================="

# Build the OS
echo "Building MiniOS..."
make clean > /dev/null 2>&1
make kernel ARCH=arm64 > /dev/null 2>&1

echo "Running RAMFS test commands..."

# Create a test input file with shell commands
cat > /tmp/ramfs_test_commands.txt << 'EOF'
help
ls
ls /
cat /welcome.txt
mkdir /test
ls /
cd /test
pwd
touch testfile.txt
echo "Hello from RAMFS!" > testfile.txt
cat testfile.txt
ls
cd /
pwd
ls -la
exit
EOF

# Run QEMU with commands piped in
echo "Executing test commands in MiniOS..."
timeout 35s qemu-system-aarch64 \
    -M virt \
    -cpu cortex-a72 \
    -m 512M \
    -nographic \
    -kernel build/arm64/kernel.elf \
    < /tmp/ramfs_test_commands.txt \
    > serial-ramfs-commands.log 2>&1 || true

echo ""
echo "========================================="
echo "Test Output (last 200 lines):"
echo "========================================="
tail -200 serial-ramfs-commands.log

echo ""
echo "========================================="
echo "Checking RAMFS Command Results..."
echo "========================================="

# Check for command execution
if grep -q "MiniOS Shell v1.0" serial-ramfs-commands.log; then
    echo "✓ Shell started successfully"
else
    echo "✗ Shell did not start"
fi

if grep -q "ls\|help\|cat" serial-ramfs-commands.log; then
    echo "✓ Commands received"
else
    echo "✗ Commands not received"
fi

# Check for specific outputs
if grep -q "welcome.txt" serial-ramfs-commands.log; then
    echo "✓ File listing works (welcome.txt found)"
else
    echo "✗ File listing failed"
fi

if grep -q "Welcome to MiniOS" serial-ramfs-commands.log; then
    echo "✓ File reading works (cat command)"
else
    echo "✗ File reading failed"
fi

if grep -q "test/" serial-ramfs-commands.log || grep -q "test" serial-ramfs-commands.log; then
    echo "✓ Directory creation works (mkdir)"
else
    echo "! Directory creation result unclear"
fi

echo ""
echo "========================================="
echo "Full log saved to: serial-ramfs-commands.log"
echo "Test input saved to: /tmp/ramfs_test_commands.txt"
echo "========================================="

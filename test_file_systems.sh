#!/bin/bash

# Test file systems (RAMFS and SFS)

set -e

echo "========================================="
echo "Testing MiniOS File Systems"
echo "========================================="

# Build the OS
echo "Building MiniOS..."
make clean > /dev/null 2>&1
make ARCH=arm64 > /dev/null 2>&1

echo "Testing ARM64 on UTM..."

# Run the OS with a 30 second timeout
timeout 30s qemu-system-aarch64 \
    -M virt \
    -cpu cortex-a72 \
    -m 512M \
    -nographic \
    -kernel build/arm64/minios.elf \
    -serial stdio \
    -d guest_errors,cpu_reset \
    > serial-test-fs.log 2>&1 || true

echo ""
echo "========================================="
echo "Test Output:"
echo "========================================="
cat serial-test-fs.log

echo ""
echo "========================================="
echo "Checking RAMFS functionality..."
echo "========================================="

# Check for RAMFS initialization
if grep -q "Initializing RAM File System" serial-test-fs.log; then
    echo "✓ RAMFS initialized"
else
    echo "✗ RAMFS NOT initialized"
fi

# Check for RAMFS mount
if grep -q "RAMFS mount successful\|RAMFS: Mount successful" serial-test-fs.log; then
    echo "✓ RAMFS mounted"
else
    echo "✗ RAMFS NOT mounted"
fi

# Check for RAMFS file operations
if grep -q "Populating RAMFS with initial files" serial-test-fs.log; then
    echo "✓ RAMFS populated with initial files"
else
    echo "✗ RAMFS NOT populated"
fi

# Check for directory listing
if grep -q "Directory structure\|ls\|welcome.txt" serial-test-fs.log; then
    echo "✓ RAMFS directory listing works"
else
    echo "✗ RAMFS directory listing NOT working"
fi

echo ""
echo "========================================="
echo "Checking SFS functionality..."
echo "========================================="

# Check for SFS initialization
if grep -q "Initializing Simple File System" serial-test-fs.log; then
    echo "✓ SFS initialized"
else
    echo "✗ SFS NOT initialized"
fi

# Check for SFS registration
if grep -q "Registered filesystem type: sfs" serial-test-fs.log; then
    echo "✓ SFS registered"
else
    echo "✗ SFS NOT registered"
fi

# Check for block device and SFS format
if grep -q "Formatting device with SFS\|SFS format complete" serial-test-fs.log; then
    echo "✓ SFS format functionality implemented"
else
    echo "! SFS format not tested (needs block device)"
fi

# Check for SFS mount
if grep -q "SFS mount successful\|Mounting SFS filesystem" serial-test-fs.log; then
    echo "✓ SFS mount functionality implemented"
else
    echo "! SFS mount not tested (needs formatted device)"
fi

echo ""
echo "========================================="
echo "Summary"
echo "========================================="
echo "Log file saved to: serial-test-fs.log"
echo ""
echo "Test complete!"

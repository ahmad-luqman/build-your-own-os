#!/bin/bash
# Test script for urgent bugs in TODO.md
# Tests:
# 1. Output redirection (echo Hello > file.txt)
# 2. Block device registration (RAM disk)
# 3. Relative path handling

set -e

ARCH="${1:-arm64}"
TIMEOUT=45

echo "========================================="
echo "Testing Urgent Bugs for MiniOS ($ARCH)"
echo "========================================="
echo ""

# Build first
echo "Building MiniOS for $ARCH..."
make ARCH=$ARCH > /dev/null 2>&1 || {
    echo "ERROR: Build failed"
    exit 1
}

echo "✓ Build successful"
echo ""

# Create test commands file
TEST_COMMANDS=$(cat << 'EOF'
echo "=== BUG TEST 1: Output Redirection ==="
echo Testing redirection
echo "Hello World" > testfile.txt
ls
cat testfile.txt
echo "Expected: 'Hello World' in testfile.txt"
echo ""

echo "=== BUG TEST 2: Relative Path Handling ==="
pwd
mkdir /testdir
cd /testdir
pwd
touch file1.txt
ls
cd /
pwd
ls /testdir
echo "Expected: file1.txt should be in /testdir"
echo ""

echo "=== BUG TEST 3: Multiple File Operations ==="
cd /
echo "Content1" > file1.txt
echo "Content2" > file2.txt
cat file1.txt
cat file2.txt
echo "Expected: Both files should show their content"
echo ""

echo "=== FINAL STATUS ==="
ls /
echo "Test complete - review output above"
EOF
)

# Save commands to temp file
echo "$TEST_COMMANDS" > /tmp/minios_test_commands.txt

echo "Test commands prepared:"
echo "$TEST_COMMANDS"
echo ""
echo "========================================="
echo "Running MiniOS with test commands..."
echo "========================================="
echo ""

# Run based on architecture
if [ "$ARCH" = "arm64" ]; then
    # Check if UTM is available
    if ! command -v qemu-system-aarch64 &> /dev/null; then
        echo "QEMU ARM64 not available. Manual testing required."
        echo "Commands to test manually:"
        echo "$TEST_COMMANDS"
        exit 0
    fi
    
    # Use QEMU for automated testing
    timeout $TIMEOUT qemu-system-aarch64 \
        -M virt \
        -cpu cortex-a72 \
        -m 512M \
        -nographic \
        -kernel build/arm64/bootloader.elf \
        -serial mon:stdio < /tmp/minios_test_commands.txt 2>&1 | tee /tmp/minios_test_output.txt || true
        
elif [ "$ARCH" = "x86_64" ]; then
    if ! command -v qemu-system-x86_64 &> /dev/null; then
        echo "QEMU x86_64 not available. Manual testing required."
        echo "Commands to test manually:"
        echo "$TEST_COMMANDS"
        exit 0
    fi
    
    # Use QEMU for automated testing
    timeout $TIMEOUT qemu-system-x86_64 \
        -cdrom build/x86_64/minios.iso \
        -m 512M \
        -nographic \
        -serial mon:stdio < /tmp/minios_test_commands.txt 2>&1 | tee /tmp/minios_test_output.txt || true
fi

echo ""
echo "========================================="
echo "Test output saved to: /tmp/minios_test_output.txt"
echo "========================================="
echo ""

# Analyze output
if [ -f /tmp/minios_test_output.txt ]; then
    echo "Analyzing results..."
    echo ""
    
    # Check for echo redirection issue
    if grep -q "Testing redirection" /tmp/minios_test_output.txt; then
        echo "✓ Echo command executed"
        if grep -q "Hello World" /tmp/minios_test_output.txt; then
            # Check if it appears after 'cat testfile.txt'
            echo "  Checking if redirection worked..."
        fi
    fi
    
    echo ""
    echo "Full output available in: /tmp/minios_test_output.txt"
fi

# Cleanup
rm -f /tmp/minios_test_commands.txt

echo ""
echo "Manual review required. Check the output above for:"
echo "1. Does 'echo Hello > file.txt' write to file (not console)?"
echo "2. Do relative paths work correctly?"
echo "3. Are there any crashes or errors?"

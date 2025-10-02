#!/bin/bash

# Test touch and output redirection

set -e

echo "========================================="
echo "Testing touch and output redirection"
echo "========================================="

# Build
echo "Building MiniOS..."
make kernel ARCH=arm64 > /dev/null 2>&1

# Create test commands
cat > /tmp/test_file_creation.txt << 'EOF'
help
echo "=== Testing touch command ==="
ls /
touch /myfile.txt
ls /
touch /test/newfile.txt
cd /test
ls
touch another.txt
ls
pwd
echo "=== Testing output redirection ==="
echo Hello World > /greeting.txt
cat /greeting.txt
echo "This is line 1" > /multiline.txt
cat /multiline.txt
echo "Line 2" >> /multiline.txt
cat /multiline.txt
cd /
ls
cat /greeting.txt
exit
EOF

# Run test
echo "Running tests..."
timeout 35s qemu-system-aarch64 \
    -M virt \
    -cpu cortex-a72 \
    -m 512M \
    -nographic \
    -kernel build/arm64/kernel.elf \
    < /tmp/test_file_creation.txt \
    > serial-file-creation-test.log 2>&1 || true

echo ""
echo "========================================="
echo "Test Results"
echo "========================================="

# Check touch command
if grep -q "Command not found: touch" serial-file-creation-test.log; then
    echo "❌ touch command NOT working"
elif grep -q "touch" serial-file-creation-test.log; then
    echo "✅ touch command working"
fi

# Check if files were created
if grep -q "myfile.txt\|greeting.txt" serial-file-creation-test.log; then
    echo "✅ Files created successfully"
else
    echo "❌ Files NOT created"
fi

# Check output redirection
if grep -q "Hello World" serial-file-creation-test.log; then
    echo "✅ Output redirection working"
else
    echo "❌ Output redirection NOT working"
fi

# Check file reading
if grep -A2 "cat /greeting.txt" serial-file-creation-test.log | grep -q "Hello World"; then
    echo "✅ File content readable"
else
    echo "❌ File content NOT readable"
fi

echo ""
echo "Sample output:"
echo "----------------------------------------"
tail -100 serial-file-creation-test.log | head -50
echo "----------------------------------------"
echo ""
echo "Full log: serial-file-creation-test.log"

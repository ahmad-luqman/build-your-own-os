#!/bin/bash
# Manual test for echo redirection bug
# Run this and manually type commands to test

echo "========================================="
echo "BASELINE TEST: Echo Redirection Bug #1"
echo "========================================="
echo ""
echo "Starting MiniOS in QEMU..."
echo "Once you see the Shell> prompt, type these commands:"
echo ""
echo "  1. ls"
echo "  2. echo Hello World > test.txt"
echo "  3. ls"
echo "  4. cat test.txt"
echo ""
echo "EXPECTED BEHAVIOR (if bug is FIXED):"
echo "  - Step 2 should NOT show 'Hello World' on screen"
echo "  - Step 4 should show 'Hello World' from the file"
echo ""
echo "ACTUAL BEHAVIOR (bug present):"
echo "  - Step 2 WILL show 'Hello World' on screen"
echo "  - Step 4 will show nothing (file is empty or not created)"
echo ""
echo "Press ENTER to start QEMU, Ctrl+A then X to exit QEMU"
read -p ""

cd /Users/ahmadluqman/src/build-your-own-os

qemu-system-aarch64 \
    -M virt \
    -cpu cortex-a72 \
    -m 512M \
    -nographic \
    -kernel build/arm64/bootloader.elf \
    -serial mon:stdio

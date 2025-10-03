#!/bin/bash
# Simple test to boot MiniOS in QEMU and test echo redirection
# Based on VM_SETUP.md documentation

echo "========================================="
echo "MiniOS QEMU Test - Echo Redirection Bug"
echo "========================================="
echo ""
echo "Starting QEMU with kernel.elf (direct boot)..."
echo ""
echo "Once you see 'MiniOS>' prompt, type:"
echo "  ls"
echo "  echo Hello World > test.txt"
echo "  ls"
echo "  cat test.txt"
echo ""
echo "To exit QEMU: Press Ctrl+A then X"
echo ""
echo "Starting in 3 seconds..."
sleep 3

cd /Users/ahmadluqman/src/build-your-own-os

exec qemu-system-aarch64 \
    -machine virt \
    -cpu cortex-a72 \
    -m 512M \
    -nographic \
    -serial mon:stdio \
    -kernel build/arm64/kernel.elf

#!/bin/bash
# Test Phase 3 Only (Foundation + Bootloader + Memory Management)
echo "🧪 Testing Phase 3: Memory Management"
echo "===================================="

# Clean and build with Phase 3 only
make clean
echo "Building Phase 3 kernel..."
make ARCH=arm64 CFLAGS_EXTRA="-DPHASE_3_ONLY" kernel

if [[ $? -eq 0 ]]; then
    echo "✅ Phase 3 kernel built successfully"
    echo "🚀 Starting QEMU test..."
    echo "Expected: Memory management initialization (may hang - that's expected)"
    echo "Press Ctrl+C to exit"
    echo
    
    timeout 30 qemu-system-aarch64 \
        -machine virt \
        -cpu cortex-a72 \
        -m 512M \
        -kernel build/arm64/kernel.elf \
        -nographic \
        -serial mon:stdio
    
    echo
    echo "Phase 3 test completed (timeout after 30s is normal)"
else
    echo "❌ Phase 3 build failed"
    exit 1
fi
#!/bin/bash
# Test Phase 1-2 Only (Foundation + Bootloader)
echo "🧪 Testing Phase 1-2: Foundation + Bootloader"
echo "============================================="

# Clean and build with Phase 1-2 only
make clean
echo "Building Phase 1-2 kernel..."
make ARCH=arm64 CFLAGS_EXTRA="-DPHASE_1_2_ONLY" kernel

if [[ $? -eq 0 ]]; then
    echo "✅ Phase 1-2 kernel built successfully"
    echo "🚀 Starting QEMU test..."
    echo "Expected: Basic boot validation and heartbeat"
    echo "Press Ctrl+C to exit"
    echo
    
    qemu-system-aarch64 \
        -machine virt \
        -cpu cortex-a72 \
        -m 512M \
        -kernel build/arm64/kernel.elf \
        -nographic \
        -serial mon:stdio
else
    echo "❌ Phase 1-2 build failed"
    exit 1
fi
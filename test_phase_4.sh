#!/bin/bash
# Test Phase 4 Only (Foundation + Bootloader + Memory Management + Device Drivers)
echo "🧪 Testing Phase 4: Device Drivers & System Services"
echo "=================================================="

# Clean and build with Phase 4 only
make clean
echo "Building Phase 4 kernel..."
make ARCH=arm64 CFLAGS_EXTRA="-DPHASE_4_ONLY" kernel

if [[ $? -eq 0 ]]; then
    echo "✅ Phase 4 kernel built successfully"
    echo "🚀 Starting QEMU test..."
    echo "Expected: Device driver initialization and system services"
    echo "Press Ctrl+C to exit"
    echo
    
    timeout 90 qemu-system-aarch64 \
        -machine virt \
        -cpu cortex-a72 \
        -m 512M \
        -kernel build/arm64/kernel.elf \
        -nographic \
        -serial mon:stdio
    
    echo
    echo "Phase 4 test completed"
else
    echo "❌ Phase 4 build failed"
    exit 1
fi
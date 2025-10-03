#!/bin/bash
# MiniOS Phases 1-2 Demonstration Script
# Shows working foundation and bootloader functionality

echo "🎬 MiniOS Phases 1-2 Live Demonstration"
echo "======================================="
echo
echo "This script demonstrates the completed Phase 1 (Foundation) and"
echo "Phase 2 (Enhanced Bootloader) functionality of MiniOS."
echo

# Ensure we're using the test kernel
echo "📋 Setting up Phase 1-2 test kernel..."
if [[ ! -f "src/kernel/main_phase12_test.c" ]]; then
    echo "❌ Phase 1-2 test kernel not found!"
    echo "Please run this from the MiniOS project root directory."
    exit 1
fi

# Backup current kernel and use test version
cp src/kernel/main.c src/kernel/main_backup.c 2>/dev/null || true
cp src/kernel/main_phase12_test.c src/kernel/main.c

echo "✅ Phase 1-2 test kernel activated"
echo

echo "🔧 Building ARM64 kernel for demonstration..."
if make ARCH=arm64 kernel > build.log 2>&1; then
    echo "✅ ARM64 kernel built successfully!"
    echo "   Size: $(ls -lh build/arm64/kernel.elf | awk '{print $5}')"
else
    echo "❌ ARM64 build failed - check build.log"
    exit 1
fi
echo

echo "🚀 Starting ARM64 demonstration in QEMU..."
echo "Expected output:"
echo "  - Boot messages showing architecture detection"
echo "  - Phase 1 & 2 completion confirmations"
echo "  - Basic kernel execution tests"
echo "  - Heartbeat showing system stability"
echo
echo "Press Ctrl+C to exit the demonstration."
echo "Starting in 3 seconds..."
sleep 1 && echo "3..." && sleep 1 && echo "2..." && sleep 1 && echo "1..." && echo

# Run the demonstration
qemu-system-aarch64 \
    -machine virt \
    -cpu cortex-a72 \
    -m 512M \
    -kernel build/arm64/kernel.elf \
    -nographic \
    -serial mon:stdio

# Restore original kernel
echo
echo "🔄 Restoring original kernel..."
if [[ -f "src/kernel/main_backup.c" ]]; then
    cp src/kernel/main_backup.c src/kernel/main.c
    rm src/kernel/main_backup.c
    echo "✅ Original kernel restored"
fi

echo
echo "🎉 Phase 1-2 demonstration complete!"
echo "The foundation is solid and ready for Phase 3 development."
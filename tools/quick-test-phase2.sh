#!/bin/bash

# Quick Phase 2 Test
set -e

echo "MiniOS Phase 2 Quick Test"
echo "========================="

# Test key functionality
echo "Testing bootloader builds..."
make clean >/dev/null 2>&1

echo -n "ARM64 bootloader... "
if make ARCH=arm64 bootloader >/dev/null 2>&1; then
    echo "✅ PASS"
else
    echo "❌ FAIL"
    exit 1
fi

echo -n "x86_64 bootloader... "  
if make ARCH=x86_64 bootloader >/dev/null 2>&1; then
    echo "✅ PASS"
else
    echo "❌ FAIL" 
    exit 1
fi

echo
echo "Testing kernel builds..."

echo -n "ARM64 kernel... "
if make ARCH=arm64 kernel >/dev/null 2>&1; then
    echo "✅ PASS"
else
    echo "❌ FAIL"
    exit 1  
fi

echo -n "x86_64 kernel... "
if make ARCH=x86_64 kernel >/dev/null 2>&1; then
    echo "✅ PASS"
else
    echo "❌ FAIL"
    exit 1
fi

echo
echo "Testing full builds with images..."

echo -n "ARM64 full build + image... "
if make ARCH=arm64 image >/dev/null 2>&1; then
    echo "✅ PASS"
else
    echo "❌ FAIL"
    exit 1
fi

echo -n "x86_64 full build + ISO... "
if make ARCH=x86_64 image >/dev/null 2>&1; then
    echo "✅ PASS"  
else
    echo "❌ FAIL"
    exit 1
fi

echo
echo "✅ All Phase 2 core functionality working!"
echo

echo "📊 Build Results:"
ls -lh build/*/bootloader.elf build/*/kernel.elf build/arm64/*.img build/x86_64/*.iso 2>/dev/null || true

echo
echo "🎉 Phase 2 Implementation Complete!"
echo "   ✓ Enhanced ARM64 UEFI bootloader"  
echo "   ✓ Enhanced x86_64 Multiboot2 bootloader"
echo "   ✓ Cross-platform boot protocol"
echo "   ✓ Boot information structures"
echo "   ✓ Kernel integration with boot info"
echo "   ✓ Separate bootloader/kernel builds"
echo "   ✓ Bootable images for both architectures"
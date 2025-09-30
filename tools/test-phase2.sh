#!/bin/bash

# MiniOS Phase 2 Testing
# Tests enhanced bootloader implementation

set -e

echo "MiniOS Phase 2 Automated Testing"
echo "=================================="
echo "Project root: $(pwd)"
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

PASS_COUNT=0
TOTAL_COUNT=0

test_result() {
    ((TOTAL_COUNT++))
    if [ $1 -eq 0 ]; then
        echo -e "✅ ${GREEN}PASS${NC}"
        ((PASS_COUNT++))
    else
        echo -e "❌ ${RED}FAIL${NC}"
    fi
}

echo "🔨 Build System Tests"

# Test 1: ARM64 bootloader builds
echo -n "Test 1: ARM64 bootloader builds ... "
if make ARCH=arm64 bootloader >/dev/null 2>&1; then
    test_result 0
else
    test_result 1
fi

# Test 2: x86_64 bootloader builds  
echo -n "Test 2: x86_64 bootloader builds ... "
if make ARCH=x86_64 bootloader >/dev/null 2>&1; then
    test_result 0
else
    test_result 1
fi

# Test 3: ARM64 kernel builds
echo -n "Test 3: ARM64 kernel builds ... "
if make ARCH=arm64 kernel >/dev/null 2>&1; then
    test_result 0
else
    test_result 1  
fi

# Test 4: x86_64 kernel builds
echo -n "Test 4: x86_64 kernel builds ... "
if make ARCH=x86_64 kernel >/dev/null 2>&1; then
    test_result 0
else
    test_result 1
fi

echo

echo "📄 Boot Protocol Tests"

# Test 5: Boot protocol header exists
echo -n "Test 5: Boot protocol header exists ... "
if [ -f "src/include/boot_protocol.h" ]; then
    test_result 0
else
    test_result 1
fi

# Test 6: ARM64 boot info header exists  
echo -n "Test 6: ARM64 boot info header exists ... "
if [ -f "src/arch/arm64/include/boot_info_arm64.h" ]; then
    test_result 0
else
    test_result 1
fi

# Test 7: x86_64 boot info header exists
echo -n "Test 7: x86_64 boot info header exists ... "
if [ -f "src/arch/x86_64/include/boot_info_x86_64.h" ]; then
    test_result 0
else
    test_result 1
fi

# Test 8: Boot protocol magic number check
echo -n "Test 8: Boot protocol magic number defined ... "
if grep -q "BOOT_PROTOCOL_MAGIC.*0x4D696E694F53" src/include/boot_protocol.h; then
    test_result 0
else
    test_result 1
fi

echo

echo "🚀 Bootloader Implementation Tests"

# Test 9: ARM64 UEFI bootloader source exists
echo -n "Test 9: ARM64 UEFI bootloader source exists ... "
if [ -f "src/arch/arm64/uefi_boot.c" ]; then
    test_result 0
else
    test_result 1
fi

# Test 10: x86_64 Multiboot2 bootloader source exists  
echo -n "Test 10: x86_64 Multiboot2 bootloader source exists ... "
if [ -f "src/arch/x86_64/boot_main.c" ]; then
    test_result 0
else
    test_result 1
fi

# Test 11: ARM64 bootloader has UEFI entry point
echo -n "Test 11: ARM64 bootloader has UEFI entry point ... "
if grep -q "uefi_main" src/arch/arm64/uefi_boot.c; then
    test_result 0
else
    test_result 1
fi

# Test 12: x86_64 bootloader has Multiboot2 support
echo -n "Test 12: x86_64 bootloader has Multiboot2 support ... " 
if grep -q "multiboot_magic.*0x36d76289" src/arch/x86_64/boot_main.c; then
    test_result 0
else
    test_result 1
fi

echo

echo "🏗️  Integration Tests"

# Test 13: Kernel accepts boot_info parameter
echo -n "Test 13: Kernel accepts boot_info parameter ... "
if grep -q "kernel_main.*boot_info" src/kernel/main.c; then
    test_result 0
else
    test_result 1
fi

# Test 14: Boot info validation in kernel
echo -n "Test 14: Boot info validation in kernel ... "
if grep -q "boot_info_valid" src/kernel/main.c; then
    test_result 0
else
    test_result 1
fi

# Test 15: ARM64 full build creates image
echo -n "Test 15: ARM64 full build creates image ... "
make clean >/dev/null 2>&1
if make ARCH=arm64 image >/dev/null 2>&1 && [ -f "build/arm64/minios.img" ]; then
    test_result 0
else
    test_result 1
fi

# Test 16: x86_64 full build creates ISO
echo -n "Test 16: x86_64 full build creates ISO ... "
if make ARCH=x86_64 image >/dev/null 2>&1 && [ -f "build/x86_64/minios.iso" ]; then
    test_result 0
else
    test_result 1
fi

echo

echo "📊 File Structure Tests"

# Test 17: Bootloader ELF files exist
echo -n "Test 17: Bootloader ELF files exist ... "
if [ -f "build/arm64/bootloader.elf" ] && [ -f "build/x86_64/bootloader.elf" ]; then
    test_result 0
else
    test_result 1
fi

# Test 18: Kernel ELF files exist  
echo -n "Test 18: Kernel ELF files exist ... "
if [ -f "build/arm64/kernel.elf" ] && [ -f "build/x86_64/kernel.elf" ]; then
    test_result 0
else
    test_result 1
fi

# Test 19: Architecture-specific linker scripts exist
echo -n "Test 19: Bootloader linker scripts exist ... "
if [ -f "src/arch/arm64/boot.ld" ] && [ -f "src/arch/x86_64/bootloader.ld" ]; then
    test_result 0
else
    test_result 1
fi

# Test 20: Build artifacts have correct sizes (non-zero)
echo -n "Test 20: Build artifacts have non-zero sizes ... "
arm64_bootloader_size=$(stat -f%z build/arm64/bootloader.elf 2>/dev/null || echo 0)
x86_64_bootloader_size=$(stat -f%z build/x86_64/bootloader.elf 2>/dev/null || echo 0)
arm64_kernel_size=$(stat -f%z build/arm64/kernel.elf 2>/dev/null || echo 0)
x86_64_kernel_size=$(stat -f%z build/x86_64/kernel.elf 2>/dev/null || echo 0)

if [ "$arm64_bootloader_size" -gt 1000 ] && [ "$x86_64_bootloader_size" -gt 1000 ] && \
   [ "$arm64_kernel_size" -gt 1000 ] && [ "$x86_64_kernel_size" -gt 1000 ]; then
    test_result 0
else
    test_result 1
fi

echo

echo "==================================="
echo "Phase 2 Test Results Summary"  
echo "==================================="
echo "Tests passed: $PASS_COUNT"
echo "Tests total:  $TOTAL_COUNT"
echo "Pass rate:    $((PASS_COUNT * 100 / TOTAL_COUNT))%"

if [ $PASS_COUNT -eq $TOTAL_COUNT ]; then
    echo -e "✅ ${GREEN}Phase 2 Complete - All tests passed!${NC}"
    echo
    echo "✨ Phase 2 Achievements:"
    echo "  • Enhanced ARM64 UEFI bootloader with memory detection"
    echo "  • Enhanced x86_64 Multiboot2 bootloader with boot info parsing"
    echo "  • Cross-platform boot protocol for unified kernel interface"
    echo "  • Boot information structures with graphics and memory data"
    echo "  • Separate bootloader and kernel builds with proper linking"
    echo "  • Bootable images created for both UTM (ARM64) and QEMU (x86_64)"
    echo
    exit 0
else
    echo -e "❌ ${RED}Phase 2 Incomplete - Some tests failed${NC}"
    echo "Check failed tests above and fix issues."
    exit 1
fi
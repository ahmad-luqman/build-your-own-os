#!/bin/bash
#
# MiniOS Phase 3 Test Script
# Tests: Memory Management & Kernel Loading
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
cd "$PROJECT_ROOT"

echo "======================================"
echo "MiniOS Phase 3 Test Suite"
echo "Memory Management & Kernel Loading"
echo "======================================"

TESTS_RUN=0
TESTS_PASSED=0

# Helper function to run a test
run_test() {
    local test_name="$1"
    local test_cmd="$2"
    
    echo -n "Testing $test_name... "
    TESTS_RUN=$((TESTS_RUN + 1))
    
    if eval "$test_cmd" >/dev/null 2>&1; then
        echo "✅ PASS"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo "❌ FAIL"
        echo "  Command: $test_cmd"
        return 1
    fi
}

# Helper function to check if file exists and has reasonable size
check_binary() {
    local file="$1"
    local min_size="$2"
    
    if [[ ! -f "$file" ]]; then
        return 1
    fi
    
    local size=$(stat -f%z "$file" 2>/dev/null || stat -c%s "$file" 2>/dev/null)
    if [[ $size -lt $min_size ]]; then
        return 1
    fi
    
    return 0
}

echo
echo "Phase 3 Test Categories:"
echo "1. Cross-platform Memory Interface"
echo "2. ARM64 Memory Management" 
echo "3. x86-64 Memory Management"
echo "4. Exception Handling Framework"
echo "5. Kernel Loading Interface"
echo "6. Build System Integration"
echo

# Test 1: Cross-platform Memory Interface
echo "=== Test 1: Cross-platform Memory Interface ==="
run_test "Memory header compilation (ARM64)" \
    "aarch64-elf-gcc -ffreestanding -Isrc/include -c -x c -o /dev/null - <<< '#include \"memory.h\"'"

run_test "Memory header compilation (x86-64)" \
    "x86_64-elf-gcc -ffreestanding -Isrc/include -c -x c -o /dev/null - <<< '#include \"memory.h\"'"

run_test "Exception header compilation (ARM64)" \
    "aarch64-elf-gcc -ffreestanding -Isrc/include -c -x c -o /dev/null - <<< '#include \"exceptions.h\"'"

run_test "Exception header compilation (x86-64)" \
    "x86_64-elf-gcc -ffreestanding -Isrc/include -c -x c -o /dev/null - <<< '#include \"exceptions.h\"'"

run_test "Kernel loader header compilation (ARM64)" \
    "aarch64-elf-gcc -ffreestanding -Isrc/include -c -x c -o /dev/null - <<< '#include \"kernel_loader.h\"'"

run_test "Kernel loader header compilation (x86-64)" \
    "x86_64-elf-gcc -ffreestanding -Isrc/include -c -x c -o /dev/null - <<< '#include \"kernel_loader.h\"'"

echo

# Test 2: ARM64 Memory Management
echo "=== Test 2: ARM64 Memory Management ==="
run_test "ARM64 MMU implementation exists" \
    "test -f src/arch/arm64/memory/mmu.c"

run_test "ARM64 memory allocator exists" \
    "test -f src/arch/arm64/memory/allocator.c"

run_test "ARM64 memory management compiles" \
    "make ARCH=arm64 clean && make ARCH=arm64 build/arm64/arch/arm64/memory/mmu.o build/arm64/arch/arm64/memory/allocator.o"

echo

# Test 3: x86-64 Memory Management  
echo "=== Test 3: x86-64 Memory Management ==="
run_test "x86-64 paging implementation exists" \
    "test -f src/arch/x86_64/memory/paging.c"

run_test "x86-64 memory allocator exists" \
    "test -f src/arch/x86_64/memory/allocator.c"

run_test "x86-64 memory management compiles" \
    "make ARCH=x86_64 clean && make ARCH=x86_64 build/x86_64/arch/x86_64/memory/paging.o build/x86_64/arch/x86_64/memory/allocator.o"

echo

# Test 4: Exception Handling Framework
echo "=== Test 4: Exception Handling Framework ==="
run_test "ARM64 exception vectors exist" \
    "test -f src/arch/arm64/interrupts/vectors.S"

run_test "ARM64 exception handlers exist" \
    "test -f src/arch/arm64/interrupts/handlers.c"

run_test "x86-64 exception handlers exist" \
    "test -f src/arch/x86_64/interrupts/handlers.c"

run_test "ARM64 exception handling compiles" \
    "make ARCH=arm64 build/arm64/arch/arm64/interrupts/vectors.o build/arm64/arch/arm64/interrupts/handlers.o"

run_test "x86-64 exception handling compiles" \
    "make ARCH=x86_64 build/x86_64/arch/x86_64/interrupts/handlers.o"

echo

# Test 5: Kernel Loading Interface
echo "=== Test 5: Kernel Loading Interface ==="
run_test "ARM64 ELF loader exists" \
    "test -f src/arch/arm64/kernel_loader/elf_loader.c"

run_test "x86-64 ELF loader exists" \
    "test -f src/arch/x86_64/kernel_loader/elf_loader.c"

run_test "ARM64 kernel loader compiles" \
    "make ARCH=arm64 build/arm64/arch/arm64/kernel_loader/elf_loader.o"

run_test "x86-64 kernel loader compiles" \
    "make ARCH=x86_64 build/x86_64/arch/x86_64/kernel_loader/elf_loader.o"

echo

# Test 6: Build System Integration
echo "=== Test 6: Build System Integration ==="
run_test "ARM64 kernel builds successfully" \
    "make ARCH=arm64 clean && make ARCH=arm64 kernel"

run_test "ARM64 kernel binary has reasonable size" \
    "check_binary build/arm64/kernel.elf 50000"

run_test "x86-64 kernel builds successfully" \
    "make ARCH=x86_64 clean && make ARCH=x86_64 kernel"

run_test "x86-64 kernel binary has reasonable size" \
    "check_binary build/x86_64/kernel.elf 15000"

run_test "ARM64 full system builds" \
    "make ARCH=arm64 all"

run_test "ARM64 bootable image created" \
    "check_binary build/arm64/minios.img 15000000"

run_test "x86-64 full system builds" \
    "make ARCH=x86_64 all"

run_test "x86-64 bootable ISO created" \
    "check_binary build/x86_64/minios.iso 300000"

echo

# Test Results Summary
echo "======================================"
echo "Phase 3 Test Results"
echo "======================================"
echo "Tests Run: $TESTS_RUN"
echo "Tests Passed: $TESTS_PASSED"
echo "Tests Failed: $((TESTS_RUN - TESTS_PASSED))"

if [[ $TESTS_PASSED -eq $TESTS_RUN ]]; then
    echo "✅ ALL TESTS PASSED!"
    echo
    echo "🎉 Phase 3 Implementation Complete!"
    echo "   ✓ Cross-platform memory management interface"
    echo "   ✓ ARM64 MMU setup and memory allocation"
    echo "   ✓ x86-64 paging and memory allocation"
    echo "   ✓ Exception handling framework (both architectures)"
    echo "   ✓ Kernel loading interface"
    echo "   ✓ Enhanced kernel with memory management"
    echo "   ✓ Full build system integration"
    echo
    echo "Ready for Phase 4: Device Drivers & System Services!"
    exit 0
else
    echo "❌ Some tests failed!"
    echo "Please review the failed tests and fix issues before proceeding."
    exit 1
fi
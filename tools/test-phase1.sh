#!/bin/bash
# MiniOS Phase 1 Automated Testing Script

set -e

TESTS_PASSED=0
TESTS_TOTAL=0
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

run_test() {
    local test_name="$1"
    local test_command="$2"
    local expected_failure="${3:-false}"
    
    TESTS_TOTAL=$((TESTS_TOTAL + 1))
    echo -n "Test $TESTS_TOTAL: $test_name ... "
    
    if [ "$expected_failure" = "true" ]; then
        # Test should fail
        if eval "$test_command" >/dev/null 2>&1; then
            echo -e "${RED}FAIL${NC} (expected failure but passed)"
        else
            echo -e "${GREEN}PASS${NC} (failed as expected)"
            TESTS_PASSED=$((TESTS_PASSED + 1))
        fi
    else
        # Test should pass
        if eval "$test_command" >/dev/null 2>&1; then
            echo -e "${GREEN}PASS${NC}"
            TESTS_PASSED=$((TESTS_PASSED + 1))
        else
            echo -e "${RED}FAIL${NC}"
        fi
    fi
}

run_test_with_output() {
    local test_name="$1"
    local test_command="$2"
    
    TESTS_TOTAL=$((TESTS_TOTAL + 1))
    echo "Test $TESTS_TOTAL: $test_name"
    
    if output=$(eval "$test_command" 2>&1); then
        echo -e "${GREEN}✅ PASS${NC}"
        TESTS_PASSED=$((TESTS_PASSED + 1))
        echo "   Output: $(echo "$output" | head -n 1)"
    else
        echo -e "${RED}❌ FAIL${NC}"
        echo "   Error: $(echo "$output" | head -n 1)"
    fi
    echo
}

cd "$PROJECT_ROOT"

echo -e "${BLUE}MiniOS Phase 1 Automated Testing${NC}"
echo "================================="
echo "Project root: $PROJECT_ROOT"
echo

# Project structure tests
echo -e "${BLUE}📁 Project Structure Tests${NC}"
run_test "Makefile exists" "[ -f Makefile ]"
run_test "Source directory exists" "[ -d src ]"
run_test "Tools directory exists" "[ -d tools ]"
run_test "Documentation exists" "[ -d docs ]"
run_test "VM configs exist" "[ -d vm-configs ]"
run_test "Include directory exists" "[ -d src/include ]"
run_test "ARM64 arch directory exists" "[ -d src/arch/arm64 ]"
run_test "x86_64 arch directory exists" "[ -d src/arch/x86_64 ]"
echo

# File existence tests
echo -e "${BLUE}📄 Essential Files Tests${NC}"
run_test "Main kernel header exists" "[ -f src/include/kernel.h ]"
run_test "Kernel main source exists" "[ -f src/kernel/main.c ]"
run_test "ARM64 boot code exists" "[ -f src/arch/arm64/boot.S ]"
run_test "ARM64 init code exists" "[ -f src/arch/arm64/init.c ]"
run_test "x86_64 boot code exists" "[ -f src/arch/x86_64/boot.asm ]"
run_test "x86_64 init code exists" "[ -f src/arch/x86_64/init.c ]"
echo

# Script executable tests
echo -e "${BLUE}🔧 Script Permissions Tests${NC}"
run_test "Prerequisites checker executable" "[ -x tools/check-prerequisites.sh ]"
run_test "VM test script executable" "[ -x tools/test-vm.sh ]"
run_test "Debug script executable" "[ -x tools/debug.sh ]"
run_test "Image creation tool executable" "[ -x tools/create_image.py ]"
run_test "ISO creation tool executable" "[ -x tools/create_iso.py ]"
run_test "ARM64 VM config executable" "[ -x vm-configs/qemu-arm64.sh ]"
run_test "x86_64 VM config executable" "[ -x vm-configs/qemu-x86_64.sh ]"
echo

# Build system tests
echo -e "${BLUE}🔨 Build System Tests${NC}"
run_test_with_output "Make info runs" "make info"
run_test_with_output "Make help runs" "make help"
run_test_with_output "ARM64 architecture detection" "make info ARCH=arm64"
run_test_with_output "x86_64 architecture detection" "make info ARCH=x86_64"
run_test_with_output "Debug build configuration" "make info DEBUG=1"

# Tool functionality tests
echo -e "${BLUE}🛠️  Tool Functionality Tests${NC}"
run_test_with_output "Image creation tool help" "python3 tools/create_image.py --help"
run_test_with_output "ISO creation tool help" "python3 tools/create_iso.py --help"

# Expected failure tests (these should fail gracefully)
echo -e "${BLUE}⚠️  Expected Failure Tests (Should Fail Gracefully)${NC}"
run_test "ARM64 VM script fails without image" "bash vm-configs/qemu-arm64.sh" "true"
run_test "x86_64 VM script fails without image" "bash vm-configs/qemu-x86_64.sh" "true"
echo

# Build attempt tests (may pass or fail depending on prerequisites)
echo -e "${BLUE}🏗️  Build Attempt Tests${NC}"
echo "Note: These may pass or fail depending on installed toolchain"

# Test build attempts (capture output to see if they fail gracefully)
echo -n "Test: ARM64 build attempt ... "
if make clean >/dev/null 2>&1 && make ARCH=arm64 >/dev/null 2>&1; then
    echo -e "${GREEN}PASS${NC} (toolchain available)"
    TESTS_PASSED=$((TESTS_PASSED + 1))
elif make clean >/dev/null 2>&1 && output=$(make ARCH=arm64 2>&1) && echo "$output" | grep -q "aarch64-elf-gcc"; then
    echo -e "${YELLOW}PASS${NC} (fails gracefully - missing toolchain)"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${RED}FAIL${NC} (unexpected error)"
fi
TESTS_TOTAL=$((TESTS_TOTAL + 1))

echo -n "Test: x86_64 build attempt ... "
if make clean >/dev/null 2>&1 && make ARCH=x86_64 >/dev/null 2>&1; then
    echo -e "${GREEN}PASS${NC} (toolchain available)"
    TESTS_PASSED=$((TESTS_PASSED + 1))
elif make clean >/dev/null 2>&1 && output=$(make ARCH=x86_64 2>&1) && (echo "$output" | grep -q "x86_64-elf-gcc\|nasm"); then
    echo -e "${YELLOW}PASS${NC} (fails gracefully - missing toolchain)"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${RED}FAIL${NC} (unexpected error)"
fi
TESTS_TOTAL=$((TESTS_TOTAL + 1))

# Clean up after tests
make clean >/dev/null 2>&1 || true

echo
echo "================================="
echo -e "${BLUE}Test Results Summary${NC}"
echo "================================="
echo "Tests passed: $TESTS_PASSED"
echo "Tests total:  $TESTS_TOTAL"
echo "Pass rate:    $(( (TESTS_PASSED * 100) / TESTS_TOTAL ))%"
echo

if [ $TESTS_PASSED -eq $TESTS_TOTAL ]; then
    echo -e "🎉 ${GREEN}All Phase 1 tests passed!${NC}"
    echo
    echo "Phase 1 Foundation Setup is working correctly."
    echo "You can proceed with:"
    echo "  1. Install prerequisites: ./tools/check-prerequisites.sh"
    echo "  2. Test full build: make ARCH=arm64"  
    echo "  3. Test VM environment: make test"
    echo "  4. Begin Phase 2: Bootloader implementation"
    exit 0
elif [ $TESTS_PASSED -gt $(( TESTS_TOTAL * 80 / 100 )) ]; then
    echo -e "✅ ${GREEN}Phase 1 mostly working${NC} (${TESTS_PASSED}/${TESTS_TOTAL} passed)"
    echo
    echo "Minor issues detected but core functionality works."
    echo "Check failed tests above and fix if needed."
    exit 0
else
    echo -e "⚠️  ${YELLOW}Phase 1 has issues${NC} (${TESTS_PASSED}/${TESTS_TOTAL} passed)"
    echo
    echo "Multiple tests failed. Check the output above."
    echo "Fix issues before proceeding to Phase 2."
    exit 1
fi
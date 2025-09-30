#!/bin/bash

# MiniOS Phase 4 Test Suite
# Device Drivers & System Services

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Helper functions
print_header() {
    echo -e "${BLUE}======================================${NC}"
    echo -e "${BLUE}MiniOS Phase 4 Test Suite${NC}"
    echo -e "${BLUE}Device Drivers & System Services${NC}"
    echo -e "${BLUE}======================================${NC}"
    echo ""
}

print_test_category() {
    echo -e "${YELLOW}=== $1 ===${NC}"
}

run_test() {
    local test_name="$1"
    local test_command="$2"
    
    TESTS_RUN=$((TESTS_RUN + 1))
    echo -n "Testing $test_name... "
    
    if eval "$test_command" >/dev/null 2>&1; then
        echo -e "${GREEN}✅ PASS${NC}"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo -e "${RED}❌ FAIL${NC}"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
}

print_summary() {
    echo ""
    echo -e "${BLUE}======================================${NC}"
    echo -e "${BLUE}Phase 4 Test Results${NC}"
    echo -e "${BLUE}======================================${NC}"
    echo "Tests Run: $TESTS_RUN"
    echo -e "Tests Passed: ${GREEN}$TESTS_PASSED${NC}"
    echo -e "Tests Failed: ${RED}$TESTS_FAILED${NC}"
    
    if [ $TESTS_FAILED -eq 0 ]; then
        echo -e "${GREEN}✅ ALL TESTS PASSED!${NC}"
        echo ""
        echo -e "${GREEN}🎉 Phase 4 Implementation Complete!${NC}"
        echo -e "${GREEN}   ✓ Device driver framework${NC}"
        echo -e "${GREEN}   ✓ Timer services${NC}"
        echo -e "${GREEN}   ✓ Build system integration${NC}"
        echo -e "${GREEN}   ✓ Cross-platform compatibility${NC}"
        echo ""
        echo -e "${GREEN}Ready for Phase 4 continued development!${NC}"
    else
        echo -e "${RED}❌ SOME TESTS FAILED${NC}"
        echo "Please check the failing tests and fix the issues."
    fi
}

# Start tests
print_header

echo "Phase 4 Test Categories:"
echo "1. Device Driver Framework"
echo "2. Timer Services"
echo "3. Build System Integration"
echo "4. Cross-platform Compatibility"
echo ""

# Test 1: Device Driver Framework
print_test_category "Test 1: Device Driver Framework"

run_test "Device header compilation (ARM64)" "aarch64-elf-gcc -ffreestanding -nostdlib -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -std=c11 -Isrc/include -Isrc/arch/arm64/include -c -x c -o /dev/null - <<< '#include \"device.h\"'"

run_test "Device header compilation (x86-64)" "x86_64-elf-gcc -ffreestanding -nostdlib -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -std=c11 -Isrc/include -Isrc/arch/x86_64/include -c -x c -o /dev/null - <<< '#include \"device.h\"'"

run_test "Driver header compilation (ARM64)" "aarch64-elf-gcc -ffreestanding -nostdlib -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -std=c11 -Isrc/include -Isrc/arch/arm64/include -c -x c -o /dev/null - <<< '#include \"driver.h\"'"

run_test "Driver header compilation (x86-64)" "x86_64-elf-gcc -ffreestanding -nostdlib -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -std=c11 -Isrc/include -Isrc/arch/x86_64/include -c -x c -o /dev/null - <<< '#include \"driver.h\"'"

run_test "Device management implementation exists" "[ -f src/kernel/device.c ]"

run_test "Driver management implementation exists" "[ -f src/kernel/driver.c ]"

run_test "Device management compiles (ARM64)" "aarch64-elf-gcc -ffreestanding -nostdlib -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -std=c11 -Isrc/include -Isrc/arch/arm64/include -O2 -c src/kernel/device.c -o /tmp/device_arm64.o"

run_test "Driver management compiles (x86-64)" "x86_64-elf-gcc -ffreestanding -nostdlib -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -std=c11 -Isrc/include -Isrc/arch/x86_64/include -O2 -c src/kernel/driver.c -o /tmp/driver_x86_64.o"

# Test 2: Timer Services
print_test_category "Test 2: Timer Services"

run_test "Timer header compilation (ARM64)" "aarch64-elf-gcc -ffreestanding -nostdlib -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -std=c11 -Isrc/include -Isrc/arch/arm64/include -c -x c -o /dev/null - <<< '#include \"timer.h\"'"

run_test "Timer header compilation (x86-64)" "x86_64-elf-gcc -ffreestanding -nostdlib -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -std=c11 -Isrc/include -Isrc/arch/x86_64/include -c -x c -o /dev/null - <<< '#include \"timer.h\"'"

run_test "Timer subsystem implementation exists" "[ -f src/kernel/timer.c ]"

run_test "ARM64 timer driver exists" "[ -f src/drivers/timer/arm64_timer.c ]"

run_test "x86-64 timer driver exists" "[ -f src/drivers/timer/x86_64_timer.c ]"

run_test "Timer subsystem compiles (ARM64)" "aarch64-elf-gcc -ffreestanding -nostdlib -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -std=c11 -Isrc/include -Isrc/arch/arm64/include -O2 -c src/kernel/timer.c -o /tmp/timer_arm64.o"

run_test "ARM64 timer driver compiles" "aarch64-elf-gcc -ffreestanding -nostdlib -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -std=c11 -Isrc/include -Isrc/arch/arm64/include -O2 -c src/drivers/timer/arm64_timer.c -o /tmp/arm64_timer.o"

run_test "x86-64 timer driver compiles" "x86_64-elf-gcc -ffreestanding -nostdlib -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -std=c11 -Isrc/include -Isrc/arch/x86_64/include -O2 -c src/drivers/timer/x86_64_timer.c -o /tmp/x86_64_timer.o"

# Test 3: Build System Integration
print_test_category "Test 3: Build System Integration"

run_test "ARM64 kernel builds with Phase 4" "make ARCH=arm64 clean >/dev/null 2>&1 && make ARCH=arm64 kernel >/dev/null 2>&1"

run_test "ARM64 kernel binary has reasonable size" "[ -f build/arm64/kernel.elf ] && [ \$(stat -f%z build/arm64/kernel.elf 2>/dev/null || stat -c%s build/arm64/kernel.elf) -gt 80000 ] && [ \$(stat -f%z build/arm64/kernel.elf 2>/dev/null || stat -c%s build/arm64/kernel.elf) -lt 200000 ]"

run_test "x86-64 kernel builds with Phase 4" "make ARCH=x86_64 clean >/dev/null 2>&1 && make ARCH=x86_64 kernel >/dev/null 2>&1"

run_test "x86-64 kernel binary has reasonable size" "[ -f build/x86_64/kernel.elf ] && [ \$(stat -f%z build/x86_64/kernel.elf 2>/dev/null || stat -c%s build/x86_64/kernel.elf) -gt 20000 ] && [ \$(stat -f%z build/x86_64/kernel.elf 2>/dev/null || stat -c%s build/x86_64/kernel.elf) -lt 100000 ]"

run_test "ARM64 full system builds" "make ARCH=arm64 clean >/dev/null 2>&1 && make ARCH=arm64 all >/dev/null 2>&1"

run_test "ARM64 bootable image created" "[ -f build/arm64/minios.img ]"

run_test "x86-64 full system builds" "make ARCH=x86_64 clean >/dev/null 2>&1 && make ARCH=x86_64 all >/dev/null 2>&1"

run_test "x86-64 bootable ISO created" "[ -f build/x86_64/minios.iso ]"

# Test 4: Cross-platform Compatibility
print_test_category "Test 4: Cross-platform Compatibility"

run_test "Device discovery implementation (ARM64)" "[ -f src/arch/arm64/device_discovery.c ]"

run_test "Device discovery implementation (x86-64)" "[ -f src/arch/x86_64/device_discovery.c ]"

run_test "Device discovery compiles (ARM64)" "aarch64-elf-gcc -ffreestanding -nostdlib -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -std=c11 -Isrc/include -Isrc/arch/arm64/include -O2 -c src/arch/arm64/device_discovery.c -o /tmp/device_discovery_arm64.o"

run_test "Device discovery compiles (x86-64)" "x86_64-elf-gcc -ffreestanding -nostdlib -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -std=c11 -Isrc/include -Isrc/arch/x86_64/include -O2 -c src/arch/x86_64/device_discovery.c -o /tmp/device_discovery_x86_64.o"

run_test "Kernel version updated to Phase 4" "grep -q '0.4.0' src/include/kernel.h"

run_test "Phase 4 includes in kernel header" "grep -q 'device.h' src/include/kernel.h && grep -q 'driver.h' src/include/kernel.h && grep -q 'timer.h' src/include/kernel.h"

run_test "Device initialization in kernel main" "grep -q 'device_init' src/kernel/main.c"

run_test "Timer initialization in kernel main" "grep -q 'timer_init' src/kernel/main.c"

# Clean up temporary files
rm -f /tmp/device_arm64.o /tmp/driver_x86_64.o /tmp/timer_arm64.o /tmp/arm64_timer.o /tmp/x86_64_timer.o
rm -f /tmp/device_discovery_arm64.o /tmp/device_discovery_x86_64.o

print_summary

exit $TESTS_FAILED
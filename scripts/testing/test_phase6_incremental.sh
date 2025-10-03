#!/bin/bash
# Phase 6 Incremental Testing Script
# Tests shell implementation step by step

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log() { echo -e "${BLUE}[PHASE6-TEST]${NC} $1"; }
success() { echo -e "${GREEN}[PHASE6-TEST]${NC} ✅ $1"; }
error() { echo -e "${RED}[PHASE6-TEST]${NC} ❌ $1"; }
warn() { echo -e "${YELLOW}[PHASE6-TEST]${NC} ⚠️  $1"; }

# Test counter
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

run_test() {
    local test_name="$1"
    local test_commands="$2"
    local expected_outputs="$3"
    local arch="${4:-arm64}"
    
    TESTS_RUN=$((TESTS_RUN + 1))
    log "Test $TESTS_RUN: $test_name (arch: $arch)"
    
    # Build for the architecture
    log "Building for $arch..."
    if ! make clean > /dev/null 2>&1; then
        warn "Clean failed, continuing..."
    fi
    
    if ! make ARCH=$arch all > /dev/null 2>&1; then
        error "Build failed for $arch"
        TESTS_FAILED=$((TESTS_FAILED + 1))
        return 1
    fi
    
    local kernel_file="$BUILD_DIR/$arch/kernel.elf"
    if [ ! -f "$kernel_file" ]; then
        error "Kernel not found: $kernel_file"
        TESTS_FAILED=$((TESTS_FAILED + 1))
        return 1
    fi
    
    log "Starting QEMU test..."
    local output_file="$BUILD_DIR/test-phase6-$arch-$TESTS_RUN.log"
    
    # Determine QEMU command based on architecture
    local qemu_cmd=""
    if [ "$arch" = "arm64" ]; then
        qemu_cmd="qemu-system-aarch64 -machine virt -cpu cortex-a72 -m 512M -kernel $kernel_file -nographic -serial mon:stdio"
    else
        qemu_cmd="qemu-system-x86_64 -m 512M -kernel $kernel_file -nographic -serial mon:stdio"
    fi
    
    # Run QEMU with test commands and timeout
    log "Sending commands: $test_commands"
    (
        sleep 2  # Wait for boot
        echo "$test_commands" | tr '|' '\n'
        sleep 2  # Wait for output
        echo "exit"
        sleep 1
    ) | timeout 15 $qemu_cmd > "$output_file" 2>&1 || true
    
    # Analyze output
    log "Analyzing output..."
    local all_passed=true
    
    # Check for each expected output
    IFS='|' read -ra OUTPUTS <<< "$expected_outputs"
    for expected in "${OUTPUTS[@]}"; do
        if grep -q "$expected" "$output_file"; then
            success "Found: $expected"
        else
            error "Missing: $expected"
            all_passed=false
        fi
    done
    
    # Check for errors
    if grep -qi "kernel panic\|UNHANDLED EXCEPTION\|Failed to initialize" "$output_file"; then
        error "Critical error detected in output"
        all_passed=false
    fi
    
    # Show last few lines
    log "Last 15 lines of output:"
    tail -15 "$output_file" | sed 's/^/  /'
    
    if [ "$all_passed" = true ]; then
        success "Test PASSED: $test_name"
        TESTS_PASSED=$((TESTS_PASSED + 1))
        return 0
    else
        error "Test FAILED: $test_name"
        TESTS_FAILED=$((TESTS_FAILED + 1))
        return 1
    fi
}

# Main test execution
main() {
    log "=========================================="
    log "Phase 6 Incremental Shell Testing"
    log "=========================================="
    echo ""
    
    # Test 1: Basic Shell Integration - Boot to prompt
    log "=========================================="
    log "TEST 1: Shell Framework Integration"
    log "Goal: Replace hardcoded loop with shell framework"
    log "=========================================="
    run_test \
        "Shell boots to proper prompt" \
        "" \
        "MiniOS Shell|Type 'help'|MiniOS>" \
        "arm64"
    echo ""
    
    # Test 2: Help command
    log "=========================================="
    log "TEST 2: Basic Commands - Help"
    log "Goal: Test help command works"
    log "=========================================="
    run_test \
        "Help command displays commands" \
        "help" \
        "Available commands|Directory Operations|File Operations|System Information" \
        "arm64"
    echo ""
    
    # Test 3: Echo command
    log "=========================================="
    log "TEST 3: Basic Commands - Echo"
    log "Goal: Test echo command"
    log "=========================================="
    run_test \
        "Echo command works" \
        "echo Hello MiniOS" \
        "Hello MiniOS" \
        "arm64"
    echo ""
    
    # Test 4: PWD command
    log "=========================================="
    log "TEST 4: Directory Navigation - PWD"
    log "Goal: Test pwd shows current directory"
    log "=========================================="
    run_test \
        "PWD shows current directory" \
        "pwd" \
        "/" \
        "arm64"
    echo ""
    
    # Test 5: Clear command
    log "=========================================="
    log "TEST 5: Basic Commands - Clear"
    log "Goal: Test clear screen command"
    log "=========================================="
    run_test \
        "Clear command works" \
        "clear" \
        "" \
        "arm64"
    echo ""
    
    # Test 6: System info - uname
    log "=========================================="
    log "TEST 6: System Information - uname"
    log "Goal: Test system information display"
    log "=========================================="
    run_test \
        "Uname shows system info" \
        "uname" \
        "MiniOS" \
        "arm64"
    echo ""
    
    # Test 7: Test on x86_64 architecture
    log "=========================================="
    log "TEST 7: Cross-Platform - x86_64"
    log "Goal: Verify shell works on x86_64"
    log "=========================================="
    run_test \
        "Shell works on x86_64" \
        "help" \
        "Available commands|help" \
        "x86_64"
    echo ""
    
    # Summary
    log "=========================================="
    log "TEST SUMMARY"
    log "=========================================="
    log "Total tests run: $TESTS_RUN"
    success "Tests passed: $TESTS_PASSED"
    if [ $TESTS_FAILED -gt 0 ]; then
        error "Tests failed: $TESTS_FAILED"
    else
        log "Tests failed: 0"
    fi
    echo ""
    
    if [ $TESTS_FAILED -eq 0 ]; then
        success "=========================================="
        success "ALL TESTS PASSED! ✅"
        success "Phase 6 Task A (Core Integration) COMPLETE"
        success "=========================================="
        return 0
    else
        error "=========================================="
        error "SOME TESTS FAILED ❌"
        error "Review the logs above for details"
        error "=========================================="
        return 1
    fi
}

# Run main
main "$@"

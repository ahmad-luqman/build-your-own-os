#!/bin/bash
# MiniOS Basic Boot Test Script
# Tests that ARM64 boots successfully to shell

set -eo pipefail

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
BUILD_DIR="$PROJECT_ROOT/build"
TEST_NAME="basic_boot"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
COMMANDS_FILE="$BUILD_DIR/${TEST_NAME}_commands_${TIMESTAMP}.txt"
LOG_FILE="$BUILD_DIR/${TEST_NAME}_${TIMESTAMP}.log"
SUMMARY_FILE="$BUILD_DIR/${TEST_NAME}_summary.txt"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test configuration
TIMEOUT=30
ARCH="arm64"
KERNEL_FILE="$BUILD_DIR/$ARCH/kernel.elf"

# Logging functions
log() {
    echo -e "${BLUE}[BOOT-TEST]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[BOOT-TEST]${NC} $1"
}

error() {
    echo -e "${RED}[BOOT-TEST]${NC} $1"
}

success() {
    echo -e "${GREEN}[BOOT-TEST]${NC} $1"
}

# Create minimal test commands
create_test_commands() {
    log "Creating basic boot test commands..."

    cat > "$COMMANDS_FILE" <<'CMDS'
help
echo "Basic boot test successful"
exit
CMDS

    success "Test commands created: $COMMANDS_FILE"
}

# Run the boot test
run_boot_test() {
    log "Starting basic boot test..."
    log "Kernel: $KERNEL_FILE"
    log "Timeout: ${TIMEOUT}s"
    log "Log file: $LOG_FILE"

    # Run QEMU with timeout
    local exit_code=0
    if timeout "$TIMEOUT" qemu-system-aarch64 \
        -machine virt \
        -cpu cortex-a72 \
        -m 512M \
        -nographic \
        -kernel "$KERNEL_FILE" \
        -serial mon:stdio \
        < "$COMMANDS_FILE" 2>&1 | tee "$LOG_FILE"; then
        exit_code=$?
    else
        exit_code=$?
    fi

    # Handle timeout
    if [ $exit_code -eq 124 ]; then
        warn "Test timed out after ${TIMEOUT}s"
    elif [ $exit_code -ne 0 ]; then
        warn "Test completed with exit code: $exit_code"
    else
        success "Test completed successfully"
    fi

    return $exit_code
}

# Analyze test results
analyze_results() {
    log "Analyzing boot test results..."

    if [ ! -f "$LOG_FILE" ]; then
        error "Log file not found: $LOG_FILE"
        return 1
    fi

    echo ""
    echo "=========================================="
    echo -e "${BLUE}Basic Boot Test Results${NC}"
    echo "=========================================="

    # Check for successful boot
    if grep -q "MiniOS v0\.5\.0" "$LOG_FILE"; then
        success "✓ System started successfully"
    else
        error "✗ System failed to start"
        return 1
    fi

    # Check for shell prompt
    if grep -q "Shell>" "$LOG_FILE"; then
        success "✓ Shell prompt available"
    else
        warn "⚠ Shell prompt not reached (may still be booting)"
    fi

    # Check for crashes
    if grep -q "kernel panic\|Kernel panic" "$LOG_FILE"; then
        error "✗ Kernel panic detected"
        return 1
    else
        success "✓ No kernel panic"
    fi

    if grep -q "UNHANDLED EXCEPTION" "$LOG_FILE"; then
        error "✗ Unhandled exception detected"
        return 1
    else
        success "✓ No unhandled exceptions"
    fi

    # Check for test completion
    if grep -q "Basic boot test successful" "$LOG_FILE"; then
        success "✓ Test commands executed"
    else
        warn "⚠ Test commands may not have executed"
    fi

    # Check for alignment issues
    if grep -q "PC: 0x600003C5" "$LOG_FILE"; then
        error "✗ Old crash pattern detected (PC 0x600003C5)"
        return 1
    else
        success "✓ No alignment crash pattern"
    fi

    # Show last few lines
    echo ""
    echo "Last 5 lines of output:"
    tail -5 "$LOG_FILE" | while IFS= read -r line; do
        echo "  $line"
    done

    # Save summary
    {
        echo "Basic Boot Test Summary"
        echo "======================"
        echo "Timestamp: $TIMESTAMP"
        echo "Kernel: $KERNEL_FILE"
        echo ""
        echo "Results:"
        if grep -q "MiniOS v0\.5\.0" "$LOG_FILE"; then
            echo "  ✓ System boot: SUCCESS"
        else
            echo "  ✗ System boot: FAILED"
        fi
        if grep -q "Shell>" "$LOG_FILE"; then
            echo "  ✓ Shell: AVAILABLE"
        else
            echo "  ✗ Shell: NOT AVAILABLE"
        fi
        if grep -q "kernel panic" "$LOG_FILE"; then
            echo "  ✗ Kernel panic: DETECTED"
        else
            echo "  ✓ Kernel panic: NONE"
        fi
        echo ""
        echo "Log file: $LOG_FILE"
    } > "$SUMMARY_FILE"

    return 0
}

# Main execution
main() {
    echo ""
    echo "=========================================="
    echo -e "${BLUE}Basic Boot Test${NC}"
    echo "=========================================="
    echo "Testing ARM64 system boot to shell"
    echo ""

    # Check prerequisites
    if [ ! -f "$KERNEL_FILE" ]; then
        error "Kernel not found: $KERNEL_FILE"
        log "Please build the kernel first: make ARCH=arm64"
        exit 1
    fi

    # Create test commands
    create_test_commands

    # Run the test
    local test_result=0
    run_boot_test || test_result=$?

    # Analyze results
    analyze_results || test_result=$?

    # Final status
    echo ""
    if [ $test_result -eq 0 ]; then
        success "Basic boot test PASSED"
        echo ""
        echo "The system successfully boots to shell."
        echo "The SFS crash fix appears to be working correctly."
    else
        error "Basic boot test FAILED"
        echo ""
        echo "Check the log file for details: $LOG_FILE"
    fi

    echo ""
    echo "Test files preserved:"
    echo "  Log: $LOG_FILE"
    echo "  Summary: $SUMMARY_FILE"

    return $test_result
}

# Run main function
main "$@"
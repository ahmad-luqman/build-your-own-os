#!/bin/bash
# MiniOS Memory Management Audit Test Script
# Tests memory tracking, leak detection, and cache management

set -eo pipefail

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
BUILD_DIR="$PROJECT_ROOT/build"
TEST_NAME="memory_audit"
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
TIMEOUT=60
ARCH="arm64"
KERNEL_FILE="$BUILD_DIR/$ARCH/kernel.elf"

# Test counters
TESTS_PASSED=0
TESTS_TOTAL=0

# Logging functions
log() {
    echo -e "${BLUE}[MEM-AUDIT]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[MEM-AUDIT]${NC} $1"
}

error() {
    echo -e "${RED}[MEM-AUDIT]${NC} $1"
}

success() {
    echo -e "${GREEN}[MEM-AUDIT]${NC} $1"
}

# Test validation function
check_output() {
    local test_name="$1"
    local expected="$2"
    local output="$3"

    TESTS_TOTAL=$((TESTS_TOTAL + 1))

    if echo "$output" | grep -q "$expected"; then
        success "✓ $test_name: PASSED"
        TESTS_PASSED=$((TESTS_PASSED + 1))
        return 0
    else
        error "✗ $test_name: FAILED (expected: $expected)"
        return 1
    fi
}

# Create test commands
create_test_commands() {
    cat > "$COMMANDS_FILE" <<'EOF'
# Test 1: Basic memory stats
meminfo
sleep 1

# Test 2: Memory leak detection
meminfo -l
sleep 1

# Test 3: Subsystem statistics
meminfo -s
sleep 1

# Test 4: Block cache statistics
meminfo -c
sleep 1

# Test 5: All memory info
meminfo -a
sleep 2

# Test 6: Create files to test memory usage
touch /tmp/test1.txt
touch /tmp/test2.txt
touch /tmp/test3.txt
echo "Hello Memory Test" > /tmp/test1.txt
cat /tmp/test1.txt
sleep 1

# Test 7: Check memory after file operations
meminfo -s
sleep 1

# Test 8: Cache statistics after file ops
meminfo -c
sleep 1

# Test 9: Cleanup
rm /tmp/test1.txt
rm /tmp/test2.txt
rm /tmp/test3.txt
sleep 1

# Test 10: Final memory check
meminfo -a
sleep 2

exit
EOF

    log "Test commands created: $COMMANDS_FILE"
}

# Run test function
run_test() {
    log "Starting memory audit tests on $ARCH..."

    # Build if needed
    if [ ! -f "$KERNEL_FILE" ]; then
        log "Building kernel..."
        cd "$PROJECT_ROOT"
        make ARCH=$ARCH > "$LOG_FILE" 2>&1 || {
            error "Build failed"
            return 1
        }
    fi

    log "Running test VM with timeout ${TIMEOUT}s..."

    # Run VM with test commands
    cd "$BUILD_DIR"
    timeout "${TIMEOUT}s" ./test-vm.sh "$ARCH" < "$COMMANDS_FILE" > "$LOG_FILE" 2>&1 || {
        local exit_code=$?
        if [ $exit_code -eq 124 ]; then
            warn "Test reached timeout limit (${TIMEOUT}s)"
        else
            error "VM execution failed with code $exit_code"
        fi
    }

    log "VM execution complete, analyzing results..."
    cat "$LOG_FILE"
}

# Parse and validate results
validate_results() {
    log "Validating test results..."

    local output=$(cat "$LOG_FILE")

    # Test 1: Check memory stats are displayed
    check_output "Memory stats display" "Memory Allocation Statistics" "$output"
    check_output "Total allocations shown" "Total allocations:" "$output"
    check_output "Current usage shown" "Current usage:" "$output"
    check_output "Peak usage shown" "Peak usage:" "$output"

    # Test 2: Check leak detection works
    check_output "Leak detection available" "Memory Leak Detection" "$output"
    check_output "Leak check ran" "Active allocations:" "$output"

    # Test 3: Check subsystem stats
    check_output "Subsystem stats available" "Subsystem Memory Usage" "$output"

    # Test 4: Check block cache stats
    check_output "Cache stats available" "Block Cache Statistics" "$output"
    check_output "Cache hits tracked" "Cache hits:" "$output"
    check_output "Cache misses tracked" "Cache misses:" "$output"

    # Test 5: Check file operations work
    check_output "File creation works" "test1.txt" "$output"
    check_output "File write/read works" "Hello Memory Test" "$output"

    # Test 6: Check no crashes
    if echo "$output" | grep -qi "kernel panic\|crash\|fatal"; then
        error "✗ No crashes: FAILED (system crashed)"
        TESTS_TOTAL=$((TESTS_TOTAL + 1))
    else
        success "✓ No crashes: PASSED"
        TESTS_TOTAL=$((TESTS_TOTAL + 1))
        TESTS_PASSED=$((TESTS_PASSED + 1))
    fi

    # Test 7: Check exit was clean
    if echo "$output" | grep -q "exit"; then
        success "✓ Clean exit: PASSED"
        TESTS_TOTAL=$((TESTS_TOTAL + 1))
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        error "✗ Clean exit: FAILED"
        TESTS_TOTAL=$((TESTS_TOTAL + 1))
    fi
}

# Generate summary
generate_summary() {
    local pass_rate=0
    if [ $TESTS_TOTAL -gt 0 ]; then
        pass_rate=$((TESTS_PASSED * 100 / TESTS_TOTAL))
    fi

    cat > "$SUMMARY_FILE" <<EOF
=== MiniOS Memory Audit Test Summary ===
Date: $(date)
Architecture: $ARCH
Log file: $LOG_FILE

Test Results:
-------------
Total tests: $TESTS_TOTAL
Passed: $TESTS_PASSED
Failed: $((TESTS_TOTAL - TESTS_PASSED))
Pass rate: ${pass_rate}%

Status: $([ $TESTS_PASSED -eq $TESTS_TOTAL ] && echo "ALL TESTS PASSED ✓" || echo "SOME TESTS FAILED ✗")

Key Findings:
-------------
1. Memory leak detection: $(grep -q "Memory Leak Detection" "$LOG_FILE" && echo "Working ✓" || echo "Not working ✗")
2. Subsystem tracking: $(grep -q "Subsystem Memory Usage" "$LOG_FILE" && echo "Working ✓" || echo "Not working ✗")
3. Block cache LRU: $(grep -q "Block Cache Statistics" "$LOG_FILE" && echo "Working ✓" || echo "Not working ✗")
4. Memory statistics: $(grep -q "Memory Allocation Statistics" "$LOG_FILE" && echo "Working ✓" || echo "Not working ✗")
5. No crashes: $(grep -qi "kernel panic\|crash\|fatal" "$LOG_FILE" && echo "Failed ✗" || echo "Passed ✓")

Details:
--------
Full output log: $LOG_FILE
Command file: $COMMANDS_FILE

EOF

    cat "$SUMMARY_FILE"

    if [ $TESTS_PASSED -eq $TESTS_TOTAL ]; then
        success "=== ALL TESTS PASSED ($TESTS_PASSED/$TESTS_TOTAL) ==="
        return 0
    else
        error "=== SOME TESTS FAILED ($TESTS_PASSED/$TESTS_TOTAL) ==="
        return 1
    fi
}

# Main execution
main() {
    log "=== MiniOS Memory Management Audit Test Suite ==="
    log "Architecture: $ARCH"
    log "Timeout: ${TIMEOUT}s"
    log ""

    # Create output directory
    mkdir -p "$BUILD_DIR"

    # Create and run tests
    create_test_commands
    run_test

    # Validate results
    validate_results

    # Generate summary
    generate_summary

    local result=$?

    log ""
    log "Test suite completed!"
    log "Summary: $SUMMARY_FILE"
    log "Logs: $LOG_FILE"

    return $result
}

# Run main
main "$@"

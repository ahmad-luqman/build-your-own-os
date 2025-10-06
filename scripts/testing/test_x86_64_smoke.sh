#!/bin/bash
# MiniOS x86-64 Basic Smoke Test
# Tests basic boot, shell startup, and simple commands

set -eo pipefail

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
BUILD_DIR="$PROJECT_ROOT/build"
TEST_NAME="x86_64_smoke"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
COMMANDS_FILE="$BUILD_DIR/${TEST_NAME}_commands_${TIMESTAMP}.txt"
LOG_FILE="$BUILD_DIR/${TEST_NAME}_${TIMESTAMP}.log"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test configuration
TIMEOUT=45
ARCH="x86_64"
KERNEL_FILE="$BUILD_DIR/$ARCH/kernel.elf"
ISO_FILE="$BUILD_DIR/$ARCH/minios.iso"

# Test counters
TESTS_PASSED=0
TESTS_FAILED=0
TESTS_TOTAL=0

# Logging functions
log() {
    echo -e "${BLUE}[x86-64-SMOKE]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[x86-64-SMOKE]${NC} $1"
}

error() {
    echo -e "${RED}[x86-64-SMOKE]${NC} $1"
}

success() {
    echo -e "${GREEN}[x86-64-SMOKE]${NC} $1"
}

# Check prerequisites
check_prerequisites() {
    log "Checking prerequisites..."

    if [ ! -f "$ISO_FILE" ]; then
        error "GRUB ISO not found: $ISO_FILE"
        log "Building GRUB ISO..."
        ./tools/create-grub-iso.sh || exit 1
    fi

    if ! command -v qemu-system-x86_64 &> /dev/null; then
        error "qemu-system-x86_64 not found"
        log "Install QEMU: brew install qemu"
        exit 1
    fi

    mkdir -p "$BUILD_DIR"
    success "Prerequisites check passed"
}

# Create test commands
create_test_commands() {
    log "Creating x86-64 smoke test commands..."

    cat > "$COMMANDS_FILE" <<'EOF'
# x86-64 Smoke Test - Basic functionality

echo "=== x86-64 Smoke Test Starting ==="
echo ""

# Test 1: Basic echo
echo "Test 1: Basic echo"
echo "Hello x86-64"
echo ""

# Test 2: pwd
echo "Test 2: Current directory"
pwd
echo ""

# Test 3: ls root
echo "Test 3: List root directory"
ls /
echo ""

# Test 4: Help command
echo "Test 4: Help command"
help
echo ""

# Test 5: Basic file operations on RAMFS
echo "Test 5: RAMFS file operations"
touch /tmp/test_x86.txt
echo "x86-64 test data" > /tmp/test_x86.txt
cat /tmp/test_x86.txt
ls /tmp
echo ""

# Test 6: Directory operations
echo "Test 6: Directory operations"
mkdir /tmp/x86_test_dir
ls /tmp
cd /tmp/x86_test_dir
pwd
cd /
echo ""

# Test 7: File cleanup
echo "Test 7: File cleanup"
rm /tmp/test_x86.txt
rmdir /tmp/x86_test_dir
ls /tmp
echo ""

echo "=== x86-64 Smoke Test Complete ==="
exit
EOF

    success "Test commands created: $COMMANDS_FILE"
}

# Run the test
run_test() {
    log "Starting x86-64 smoke test..."
    log "ISO: $ISO_FILE"
    log "Timeout: ${TIMEOUT}s"
    log "Log file: $LOG_FILE"

    local exit_code=0
    local debug_log="$BUILD_DIR/x86_64_debug_${TIMESTAMP}.log"
    local serial_log="$BUILD_DIR/x86_64_serial_${TIMESTAMP}.log"

    # Use GRUB-based ISO (x86-64 requires bootloader unlike ARM64)
    # Capture both debug console (0xE9) and serial output
    if timeout "$TIMEOUT" qemu-system-x86_64 \
        -m 512M \
        -cdrom "$ISO_FILE" \
        -boot d \
        -nographic \
        -debugcon file:"$debug_log" \
        -serial file:"$serial_log" \
        < "$COMMANDS_FILE" 2>&1; then
        exit_code=$?
    else
        exit_code=$?
    fi

    # Merge debug and serial output into main log
    {
        if [ -f "$debug_log" ]; then
            cat "$debug_log"
        fi
        if [ -f "$serial_log" ]; then
            cat "$serial_log"
        fi
    } | tee "$LOG_FILE"

    if [ $exit_code -eq 124 ]; then
        warn "Test timed out after ${TIMEOUT}s"
        return 1
    elif [ $exit_code -ne 0 ]; then
        warn "Test completed with exit code: $exit_code"
    else
        success "Test completed successfully"
    fi

    return 0
}

# Validate results
check_test() {
    local test_name="$1"
    local pattern="$2"
    local fail_pattern="${3:-kernel panic|EXCEPTION|crash}"

    TESTS_TOTAL=$((TESTS_TOTAL + 1))

    local log_content=$(cat "$LOG_FILE")

    # Check for failure patterns first
    if echo "$log_content" | grep -qi "$fail_pattern"; then
        error "✗ $test_name: FAILED (crash/exception detected)"
        TESTS_FAILED=$((TESTS_FAILED + 1))
        return 1
    fi

    # Check for success pattern
    if echo "$log_content" | grep -q "$pattern"; then
        success "✓ $test_name: PASSED"
        TESTS_PASSED=$((TESTS_PASSED + 1))
        return 0
    else
        error "✗ $test_name: FAILED (expected pattern not found)"
        TESTS_FAILED=$((TESTS_FAILED + 1))
        return 1
    fi
}

analyze_results() {
    log "Analyzing test results..."
    echo ""
    echo "========================================"
    echo -e "${BLUE}x86-64 Smoke Test Results${NC}"
    echo "========================================"
    echo ""

    # Check each test
    check_test "Shell startup" "MiniOS Shell"
    check_test "Basic echo" "Hello x86-64"
    check_test "pwd command" "/"
    check_test "ls command" "tmp"
    check_test "help command" "Available commands"
    check_test "File write" "x86-64 test data"
    check_test "File operations" "test_x86.txt"
    check_test "Directory operations" "x86_test_dir"
    check_test "Test completion" "x86-64 Smoke Test Complete"

    echo ""
    echo "========================================"
    echo "Test Summary"
    echo "========================================"
    echo "Tests passed: $TESTS_PASSED"
    echo "Tests failed: $TESTS_FAILED"
    echo "Tests total:  $TESTS_TOTAL"

    if [ $TESTS_TOTAL -gt 0 ]; then
        echo "Pass rate:    $(( (TESTS_PASSED * 100) / TESTS_TOTAL ))%"
    fi
    echo ""

    if [ $TESTS_PASSED -eq $TESTS_TOTAL ]; then
        success "🎉 All x86-64 smoke tests passed!"
        return 0
    else
        error "❌ Some x86-64 tests failed"
        return 1
    fi
}

# Main execution
main() {
    echo ""
    echo "========================================"
    echo -e "${BLUE}x86-64 Smoke Test${NC}"
    echo "========================================"
    echo ""

    check_prerequisites
    create_test_commands

    local test_result=0
    run_test || test_result=$?

    analyze_results || test_result=$?

    echo ""
    log "Test files:"
    log "  Commands: $COMMANDS_FILE"
    log "  Log:      $LOG_FILE"
    echo ""

    return $test_result
}

# Run main function
main "$@"

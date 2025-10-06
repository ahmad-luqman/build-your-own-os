#!/bin/bash
# MiniOS SFS Comprehensive Test Script
# Tests all SFS operations to identify what works and what doesn't

set -eo pipefail

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
BUILD_DIR="$PROJECT_ROOT/build"
TEST_NAME="sfs_comprehensive"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
COMMANDS_FILE="$BUILD_DIR/${TEST_NAME}_commands_${TIMESTAMP}.txt"
LOG_FILE="$BUILD_DIR/${TEST_NAME}_${TIMESTAMP}.log"
RESULTS_FILE="$BUILD_DIR/${TEST_NAME}_results.md"

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
TESTS_FAILED=0
TESTS_TOTAL=0

# Logging functions
log() {
    echo -e "${BLUE}[SFS-TEST]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[SFS-TEST]${NC} $1"
}

error() {
    echo -e "${RED}[SFS-TEST]${NC} $1"
}

success() {
    echo -e "${GREEN}[SFS-TEST]${NC} $1"
}

# Check prerequisites
check_prerequisites() {
    log "Checking prerequisites..."

    if [ ! -f "$KERNEL_FILE" ]; then
        error "Kernel not found: $KERNEL_FILE"
        log "Please build the kernel first: make ARCH=arm64"
        exit 1
    fi

    if ! command -v qemu-system-aarch64 &> /dev/null; then
        error "qemu-system-aarch64 not found"
        log "Install QEMU: brew install qemu"
        exit 1
    fi

    mkdir -p "$BUILD_DIR"
    success "Prerequisites check passed"
}

# Create comprehensive test commands
create_test_commands() {
    log "Creating comprehensive SFS test commands..."

    cat > "$COMMANDS_FILE" <<'CMDS'
# SFS Comprehensive Test Suite
# Testing all SFS operations to identify working vs broken functionality

echo "=== SFS Comprehensive Test Starting ==="
echo ""

# Phase 1: Setup and Mount
echo "=== Phase 1: Setup and Mount ==="
echo "Test 1.1: List available block devices"
ls /

echo ""
echo "Test 1.2: Create mount point for SFS"
mkdir /sfs

echo ""
echo "Test 1.3: Format ramdisk0 with SFS"
mkfs ramdisk0 sfs

echo ""
echo "Test 1.4: Mount SFS at /sfs"
mount ramdisk0 /sfs sfs

echo ""
echo "Test 1.5: Verify mount succeeded"
ls /sfs

echo ""
echo "Phase 1 complete"
echo ""

# Phase 2: Basic File Creation
echo "=== Phase 2: Basic File Creation ==="
echo "Test 2.1: Create file using touch"
touch /sfs/test1.txt

echo ""
echo "Test 2.2: Create file using echo redirect"
echo "Hello SFS" > /sfs/test2.txt

echo ""
echo "Test 2.3: Create multiple files"
touch /sfs/file1.txt
touch /sfs/file2.txt
touch /sfs/file3.txt

echo ""
echo "Test 2.4: List files in SFS"
ls /sfs

echo ""
echo "Phase 2 complete"
echo ""

# Phase 3: File Write Operations
echo "=== Phase 3: File Write Operations ==="
echo "Test 3.1: Write to new file"
echo "First line" > /sfs/write_test.txt

echo ""
echo "Test 3.2: Append to existing file"
echo "Second line" >> /sfs/write_test.txt

echo ""
echo "Test 3.3: Write multiple lines"
echo "Line 1" > /sfs/multiline.txt
echo "Line 2" >> /sfs/multiline.txt
echo "Line 3" >> /sfs/multiline.txt

echo ""
echo "Test 3.4: Write long content"
echo "This is a longer line of text to test write operations with more content" > /sfs/long.txt

echo ""
echo "Phase 3 complete"
echo ""

# Phase 4: File Read Operations
echo "=== Phase 4: File Read Operations ==="
echo "Test 4.1: Read simple file"
cat /sfs/test2.txt

echo ""
echo "Test 4.2: Read file with append"
cat /sfs/write_test.txt

echo ""
echo "Test 4.3: Read multiline file"
cat /sfs/multiline.txt

echo ""
echo "Test 4.4: Read long content"
cat /sfs/long.txt

echo ""
echo "Phase 4 complete"
echo ""

# Phase 5: File Operations (cp/mv)
echo "=== Phase 5: File Operations ==="
echo "Test 5.1: Copy file within SFS"
cp /sfs/test2.txt /sfs/test2_copy.txt

echo ""
echo "Test 5.2: Verify copied file"
cat /sfs/test2_copy.txt

echo ""
echo "Test 5.3: Move/rename file"
mv /sfs/file1.txt /sfs/file1_renamed.txt

echo ""
echo "Test 5.4: List files after operations"
ls /sfs

echo ""
echo "Phase 5 complete"
echo ""

# Phase 6: Directory Operations
echo "=== Phase 6: Directory Operations ==="
echo "Test 6.1: Create directory"
mkdir /sfs/testdir

echo ""
echo "Test 6.2: Create nested directory"
mkdir /sfs/testdir/subdir

echo ""
echo "Test 6.3: Create file in directory"
echo "Nested file" > /sfs/testdir/nested.txt

echo ""
echo "Test 6.4: List directory contents"
ls /sfs/testdir

echo ""
echo "Test 6.5: Read nested file"
cat /sfs/testdir/nested.txt

echo ""
echo "Phase 6 complete"
echo ""

# Phase 7: File Deletion
echo "=== Phase 7: File Deletion ==="
echo "Test 7.1: Delete single file"
rm /sfs/file2.txt

echo ""
echo "Test 7.2: Verify deletion"
ls /sfs

echo ""
echo "Test 7.3: Delete file in directory"
rm /sfs/testdir/nested.txt

echo ""
echo "Phase 7 complete"
echo ""

# Phase 8: Directory Deletion
echo "=== Phase 8: Directory Deletion ==="
echo "Test 8.1: Remove empty directory"
rmdir /sfs/testdir/subdir

echo ""
echo "Test 8.2: Verify directory removed"
ls /sfs/testdir

echo ""
echo "Phase 8 complete"
echo ""

# Phase 9: Cross-filesystem Operations
echo "=== Phase 9: Cross-filesystem Operations ==="
echo "Test 9.1: Copy from RAMFS to SFS"
cp /tmp/test1.txt /sfs/from_ramfs.txt

echo ""
echo "Test 9.2: Verify copied file"
cat /sfs/from_ramfs.txt

echo ""
echo "Test 9.3: Copy from SFS to RAMFS"
cp /sfs/test2.txt /tmp/from_sfs.txt

echo ""
echo "Test 9.4: Verify copied file"
cat /tmp/from_sfs.txt

echo ""
echo "Phase 9 complete"
echo ""

# Phase 10: Persistence Test
echo "=== Phase 10: Persistence Test ==="
echo "Test 10.1: Write test data"
echo "Persistence test" > /sfs/persist.txt

echo ""
echo "Test 10.2: Read before unmount"
cat /sfs/persist.txt

echo ""
echo "Test 10.3: Unmount SFS"
cd /
umount /sfs

echo ""
echo "Test 10.4: Remount SFS"
mount ramdisk0 /sfs sfs

echo ""
echo "Test 10.5: Read after remount"
cat /sfs/persist.txt

echo ""
echo "Test 10.6: List all files after remount"
ls /sfs

echo ""
echo "Phase 10 complete"
echo ""

# Summary
echo "=== SFS Comprehensive Test Complete ==="
echo "All phases executed"
echo ""

exit
CMDS

    success "Test commands created: $COMMANDS_FILE"
}

# Run the test
run_test() {
    log "Starting SFS comprehensive test..."
    log "Kernel: $KERNEL_FILE"
    log "Timeout: ${TIMEOUT}s"
    log "Commands: $COMMANDS_FILE"
    log "Log file: $LOG_FILE"

    # First create some test files in RAMFS for cross-fs tests
    # This needs to be injected before the main test
    cat > "${COMMANDS_FILE}.tmp" <<'PREP'
# Prepare test environment
echo "Preparing test environment..."
echo "Test data for cross-fs operations" > /tmp/test1.txt
echo "Environment ready"
echo ""
PREP

    cat "$COMMANDS_FILE" >> "${COMMANDS_FILE}.tmp"
    mv "${COMMANDS_FILE}.tmp" "$COMMANDS_FILE"

    local exit_code=0
    if timeout "$TIMEOUT" qemu-system-aarch64 \
        -machine virt \
        -cpu cortex-a72 \
        -m 512M \
        -nographic \
        -kernel "$KERNEL_FILE" \
        -serial mon:stdio \
        -append 'console=uart,mmio,0x9000000' \
        < "$COMMANDS_FILE" 2>&1 | tee "$LOG_FILE"; then
        exit_code=$?
    else
        exit_code=$?
    fi

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

# Analyze results and create detailed report
analyze_results() {
    log "Analyzing test results..."

    if [ ! -f "$LOG_FILE" ]; then
        error "Log file not found: $LOG_FILE"
        return 1
    fi

    local log_content
    log_content=$(cat "$LOG_FILE")

    # Create results markdown file
    cat > "$RESULTS_FILE" <<EOF
# SFS Comprehensive Test Results
**Date:** $(date)
**Kernel:** $KERNEL_FILE
**Log:** $LOG_FILE

---

## Test Phase Results

EOF

    # Function to check test result
    check_test() {
        local phase="$1"
        local test_name="$2"
        local success_pattern="$3"
        local failure_pattern="$4"

        TESTS_TOTAL=$((TESTS_TOTAL + 1))

        if echo "$log_content" | grep -q "$failure_pattern"; then
            TESTS_FAILED=$((TESTS_FAILED + 1))
            echo "### ❌ $phase - $test_name" >> "$RESULTS_FILE"
            echo "**Status:** FAILED" >> "$RESULTS_FILE"
            echo "\`\`\`" >> "$RESULTS_FILE"
            echo "$log_content" | grep -A 3 "$failure_pattern" | head -5 >> "$RESULTS_FILE"
            echo "\`\`\`" >> "$RESULTS_FILE"
            echo "" >> "$RESULTS_FILE"
            error "✗ $phase - $test_name"
            return 1
        elif echo "$log_content" | grep -q "$success_pattern"; then
            TESTS_PASSED=$((TESTS_PASSED + 1))
            echo "### ✅ $phase - $test_name" >> "$RESULTS_FILE"
            echo "**Status:** PASSED" >> "$RESULTS_FILE"
            echo "" >> "$RESULTS_FILE"
            success "✓ $phase - $test_name"
            return 0
        else
            TESTS_FAILED=$((TESTS_FAILED + 1))
            echo "### ⚠️ $phase - $test_name" >> "$RESULTS_FILE"
            echo "**Status:** UNKNOWN (pattern not found)" >> "$RESULTS_FILE"
            echo "" >> "$RESULTS_FILE"
            warn "? $phase - $test_name (no match)"
            return 1
        fi
    }

    echo ""
    echo "========================================"
    echo -e "${BLUE}SFS Comprehensive Test Results${NC}"
    echo "========================================"
    echo ""

    # Phase 1: Setup and Mount
    check_test "Phase 1" "Format ramdisk" "Formatted ramdisk0 with SFS\|SFS format complete" "Failed to format\|format.*failed"
    check_test "Phase 1" "Mount SFS" "Mounting ramdisk0 at /sfs\|SFS mount successful" "Failed to mount\|mount.*failed"
    check_test "Phase 1" "Verify mount" "Phase 1 complete" "kernel panic\|UNHANDLED EXCEPTION"

    # Phase 2: File Creation
    check_test "Phase 2" "Create file (touch)" "Phase 2 complete\|Test 2.2" "Command not found: touch\|Failed to create"
    check_test "Phase 2" "Create file (echo >)" "Phase 2 complete" "kernel panic\|Permission denied"

    # Phase 3: File Write
    check_test "Phase 3" "Write to file" "Phase 3 complete" "kernel panic\|write.*failed"

    # Phase 4: File Read
    check_test "Phase 4" "Read file" "Hello SFS\|Phase 4 complete" "File not found\|Failed to read"

    # Phase 5: File Operations
    check_test "Phase 5" "Copy file" "Copied.*test2_copy\|Phase 5 complete" "Failed to copy\|cp.*failed"
    check_test "Phase 5" "Move file" "Moved.*file1_renamed\|Phase 5 complete" "Failed to move\|mv.*failed"

    # Phase 6: Directory Operations
    check_test "Phase 6" "Create directory" "Directory created.*testdir\|Phase 6 complete" "Failed to create directory\|mkdir.*failed"
    check_test "Phase 6" "Nested file" "Nested file" "File not found"

    # Phase 7: File Deletion
    check_test "Phase 7" "Delete file" "File removed\|Phase 7 complete" "Failed to remove\|rm.*failed"

    # Phase 8: Directory Deletion
    check_test "Phase 8" "Remove directory" "Directory removed\|Phase 8 complete" "Failed to remove\|rmdir.*failed"

    # Phase 10: Persistence
    check_test "Phase 10" "Persistence after remount" "Persistence test" "File not found\|Failed"

    # Summary
    cat >> "$RESULTS_FILE" <<EOF

---

## Summary

- **Total Tests:** $TESTS_TOTAL
- **Passed:** $TESTS_PASSED
- **Failed:** $TESTS_FAILED
- **Pass Rate:** $(( (TESTS_PASSED * 100) / TESTS_TOTAL ))%

## Issues Identified

EOF

    # Add specific issue analysis
    if echo "$log_content" | grep -q "kernel panic\|UNHANDLED EXCEPTION"; then
        echo "- ❌ **CRITICAL:** Kernel panic or unhandled exception detected" >> "$RESULTS_FILE"
    fi

    if ! echo "$log_content" | grep -q "Hello SFS"; then
        echo "- ❌ File read operations not working" >> "$RESULTS_FILE"
    fi

    if ! echo "$log_content" | grep -q "Phase 2 complete"; then
        echo "- ❌ File creation operations failing" >> "$RESULTS_FILE"
    fi

    echo ""
    echo "========================================"
    echo "Test Summary"
    echo "========================================"
    echo "Tests passed: $TESTS_PASSED"
    echo "Tests failed: $TESTS_FAILED"
    echo "Tests total:  $TESTS_TOTAL"
    echo "Pass rate:    $(( (TESTS_PASSED * 100) / TESTS_TOTAL ))%"
    echo ""
    echo "Detailed results: $RESULTS_FILE"
    echo ""

    if [ $TESTS_PASSED -eq $TESTS_TOTAL ]; then
        success "🎉 All SFS tests passed!"
        return 0
    else
        error "❌ Some SFS tests failed"
        return 1
    fi
}

# Main execution
main() {
    echo ""
    echo "========================================"
    echo -e "${BLUE}SFS Comprehensive Test${NC}"
    echo "========================================"
    echo ""

    check_prerequisites
    create_test_commands

    local test_result=0
    run_test || test_result=$?

    analyze_results || test_result=$?

    echo ""
    log "Test files preserved:"
    log "  Commands: $COMMANDS_FILE"
    log "  Log:      $LOG_FILE"
    log "  Results:  $RESULTS_FILE"
    echo ""

    return $test_result
}

# Run main function
main "$@"

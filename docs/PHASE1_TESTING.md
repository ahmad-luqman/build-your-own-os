# Phase 1 Testing Guide

## Overview

This guide covers how to test and validate the Phase 1 Foundation Setup implementation. Follow these steps to ensure your MiniOS build system is working correctly.

## Quick Validation

### Step 1: Check Project Structure
```bash
cd build-your-own-os
ls -la

# Expected output: Should show Makefile, docs/, src/, tools/, vm-configs/
```

### Step 2: Test Build System
```bash
# Show build information
make info

# Show available commands
make help

# Test file permissions
ls -la tools/*.sh vm-configs/*.sh
# All scripts should be executable (rwxr-xr-x)
```

### Step 3: Check Prerequisites
```bash
# Run prerequisite checker
./tools/check-prerequisites.sh

# This will show what's installed and what's missing
# On first run, it may offer to auto-install missing packages
```

## Detailed Testing

### Build System Tests

#### Test 1: Makefile Structure
```bash
# Verify Makefile exists and has correct structure
make info

# Expected output:
# === MiniOS Build System ===
# Architecture: arm64
# Build Type: release
# Compiler: aarch64-elf-gcc (or error if not installed)
# Build Dir: build/arm64
# ===========================
```

#### Test 2: Architecture Support
```bash
# Test ARM64 build configuration
make info ARCH=arm64

# Test x86-64 build configuration  
make info ARCH=x86_64

# Both should show different compiler toolchains
```

#### Test 3: Debug vs Release Builds
```bash
# Test release build (default)
make info DEBUG=0

# Test debug build
make info DEBUG=1

# Should show different optimization levels (-O2 vs -O0)
```

### Tool Validation

#### Test 4: Script Executability
```bash
# Check all scripts are executable
for script in tools/*.sh tools/*.py vm-configs/*.sh; do
    if [ -x "$script" ]; then
        echo "✅ $script - executable"
    else
        echo "❌ $script - not executable"
    fi
done
```

#### Test 5: Python Tools
```bash
# Test image creation tool
python3 tools/create_image.py --help

# Test ISO creation tool  
python3 tools/create_iso.py --help

# Both should show usage information without errors
```

#### Test 6: VM Configuration Scripts
```bash
# Test ARM64 QEMU script (will fail without image, but should show error message)
bash vm-configs/qemu-arm64.sh

# Test x86-64 QEMU script
bash vm-configs/qemu-x86_64.sh

# Expected: "Error: image not found, run make first"
```

## Build Testing (With Prerequisites)

If you have the cross-compilation toolchain installed:

### Test 7: ARM64 Build Attempt
```bash
# Try building for ARM64
make clean
make ARCH=arm64

# Expected outcomes:
# - With toolchain: Should build successfully
# - Without toolchain: Should show clear error about missing aarch64-elf-gcc
```

### Test 8: x86-64 Build Attempt  
```bash
# Try building for x86-64
make clean
make ARCH=x86_64

# Expected outcomes:
# - With toolchain: Should build successfully  
# - Without toolchain: Should show clear error about missing x86_64-elf-gcc or nasm
```

### Test 9: Clean Build Process
```bash
# Test clean functionality
make clean
ls build/

# build/ directory should be empty or not exist

# Test rebuild
make ARCH=arm64
ls build/arm64/

# Should show build artifacts if toolchain is available
```

## VM Testing (Advanced)

If you have QEMU installed:

### Test 10: VM Test Script
```bash
# Test VM script without image (should fail gracefully)
./tools/test-vm.sh arm64

# Expected: Clear error message about missing image file
```

### Test 11: Debug Script
```bash  
# Test debug script setup (should fail gracefully without image)
./tools/debug.sh arm64

# Expected: Should show debug setup information, fail at VM start
```

## Troubleshooting Common Issues

### Issue 1: "Command not found" for make
```bash
# Install Xcode Command Line Tools
xcode-select --install

# Or check if make is in PATH
which make
```

### Issue 2: "Permission denied" for scripts
```bash
# Fix script permissions
chmod +x tools/*.sh tools/*.py vm-configs/*.sh
```

### Issue 3: "No such file or directory" errors
```bash
# Check you're in the correct directory
pwd
# Should show: .../build-your-own-os

# Check project structure
ls -la
# Should show Makefile, src/, docs/, tools/, etc.
```

### Issue 4: Python errors
```bash
# Check Python 3 is available
python3 --version

# Should show Python 3.x.x
```

## Expected Results Summary

### ✅ Working Correctly
When Phase 1 is working correctly, you should see:

1. **make info** - Shows build configuration without errors
2. **make help** - Displays comprehensive help text
3. **Scripts executable** - All .sh and .py files have execute permission
4. **Prerequisites checker** - Runs and identifies missing/installed tools
5. **Clear error messages** - When prerequisites are missing, errors are helpful

### ⚠️ Expected Limitations
Without full prerequisites installed:

1. **Build attempts fail** - But with clear error messages about missing tools
2. **VM tests fail** - But scripts run and show helpful error messages  
3. **Some tools unavailable** - But the framework is ready

### ❌ Problems to Investigate
If you see these, something needs fixing:

1. **Syntax errors in scripts** - Check file formatting
2. **"No such file" for existing files** - Check permissions and paths
3. **Confusing error messages** - Error handling needs improvement
4. **Scripts that hang** - Infinite loops or missing error handling

## Automated Testing Script

Save this as `tools/test-phase1.sh`:

```bash
#!/bin/bash
# Automated Phase 1 Testing

set -e

TESTS_PASSED=0
TESTS_TOTAL=0
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

run_test() {
    local test_name="$1"
    local test_command="$2"
    
    TESTS_TOTAL=$((TESTS_TOTAL + 1))
    echo "Test $TESTS_TOTAL: $test_name"
    
    if eval "$test_command" >/dev/null 2>&1; then
        echo "✅ PASS"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo "❌ FAIL"
    fi
    echo
}

cd "$PROJECT_ROOT"

echo "MiniOS Phase 1 Automated Testing"
echo "================================"
echo

# Structure tests
run_test "Makefile exists" "[ -f Makefile ]"
run_test "Source directory exists" "[ -d src ]"
run_test "Tools directory exists" "[ -d tools ]" 
run_test "Documentation exists" "[ -d docs ]"

# Script tests
run_test "Prerequisites checker executable" "[ -x tools/check-prerequisites.sh ]"
run_test "VM test script executable" "[ -x tools/test-vm.sh ]"
run_test "Debug script executable" "[ -x tools/debug.sh ]"

# Build system tests
run_test "Make info runs" "make info"
run_test "Make help runs" "make help"
run_test "Architecture detection works" "make info ARCH=x86_64"

# Tool tests  
run_test "Python image tool works" "python3 tools/create_image.py --help"
run_test "Python ISO tool works" "python3 tools/create_iso.py --help"

echo "Results: $TESTS_PASSED/$TESTS_TOTAL tests passed"

if [ $TESTS_PASSED -eq $TESTS_TOTAL ]; then
    echo "🎉 All Phase 1 tests passed!"
    exit 0
else
    echo "⚠️  Some tests failed. Check the output above."
    exit 1
fi
```

## Running the Full Test Suite

```bash
# Make the test script executable
chmod +x tools/test-phase1.sh

# Run all Phase 1 tests
./tools/test-phase1.sh

# Expected: Should show test results and overall pass/fail status
```

## Next Steps After Phase 1 Testing

Once Phase 1 testing passes:

1. **Install Prerequisites** - Run `./tools/check-prerequisites.sh` and install missing tools
2. **Test Full Build** - Try `make ARCH=arm64` and `make ARCH=x86_64`  
3. **Test VM Environment** - Try `make test` with actual images
4. **Proceed to Phase 2** - Begin bootloader implementation

The testing framework ensures Phase 1 is solid before moving forward!
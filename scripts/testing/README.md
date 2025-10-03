# Testing Scripts Directory

This directory contains test scripts for validating MiniOS functionality.

## Directory Structure

```
scripts/testing/
├── Phase Testing Scripts
├── Bug Testing Scripts
├── File System Testing Scripts
└── Demo Scripts
```

## Phase Testing Scripts

Scripts for testing specific development phases:

### Multi-Phase Tests
- **test_phases.sh** - Test multiple phases sequentially
- **test_all_phases.sh** - Comprehensive test of all completed phases
- **test_phase_12.sh** - Test phases 1 and 2 together

### Individual Phase Tests
- **test_phase_3.sh** - Test Phase 3 (Memory Management)
- **test_phase_4.sh** - Test Phase 4 (System Services)
- **test_phase6_incremental.sh** - Test Phase 6 (User Interface) incrementally

**Note**: For more comprehensive phase tests, see `tools/test-phase*.sh` scripts.

## Bug Testing Scripts

Scripts created during bug investigation and fixing:

### Bug #1: Output Redirection
- **test_echo_bug.sh** - Script to reproduce the echo redirection bug
- **TEST_ECHO_FIX.sh** - Validation script for Bug #1 fix
- **manual_test_echo.sh** - Manual testing guide for output redirection
- **test_echo_fix.exp** - Expect script for automated testing

### Bug #2: Block Device Registration
- **test_urgent_bugs.sh** - Tests both Bug #1 and Bug #2
- **QEMU_TEST_INSTRUCTIONS.sh** - Detailed instructions for QEMU testing

### General Testing
- **qemu_test.sh** - Simple QEMU boot and test script

**Status**: Both bugs are now fixed! ✅

## File System Testing Scripts

Scripts for testing file system functionality:

- **test_file_creation.sh** - Test file creation operations
- **test_file_systems.sh** - Test multiple file system types
- **test_fs_comprehensive.sh** - Comprehensive file system testing
- **test_ramfs_commands.sh** - Test RAMFS-specific commands

## Demo Scripts

Scripts for demonstrating MiniOS features:

- **demo_phases_12.sh** - Demo of Phases 1-2 (Foundation & Bootloader)
- **demo_phase8_1.sh** - Demo of Phase 8 expansion features

## How to Use

### Running a Test Script

```bash
# From project root
cd /Users/ahmadluqman/src/build-your-own-os

# Make script executable (if needed)
chmod +x scripts/testing/test_phases.sh

# Run the script
bash scripts/testing/test_phases.sh
```

### Running with Specific Architecture

Most scripts support architecture selection:

```bash
# Test with ARM64 (default)
bash scripts/testing/test_phases.sh arm64

# Test with x86_64
bash scripts/testing/test_phases.sh x86_64
```

### Quick Start Tests

For quick validation after making changes:

```bash
# Quick phase test
bash scripts/testing/test_phases.sh

# Quick bug verification
bash scripts/testing/test_urgent_bugs.sh

# Quick file system test
bash scripts/testing/test_ramfs_commands.sh
```

## Test Logs

Test scripts typically generate logs in:
- `/tmp/minios_test_*.txt` - Temporary test logs
- `tmp/` directory in project root - Persistent test logs

## Official Test Scripts

For official phase testing, use the scripts in `tools/` directory:
- `tools/test-phase1.sh` - Official Phase 1 tests
- `tools/test-phase2.sh` - Official Phase 2 tests
- `tools/test-phase3.sh` - Official Phase 3 tests
- `tools/test-phase4.sh` - Official Phase 4 tests
- `tools/test-phase5.sh` - Official Phase 5 tests
- `tools/test-phase6.sh` - Official Phase 6 tests
- `tools/test-phase7.sh` - Official Phase 7 tests

## Creating New Test Scripts

When creating new test scripts:

1. Add them to this directory
2. Make them executable: `chmod +x script.sh`
3. Follow the naming convention: `test_<feature>.sh`
4. Add documentation to this README
5. Include usage instructions in the script header

## Script Template

```bash
#!/bin/bash
# Description of what this script tests
# Usage: bash script.sh [architecture]

set -e

ARCH="${1:-arm64}"
echo "Testing <feature> for $ARCH..."

# Your test code here

echo "Test complete!"
```

## Common Issues

### QEMU Not Found
If QEMU is not available, scripts will show instructions for manual testing.

### Build Not Found
Scripts will automatically build if needed, or show instructions.

### Permission Denied
Make scripts executable: `chmod +x scripts/testing/*.sh`

## Bug Fix Testing (Historical)

These scripts document the bug investigation and fixing process:

### Bug #1 Testing
- **Reproduced**: `test_echo_bug.sh`
- **Fixed**: `TEST_ECHO_FIX.sh`
- **Status**: ✅ FIXED (Jan 3, 2025)

### Bug #2 Testing  
- **Reproduced**: Documented in `test_urgent_bugs.sh`
- **Fixed**: Verified with proper compiler barriers
- **Status**: ✅ FIXED (Jan 3, 2025)

## References

- **Main Documentation**: `docs/development/TODO.md`
- **Bug Fix Details**: `tmp/COMPLETE_BUG_FIX_SUMMARY.md`
- **Test Results**: `tmp/*.log` files
- **Build System**: `tools/` directory

## Maintenance

This directory should be cleaned up periodically:
- Archive obsolete test scripts
- Update documentation
- Remove duplicate functionality
- Consolidate similar tests

---

**Last Updated**: January 3, 2025  
**Status**: Active - All major bugs fixed
**Location**: `scripts/testing/`

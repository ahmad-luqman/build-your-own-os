# MiniOS Known Issues and Limitations

**Last Updated**: October 6, 2025
**Project Status**: Phase 6 (75% complete)

This document tracks all known issues, limitations, and planned improvements for MiniOS.

## Open GitHub Issues

### High Priority Issues
**None currently** - All high-priority bugs have been resolved!

### Medium Priority Issues

#### Issue #3: Enhanced Error Messages
**Status**: Open
**Component**: Shell
**Description**: Improve user-facing error messages for better UX
**Impact**: Low - System works correctly, messages could be more informative

**Current Behavior:**
- Error messages are functional but terse
- Some error conditions could provide more context

**Desired Behavior:**
- More descriptive error messages
- Helpful suggestions for common mistakes
- Better formatting for error output

**Workaround**: None needed - errors are correctly detected and reported

---

#### Issue #11: Comprehensive Error Checking
**Status**: Open
**Component**: SFS, Reliability
**Description**: Add comprehensive error checking for production hardening
**Impact**: Low - Core functionality works, enhanced checks needed for edge cases

**Areas for Improvement:**
- Input validation for all public APIs
- Bounds checking in filesystem operations
- Additional error recovery mechanisms
- Defensive programming enhancements

**Priority**: Medium - enhancement for production readiness

---

#### Issue #13: SFS Unit Test Suite
**Status**: Open
**Component**: SFS, Testing
**Description**: Create automated unit test suite for SFS
**Impact**: Low - SFS works correctly, automated testing would improve confidence

**Planned Tests:**
- File creation and deletion
- Directory operations
- Read/write operations
- Persistence verification
- Edge case handling
- Error condition testing

**Current Testing**: Manual testing and smoke tests (all passing)

---

#### Issue #14: Integration Tests
**Status**: Open
**Component**: Testing, Integration
**Description**: Create integration test suite for cross-component testing
**Impact**: Low - Components work correctly individually and together

**Planned Tests:**
- Cross-filesystem operations (RAMFS ↔ SFS)
- Shell command integration
- Pipe execution across file systems
- Memory management integration
- Full system workflows

**Current Testing**: Manual integration testing (working correctly)

### Low Priority Issues

#### Issue #8: SFS Directory Listing - ls -l
**Status**: Open
**Component**: SFS
**Description**: Implement detailed file metadata display for `ls -l` command
**Impact**: Very Low - Basic `ls` command works perfectly

**Current Behavior:**
- `ls` shows file and directory names correctly
- File metadata (size, permissions, dates) not displayed

**Desired Behavior:**
- `ls -l` shows detailed file information:
  - File permissions
  - File size
  - Creation/modification dates
  - Owner information

**Workaround**: Basic `ls` provides directory listing functionality

## Known Limitations

### File System Limitations

#### 1. SFS Cross-Directory Move Not Implemented
**Component**: SFS, VFS
**Impact**: Low
**Description**: `mv` command only supports same-directory renames

**Current Behavior:**
```bash
mv /sfs/file.txt /sfs/newname.txt    # ✅ Works (same directory)
mv /sfs/dir1/file.txt /sfs/dir2/     # ❌ Not supported (different directory)
```

**Workaround**: Use `cp` followed by `rm` for cross-directory moves:
```bash
cp /sfs/dir1/file.txt /sfs/dir2/file.txt
rm /sfs/dir1/file.txt
```

**Reason**: Simplicity for educational OS - cross-directory move requires:
- Source and destination inode updates
- Directory entry manipulation in two directories
- Atomic operation guarantees
- Additional error handling

**Future**: May be implemented in Phase 7 polish

---

#### 2. Limited ls Command Functionality
**Component**: Shell, File System
**Impact**: Very Low
**Description**: `ls` shows names only, no metadata display

**Missing Features:**
- `ls -l` detailed listing
- File size display
- File permissions display
- Date/time stamps
- Color-coded output

**Workaround**: Basic `ls` provides needed directory listing

**Future**: Enhancement planned for Phase 7

### Memory Management Limitations

#### 1. kfree() is No-Op
**Component**: Memory Management
**Impact**: Medium (but expected for educational OS)
**Description**: Current bump allocator doesn't support memory freeing

**Current Behavior:**
- `kmalloc()` allocates memory successfully
- `kfree()` is a no-op (does nothing)
- All allocations appear as "leaks" in meminfo
- Memory is never reclaimed until reboot

**Impact:**
- System can run 3000-5000 commands before OOM
- Sufficient for educational demonstrations
- Not suitable for long-running production use

**Future**: Free list allocator or slab allocator planned for Phase 7

---

#### 2. Fixed Heap Size
**Component**: Memory Management
**Impact**: Low
**Description**: Static 2MB heap size

**Current Behavior:**
- 2MB static heap buffer
- Cannot grow dynamically
- Sufficient for current workloads

**Workaround**: None needed - 2MB is adequate

**Future**: Dynamic heap expansion may be added

### Shell Limitations

#### 1. No Command Line Editing (Limited)
**Component**: Shell
**Impact**: Low
**Description**: Limited command-line editing capabilities

**Supported:**
- ✅ Backspace
- ✅ Arrow keys for history
- ✅ Ctrl+C (cancel)
- ✅ Ctrl+D (EOF)

**Not Supported:**
- ❌ Left/right arrow navigation
- ❌ Home/End keys
- ❌ Delete key
- ❌ Ctrl+A/E for line start/end

**Workaround**: Use backspace to correct mistakes

---

#### 2. No Background Job Control
**Component**: Shell, Process Management
**Impact**: Low
**Description**: No background process management

**Not Supported:**
- Background job execution with `&`
- Job control (fg, bg commands)
- Process suspension (Ctrl+Z)
- Job listing

**Reason**: Simplified shell for educational purposes

### Process Management Limitations

#### 1. Simple Round-Robin Scheduler
**Component**: Process Management
**Impact**: Low (suitable for educational OS)
**Description**: Basic round-robin scheduling without priorities

**Limitations:**
- No priority-based scheduling
- No real-time support
- Fixed time slices
- No CPU affinity

**Suitable For**: Educational demonstrations and simple workloads

---

#### 2. No Inter-Process Communication (IPC)
**Component**: Process Management
**Impact**: Medium
**Description**: Limited IPC mechanisms

**Not Supported:**
- Message queues
- Shared memory
- Semaphores
- Signals (limited)

**Supported:**
- Pipes (via shell `|` operator using temporary files)
- File-based communication

## Performance Limitations

### 1. Block Cache Size
**Component**: Block Device Layer
**Impact**: Low
**Description**: 32-block cache with LRU eviction

**Limitations:**
- Maximum 32 cached blocks (128KB)
- LRU eviction may cause thrashing under heavy I/O
- No read-ahead or write-behind caching

**Performance**: Adequate for educational use and light workloads

**Future**: Larger cache or adaptive sizing may be added

---

### 2. Linear Search Performance
**Component**: Multiple
**Impact**: Very Low (acceptable for small datasets)

**Areas Using Linear Search:**
- File descriptor table (O(n) for 32 entries)
- Block cache lookup (O(n) for 32 entries)
- Allocation tracking (O(n) for 256 entries)
- Directory listings (O(n) for entries)

**Performance**: Acceptable for educational OS with small datasets

**Future**: Hash tables or more efficient structures for Phase 7

## Architecture Limitations

### 1. Single Core Only
**Component**: System-wide
**Impact**: Low (suitable for educational OS)
**Description**: No SMP (Symmetric Multi-Processing) support

**Limitations:**
- Single CPU core only
- No multi-core scheduling
- No CPU-local data structures
- No inter-processor interrupts

**Suitable For**: Educational demonstrations and development

---

### 2. No Hardware Acceleration
**Component**: System-wide
**Impact**: Very Low
**Description**: No GPU, crypto, or other hardware acceleration

**Not Supported:**
- Graphics acceleration
- Cryptographic hardware
- DMA (Direct Memory Access)
- Hardware virtualization extensions

**Reason**: Simplicity and focus on core OS concepts

## Build System Limitations

### 1. Manual Toolchain Setup
**Component**: Build System
**Impact**: Low
**Description**: Toolchains must be manually installed

**Requirements:**
- ARM64: `aarch64-elf-gcc` toolchain
- x86-64: `x86_64-elf-gcc` toolchain and `nasm`

**Future**: Automated toolchain installation may be added

---

### 2. Limited Platform Support
**Component**: Build System
**Impact**: Low
**Description**: macOS-focused with limited Linux/Windows support

**Supported:**
- ✅ macOS (primary development platform)
- ⚠️ Linux (should work, less tested)
- ❌ Windows (requires WSL or similar)

## Testing Limitations

### 1. Limited Automated Test Coverage
**Component**: Testing
**Impact**: Medium
**Description**: Some areas lack automated tests

**Good Coverage:**
- ✅ RAMFS smoke tests (15/15 passing)
- ✅ Memory audit tests (all passing)
- ✅ Build system tests

**Needs Coverage:**
- ⏳ SFS unit tests (Issue #13)
- ⏳ Integration tests (Issue #14)
- ⏳ Stress tests
- ⏳ Performance regression tests

---

### 2. Manual Testing Required
**Component**: Testing
**Impact**: Low
**Description**: Some features require manual VM testing

**Manual Testing:**
- Shell interaction
- Pipe execution
- Tab completion
- File system persistence

**Reason**: Interactive features difficult to automate

## Documentation Gaps

### Areas Needing Updates
1. **User Guide** - Needs Phase 6 feature documentation (Issue: Update in progress)
2. **API Documentation** - Some newer APIs need documentation
3. **Tutorial** - Step-by-step tutorial for new users
4. **Debugging Guide** - Comprehensive debugging strategies

These will be addressed in Phase 7: Polish & Documentation.

## Security Considerations

**Note**: MiniOS is an **educational operating system** and not designed for production use or security-critical applications.

### Known Security Limitations:
1. **No User Authentication** - Single-user system
2. **No Access Control** - No file permissions enforcement
3. **No Process Isolation** - Limited process separation
4. **No Network Security** - No network stack
5. **No Cryptography** - No encryption or secure storage
6. **Buffer Overflow Risks** - Limited bounds checking in some areas

**Important**: Do NOT use MiniOS for any security-sensitive applications. This is a learning OS for understanding OS concepts.

## Resolved Issues

For a list of resolved issues, see:
- GitHub closed issues: https://github.com/ahmad-luqman/build-your-own-os/issues?q=is%3Aissue+is%3Aclosed
- Recent accomplishments in CURRENT_STATUS.md

### Recently Fixed (October 2025)
- ✅ Issue #1: Tab Completion Cursor Bug
- ✅ Issue #2: Smoke Test Mount Path
- ✅ Issue #4: SFS File Creation
- ✅ Issue #5: SFS File Write
- ✅ Issue #6: SFS File Read
- ✅ Issue #7: SFS File Deletion
- ✅ Issue #9: SFS Directory Creation (including strncpy infinite loop bug)
- ✅ Issue #10: SFS Directory Deletion
- ✅ Issue #12: Memory Management Audit
- ✅ Issue #15: SFS File Rename/Move
- ✅ Issue #16: Pipe Execution

## Summary

MiniOS has **very few critical issues** remaining:
- ✅ All critical bugs fixed
- ✅ All high-priority features implemented
- ⏳ 5 open issues (all medium/low priority)
- ⏳ Known limitations documented and understood

**The system is stable, functional, and suitable for its intended educational purpose.** 🎉

---

**For the latest issue status, always check**: `./project_dashboard.sh` or https://github.com/ahmad-luqman/build-your-own-os/issues

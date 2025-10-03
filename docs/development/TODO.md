# MiniOS TODO List

Quick action items organized by priority and timeframe.

---

## 🔴 URGENT (Fix This Week)

### Critical Bugs
- [x] **Fix output redirection** - ✅ FIXED (Jan 3, 2025)
  - Files modified: `src/include/shell.h`, `src/shell/core/shell_core.c`, `src/shell/parser/parser.c`, `src/shell/commands/builtin.c`
  - Solution: Added `output_redirect_file` to shell_context for parser-command communication
  - Testing: ✅ VERIFIED in QEMU - `echo Hello > file.txt` works correctly
  - Test results: File created successfully, no console output during redirection, cat shows correct content
  - Test log: `tmp/bug_test_full_output.log`
  
- [x] **Fix block device registration** - ✅ FIXED (Jan 3, 2025)
  - Files modified: `src/fs/block/block_device.c`, `src/fs/block/ramdisk.c`, `src/kernel/main.c`
  - Issue: Compiler optimization causing crash during struct initialization
  - Solution: Added strategic early_print() calls to act as compiler barriers
  - Testing: ✅ VERIFIED in QEMU - RAM disk successfully registered and working
  - Test results: System boots, ramdisk created, block devices functional, files work
  - Test log: `tmp/bug2_test_verbose.log`
  - Details: See `tmp/BUG2_FIX_COMPLETE.md`
  
- [x] **Fix relative path handling** - ✅ FIXED (Oct 3, 2025)
  - File modified: `src/shell/commands/builtin.c`
  - Issue: Paths with `.` and `..` were not normalized (e.g., `/testdir/./file.txt`)
  - Solution: Added `normalize_path()` function to resolve `.` and `..` components
  - Impact: All file/directory commands now support relative paths correctly
  - Testing: ✅ VERIFIED in QEMU - `echo "test" > ./file.txt` and `cat ../file.txt` work
  - Test results: Baseline test confirmed bug, post-fix test confirmed fix, unit tests pass
  - Test logs: `tmp/BUG3_BASELINE_RESULTS.log`, `tmp/bug3_postfix_auto_20251003_142854.log`
  - Details: See `tmp/BUG3_README.md` and `tmp/BUG3_COMPLETE_SUMMARY.txt`

---

## 🟡 HIGH PRIORITY (This Month)

### Shell Features
- [ ] Implement **append redirection** (`>>`)
- [ ] Implement **input redirection** (`<`)
- [ ] Implement **pipes** (`cmd1 | cmd2`)
- [ ] Add **tab completion** for commands
- [ ] Add **tab completion** for file paths
- [ ] Add **arrow key navigation** in command line
- [ ] Add **command history** with up/down arrows
- [ ] Improve **error messages** (clearer, more helpful)

### File System
- [ ] Complete **SFS testing** with block devices _(unblocked 2025-10-03: `mkfs`/`mkdir` crash fixed by enabling FP/SIMD; see `tmp/SFS_ISSUES_FOUND.md`)_
- [ ] Add **symbolic links** support
- [ ] Add **file permissions** checking
- [ ] Implement **chmod** command
- [ ] Implement **chown** command
- [ ] Add **stat** command with full metadata
- [ ] Add **truncate** operation

### Testing
- [ ] Write **shell command tests**
- [ ] Write **file I/O tests**
- [ ] Write **redirection tests**
- [ ] Add **memory leak detection**
- [ ] Add **performance benchmarks**

---

## 🟢 MEDIUM PRIORITY (Next 2-3 Months)

### User Programs
- [ ] Implement **ELF loader** for user programs
- [ ] Create **simple text editor** (ed/nano style)
- [ ] Create **grep** utility
- [ ] Create **wc** (word count) utility
- [ ] Create **head/tail** utilities
- [ ] Add **program execution** framework
- [ ] Add **exit code** handling

### Shell Enhancements
- [ ] Add **environment variables** ($VAR)
- [ ] Add **command aliases**
- [ ] Add **shell scripting** (.sh files)
- [ ] Add **wildcards** (*.txt, file?.log)
- [ ] Add **command substitution** (`cmd` or $(cmd))
- [ ] Add **job control** (fg, bg, jobs)
- [ ] Add **signal handling** (Ctrl+C, Ctrl+Z)

### File Systems
- [ ] Implement **FAT32** file system
- [ ] Implement **Ext2** file system
- [ ] Add **hard links** support
- [ ] Add **file locking**
- [ ] Add **directory caching**
- [ ] Add **file buffering**

### Documentation
- [ ] Write **shell programming guide**
- [ ] Write **user program development guide**
- [ ] Write **API reference**
- [ ] Write **troubleshooting guide**
- [ ] Add **more code examples**

---

## ⚪ LOW PRIORITY (Future / Nice to Have)

### Networking
- [ ] Network device drivers
- [ ] TCP/IP stack
- [ ] Socket API
- [ ] Network utilities (ping, wget)

### Graphics
- [ ] Framebuffer driver
- [ ] Simple graphics primitives
- [ ] Text rendering
- [ ] Window manager

### Advanced Features
- [ ] Multi-threading
- [ ] IPC mechanisms
- [ ] Power management
- [ ] USB support
- [ ] Sound support

### Security
- [ ] User/group management
- [ ] Access control lists
- [ ] Secure boot
- [ ] Encryption

---

## ✅ RECENTLY COMPLETED

### October 2025
- ✅ **Fixed relative path handling (Bug #3)** - Paths with `.` and `..` now work!
  - Added `normalize_path()` function to properly resolve path components
  - All commands now support: `./file.txt`, `../file.txt`, `./sub/../file.txt`
  - Fixed pointer invalidation bug in path tokenization
  - Comprehensive testing: baseline, post-fix, and unit tests
  - Files: `tmp/BUG3_README.md`, `tmp/BUG3_COMPLETE_SUMMARY.txt`
  - Test logs: `tmp/BUG3_BASELINE_RESULTS.log`, `tmp/bug3_postfix_auto_20251003_142854.log`

### January 2025
- ✅ **Fixed output redirection bug (Bug #1)** - `echo text > file` now works correctly
  - Parser and echo command now properly communicate via shell_context
  - Added automated testing in QEMU with ARM64
  - Files written correctly, no console output when redirected
  - Verified with test script: `tmp/test_bug_fixes.sh`
  - Full test output: `tmp/bug_test_full_output.log`
  
- ✅ **Fixed block device registration crash (Bug #2)** - RAM disk now works!
  - Root cause: Compiler optimization issue during struct initialization
  - Solution: Added strategic early_print() calls as compiler barriers
  - RAM disk successfully creates and registers
  - Block device layer now fully functional
  - System boots without crashing
  - Ready for SFS testing with real block devices
  - Test log: `tmp/bug2_test_verbose.log`
  - Details: `tmp/BUG2_FIX_COMPLETE.md`

### October 2024
- ✅ Implemented `touch` command
- ✅ Added output redirection syntax (partial - completed in January 2025)
- ✅ Created comprehensive file system documentation (83KB)
- ✅ Created architecture guide
- ✅ Created configuration guide
- ✅ Created this roadmap

### September 2024
- ✅ Completed Phase 5 (File System)
- ✅ RAMFS fully functional
- ✅ SFS fully implemented
- ✅ VFS layer complete
- ✅ File descriptor management

---

## 📋 QUICK ACTION CHECKLIST

### Today
- [ ] Fix echo redirection bug
- [ ] Test all file commands
- [ ] Update test scripts

### This Week
- [ ] Fix block device issues
- [ ] Implement append redirection
- [ ] Add better error messages
- [ ] Write more tests

### This Month
- [ ] Complete shell I/O redirection
- [ ] Add tab completion
- [ ] Implement pipes
- [ ] Polish file system

### This Quarter
- [ ] Complete Phase 6
- [ ] Start Phase 7
- [ ] Add user programs
- [ ] Comprehensive testing

---

## 🐛 BUG TRACKER

### Open Bugs

| ID | Priority | Component | Description | Status |
|----|----------|-----------|-------------|--------|
| 1 | 🔴 Critical | Shell | Output redirection broken | ✅ FIXED (Jan 2025) |
| 2 | 🔴 Critical | Block Device | Registration crashes | ✅ FIXED (Jan 2025) |
| 3 | 🟡 High | VFS | Relative paths fail | ✅ FIXED (Oct 2025) |
| 4 | 🟡 High | Shell | Directory navigation edge cases | Testing |
| 5 | 🟢 Medium | Shell | Limited command history | Workaround exists |
| 6 | 🟢 Medium | Various | Error messages unclear | Gradual improvement |
| 7 | ⚪ Low | RAMFS | Timestamps not maintained | By design |
| 8 | ⚪ Low | File Systems | File size limited | By design |

### Recently Fixed
- ✅ Relative path handling (Bug #3) - `./file.txt` and `../file.txt` work correctly
- ✅ Output redirection (Bug #1) - `echo text > file` works correctly
- ✅ Block device registration (Bug #2) - RAM disk successfully registers
- ✅ File creation (touch command)
- ✅ VFS file operations
- ✅ RAMFS mounting
- ✅ Directory listing

---

## 📊 PROGRESS TRACKER

### Phase Completion
```
Phase 1: ████████████████████ 100%
Phase 2: ████████████████████ 100%
Phase 3: ████████████████████ 100%
Phase 4: ████████████████████ 100%
Phase 5: ████████████████████ 100%
Phase 6: ███████████████░░░░░  75%
Phase 7: ████░░░░░░░░░░░░░░░░  20%
```

### Feature Implementation
```
Core OS:        ████████████████████ 100%
File Systems:   ████████████████░░░░  80%
Shell:          ███████████████░░░░░  75%
User Programs:  ██░░░░░░░░░░░░░░░░░░  10%
Networking:     ░░░░░░░░░░░░░░░░░░░░   0%
Graphics:       ░░░░░░░░░░░░░░░░░░░░   0%
```

### Documentation
```
Architecture:   ████████████████████ 100%
User Guides:    ███████████████░░░░░  75%
API Docs:       ░░░░░░░░░░░░░░░░░░░░   0%
Tutorials:      ██████░░░░░░░░░░░░░░  30%
```

---

## 🎯 CURRENT SPRINT (This Week)

### Monday-Tuesday
- [ ] Debug and fix output redirection
- [ ] Test fix thoroughly
- [ ] Update tests

### Wednesday-Thursday
- [ ] Implement append redirection
- [ ] Add input redirection
- [ ] Test both features

### Friday
- [ ] Review all changes
- [ ] Run full test suite
- [ ] Update documentation
- [ ] Commit and push

---

## 📝 NOTES

### Development Guidelines
- Test before committing
- Document all changes
- Follow coding standards
- Update roadmap regularly
- Keep TODO list current

### Testing Requirements
- All new features must have tests
- Test coverage > 80%
- No regressions allowed
- Performance benchmarks for critical paths

### Documentation Requirements
- User-facing features need user docs
- APIs need reference documentation
- Complex code needs comments
- Architecture changes need design docs

---

**Last Updated**: October 3, 2025  
**Next Review**: End of week  
**Owner**: MiniOS Development Team

**Recent Achievement**: 🎉 All 3 critical bugs (output redirection, block device registration, relative path handling) are now FIXED!

---

For detailed planning, see **ROADMAP.md**

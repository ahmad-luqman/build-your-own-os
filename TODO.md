# MiniOS TODO List

Quick action items organized by priority and timeframe.

---

## 🔴 URGENT (Fix This Week)

### Critical Bugs
- [ ] **Fix output redirection** - `echo Hello > file.txt` writes to console instead of file
  - File: `src/shell/commands/builtin.c` (cmd_echo function)
  - Issue: Content goes to stdout instead of file
  - Impact: File creation broken
  
- [ ] **Fix block device registration** - RAM disk causes crashes
  - File: `src/fs/block/block_device.c`
  - Issue: block_device_register has issues
  - Impact: SFS cannot be tested
  
- [ ] **Fix relative path handling** - Some paths not resolved correctly
  - File: `src/fs/vfs/vfs_core.c` (vfs_resolve_path)
  - Issue: Relative paths fail in some cases
  - Impact: File operations unreliable

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
- [ ] Complete **SFS testing** with block devices
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

### October 2024
- ✅ Implemented `touch` command
- ✅ Added output redirection syntax (partial)
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
| 1 | 🔴 Critical | Shell | Output redirection broken | In Progress |
| 2 | 🔴 Critical | Block Device | Registration crashes | Identified |
| 3 | 🟡 High | VFS | Relative paths fail | Investigating |
| 4 | 🟡 High | Shell | Directory navigation edge cases | Testing |
| 5 | 🟢 Medium | Shell | Limited command history | Workaround exists |
| 6 | 🟢 Medium | Various | Error messages unclear | Gradual improvement |
| 7 | ⚪ Low | RAMFS | Timestamps not maintained | By design |
| 8 | ⚪ Low | File Systems | File size limited | By design |

### Recently Fixed
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

**Last Updated**: October 2024  
**Next Review**: End of week  
**Owner**: MiniOS Development Team

---

For detailed planning, see **ROADMAP.md**

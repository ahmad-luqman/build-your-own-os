# MiniOS Development Tracker - Project Items

## 🐛 Current Bugs (Priority: High)

### Shell/UI Bugs
1. **Tab Completion Cursor Bug**
   - Issue: `ec[TAB]` produces `ccho` instead of `echo`
   - Location: src/shell/core/shell_io.c
   - Type: Bug
   - Priority: High
   - Labels: bug, shell, phase-6

2. **Smoke Test Mount Path Issue**
   - Issue: Test expects `/ramfs` but RAMFS is mounted at `/`
   - Location: scripts/testing/test_arm64_ramfs_smoke.sh
   - Type: Bug
   - Priority: Medium
   - Labels: bug, testing

## ✨ Phase 6: User Interface Completion

### Shell Features (from SFS_NEXT_STEPS.md)
3. **Enhanced Error Messages**
   - Status: Ready to implement
   - Goal: User-friendly, contextual error messages
   - Examples: "file 'test.txt' not found in '/current/dir'"
   - Priority: Medium
   - Labels: enhancement, shell, phase-6

## 📁 SFS File System Development

### Phase 1: Core File System Operations
4. **File Creation**
   - Fix `sfs_file_create()` crash
   - Implement `touch` command support
   - Add proper inode allocation
   - Priority: High
   - Labels: feature, sfs, phase-5

5. **File Write Operations**
   - Fix `echo "text" > file` crash on SFS
   - Implement safe write operations
   - Add block allocation for file data
   - Priority: High
   - Labels: feature, sfs, phase-5

6. **File Read Operations**
   - Implement `cat` command support for SFS
   - Add file content reading
   - Handle file position/offset
   - Priority: High
   - Labels: feature, sfs, phase-5

7. **File Deletion**
   - Implement `rm` command for SFS
   - Free allocated blocks
   - Update directory entries
   - Priority: Medium
   - Labels: feature, sfs, phase-5

### Directory Operations
8. **Directory Listing for SFS**
   - Fix `ls` command on SFS mounts
   - Parse directory entries correctly
   - Display file metadata
   - Priority: High
   - Labels: feature, sfs, phase-5

9. **Directory Creation**
   - Implement `mkdir` for SFS
   - Allocate directory inodes
   - Initialize directory blocks
   - Priority: Medium
   - Labels: feature, sfs, phase-5

10. **Directory Deletion**
    - Implement `rmdir` for SFS
    - Check for empty directories
    - Handle non-empty directory errors
    - Priority: Medium
    - Labels: feature, sfs, phase-5

## 🔧 Phase 2: Reliability and Robustness

### Error Handling
11. **Comprehensive Error Checking**
    - Add error checking to all SFS operations
    - Implement graceful error recovery
    - Add filesystem consistency checks
    - Priority: Medium
    - Labels: enhancement, reliability, sfs

### Memory Management
12. **Memory Audit**
    - Audit all kmalloc/kfree calls
    - Add memory leak detection
    - Implement block cache management
    - Priority: High
    - Labels: enhancement, memory, reliability

### Data Integrity
13. **Checksums and Integrity**
    - Add checksums for metadata
    - Implement basic journaling
    - Handle power failure scenarios
    - Priority: Low
    - Labels: enhancement, sfs, reliability

## 🚀 Phase 3: Advanced Features

### Large File Support
14. **Indirect Blocks Implementation**
    - Support files > 32KB (current limit)
    - Implement indirect blocks
    - Add double indirect blocks
    - Priority: Low
    - Labels: feature, sfs, phase-7

### File Permissions
15. **Unix-like Permissions**
    - Implement rwx permissions
    - Add owner/group support
    - Implement `chmod` command
    - Priority: Low
    - Labels: feature, sfs, phase-7

## ⚡ Performance Optimization

16. **Caching System**
    - Implement block cache
    - Add inode caching
    - Directory entry caching
    - Priority: Low
    - Labels: enhancement, performance

## 🧪 Testing and Validation

17. **Unit Test Suite**
    - Create comprehensive test suite for SFS
    - Test all filesystem operations
    - Edge case testing
    - Priority: Medium
    - Labels: testing, sfs

18. **Integration Tests**
    - Test with shell commands
    - Test mount/unmount cycles
    - Test filesystem limits
    - Priority: Medium
    - Labels: testing, integration

19. **Stress Tests**
    - Create thousands of files test
    - Test with large files
    - Concurrent access testing
    - Priority: Low
    - Labels: testing, performance

## 📚 Documentation and Tools

20. **SFS Documentation**
    - Design document
    - API documentation
    - Troubleshooting guide
    - Priority: Low
    - Labels: documentation

21. **Filesystem Tools**
    - `fsck.sfs` checker
    - Statistics tool
    - Debug utilities
    - Priority: Low
    - Labels: tools, sfs

## 🔨 Technical Debt

22. **Memory Safety Review**
    - Review all structure assignments
    - Add memory barriers where needed
    - Remove unnecessary volatile qualifiers
    - Priority: Medium
    - Labels: tech-debt, memory

23. **Code Quality Improvements**
    - Refactor large functions
    - Reduce code duplication
    - Improve error messages
    - Priority: Low
    - Labels: tech-debt, code-quality

24. **Architecture Cleanup**
    - Clean up VFS integration
    - Simplify interfaces
    - Remove hardcoded values
    - Priority: Low
    - Labels: tech-debt, architecture

## Project Structure Suggestions

### Columns for GitHub Project Board:
1. **🔴 Urgent** - Current bugs affecting functionality
2. **📋 Backlog** - All unstarted items
3. **🚧 In Progress** - Currently being worked on
4. **👀 In Review** - Completed, needs testing/review
5. **✅ Done** - Completed and tested

### Milestones:
1. **Phase 6 Completion** - Shell features and UI
2. **SFS Core Operations** - Basic file/dir operations
3. **SFS Reliability** - Error handling and robustness
4. **Performance & Advanced** - Optimizations and advanced features
5. **Testing & Documentation** - Comprehensive testing and docs

### Labels to Create:
- `bug` - Something isn't working
- `enhancement` - New feature or request
- `feature` - New functionality
- `shell` - Shell-related issues
- `sfs` - SFS filesystem issues
- `testing` - Testing-related tasks
- `documentation` - Documentation improvements
- `performance` - Performance optimizations
- `tech-debt` - Technical debt items
- `phase-5`, `phase-6`, `phase-7` - Development phases
- `high-priority`, `medium-priority`, `low-priority` - Priority levels
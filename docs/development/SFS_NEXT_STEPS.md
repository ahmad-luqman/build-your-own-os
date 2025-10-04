# SFS (Simple File System) - Next Steps for Development

## Current Status

### ✅ Completed
- [x] Fixed critical stack corruption crash in directory operations
- [x] `cd /sfs` command now works correctly
- [x] Basic directory traversal is functional
- [x] Exception handling with dedicated stack implemented
- [x] Memory barriers added to prevent compiler optimization issues

### ❌ Critical Issues Remaining
- [ ] File creation crashes (`echo > file`, `touch`)
- [ ] File write operations are unstable
- [ ] System crashes after directory traversal

## Immediate Priority (Next 1-2 days)

### 1. Fix File Creation Crash - CRITICAL
**Status**: Crash occurs after `cd /sfs` succeeds
**Location**: Likely in `sfs_file_create()` or file write path
**Symptoms**: Same stack corruption pattern (SP: 0x4009AF68)

**Action Plan**:
1. Add debug output to file creation functions:
   - `sfs_file_create()`
   - `sfs_file_write()`
   - `sfs_alloc_block()`
2. Check for structure assignments needing memory barriers
3. Verify block allocation code
4. Test with -O0 to confirm optimization-related issue

### 2. Complete Basic File Operations
**Files to investigate**:
- `src/fs/sfs/sfs_core.c`:
  - `sfs_file_create()`
  - `sfs_file_open()`
  - `sfs_file_write()`
  - `sfs_file_read()`
  - `sfs_file_close()`

## Phase 1: Core File System Operations (1 week)

### File Operations
- [ ] **File Creation**
  - [ ] Fix `sfs_file_create()` crash
  - [ ] Implement `touch` command support
  - [ ] Add proper inode allocation
  - [ ] Test file creation in root and subdirectories

- [ ] **File Write**
  - [ ] Fix `echo "text" > file` crash
  - [ ] Implement safe write operations
  - [ ] Add block allocation for file data
  - [ ] Handle partial writes and buffering

- [ ] **File Read**
  - [ ] Implement `cat` command support
  - [ ] Add file content reading
  - [ ] Handle file position/offset
  - [ ] Implement read buffering

- [ ] **File Deletion**
  - [ ] Implement `rm` command
  - [ ] Free allocated blocks
  - [ ] Update directory entries
  - [ ] Handle inode cleanup

### Directory Operations
- [ ] **Directory Listing**
  - [ ] Fix `ls` command
  - [ ] Parse directory entries correctly
  - [ ] Display file metadata (size, permissions)
  - [ ] Support `ls -l` for detailed view

- [ ] **Directory Creation**
  - [ ] Implement `mkdir` command
  - [ ] Allocate directory inodes
  - [ ] Initialize directory blocks
  - [ ] Update parent directory

- [ ] **Directory Deletion**
  - [ ] Implement `rmdir` command
  - [ ] Check for empty directories
  - [ ] Recursive deletion option
  - [ ] Handle non-empty directory errors

## Phase 2: Reliability and Robustness (1 week)

### Error Handling
- [ ] Add comprehensive error checking
- [ ] Implement graceful error recovery
- [ ] Add filesystem consistency checks
- [ ] Handle disk full scenarios

### Memory Management
- [ ] Audit all kmalloc/kfree calls
- [ ] Add memory leak detection
- [ ] Implement block cache management
- [ ] Add memory usage statistics

### Data Integrity
- [ ] Add checksums for metadata
- [ ] Implement journaling (optional)
- [ ] Add filesystem repair tool
- [ ] Handle power failure scenarios

## Phase 3: Advanced Features (1-2 weeks)

### Large File Support
- [ ] Implement indirect blocks
- [ ] Support files > 32KB (current limit)
- [ ] Add double indirect blocks
- [ ] Optimize large file operations

### File Permissions
- [ ] Implement Unix-like permissions (rwx)
- [ ] Add owner/group support
- [ ] Implement permission checking
- [ ] Add `chmod` command

### Extended Attributes
- [ ] Support file metadata
- - [ ] Creation/modification times
- - [ ] File type indicators
- - [ ] Custom attributes

## Phase 4: Performance Optimization (1 week)

### Caching
- [ ] Implement block cache
- [ ] Add inode caching
- [ ] Directory entry caching
- [ ] Write-back caching strategy

### Optimization
- [ ] Profile filesystem operations
- [ ] Optimize common paths
- [ ] Reduce system call overhead
- [ ] Improve allocation algorithms

## Phase 5: Testing and Validation (Ongoing)

### Unit Tests
- [ ] Create comprehensive test suite
- [ ] Test all filesystem operations
- [ ] Edge case testing
- [ ] Error condition testing

### Integration Tests
- [ ] Test with shell commands
- [ ] Test with multiple users/processes
- [ ] Test mount/unmount cycles
- [ ] Test filesystem limits

### Stress Tests
- [ ] Create thousands of files
- [ ] Test with large files
- [ ] Concurrent access testing
- [ ] Long-running stability tests

## Phase 6: Documentation and Tools

### Documentation
- [ ] SFS design document
- [ ] API documentation
- [ ] Performance characteristics
- [ ] Troubleshooting guide

### Tools
- [ ] Filesystem checker (`fsck.sfs`)
- [ ] Defragmentation tool
- [ ] Statistics tool (`sfs-stat`)
- [ ] Debug utilities

## Technical Debt to Address

### Memory Safety
- [ ] Review all structure assignments
- [ ] Add memory barriers where needed
- [ ] Remove unnecessary volatile qualifiers
- [ ] Implement bounds checking

### Code Quality
- [ ] Refactor large functions
- [ ] Reduce code duplication
- [ ] Improve error messages
- [ ] Add code comments

### Architecture
- [ ] Clean up VFS integration
- [ ] Simplify interfaces
- [ ] Remove hardcoded values
- [ ] Improve modularity

## Immediate Action Plan (Next 24 hours)

### Day 1: Investigate File Creation Crash
1. Add debug output to identify exact crash location
2. Run with -O0 to confirm optimization issue
3. Add memory barriers to affected functions
4. Test file creation fix

### Day 2: Implement File Operations
1. Fix `sfs_file_write()` if needed
2. Implement basic `cat` command support
3. Test file read/write operations
4. Implement `rm` command

## Files to Modify Immediately

### Critical
1. `src/fs/sfs/sfs_core.c`
   - Add debug to `sfs_file_create()`
   - Check `sfs_file_write()`
   - Review all file operations for memory barriers

2. `src/shell/commands/builtin.c`
   - File creation commands (`touch`, `echo >`)
   - File read commands (`cat`)
   - File deletion commands (`rm`)

### Important
3. `src/fs/vfs/vfs_core.c`
   - VFS file operations interface
   - Path resolution improvements
   - File descriptor management

## Testing Commands for Verification

```bash
# Basic operations
cd /sfs
touch test.txt
echo "Hello" > test.txt
cat test.txt
rm test.txt

# Directory operations
mkdir subdir
cd subdir
touch file.txt
ls -l
cd ..
rmdir subdir

# Stress test
for i in {1..100}; do touch file$i.txt; done
ls
rm *.txt
```

## Success Criteria

### Minimum Viable Product (1 week)
- [ ] Can create, read, write, and delete files
- [ ] Can create and remove directories
- [ ] Can list directory contents
- [ ] Basic shell commands work with SFS

### Full Feature Set (1 month)
- [ ] All file operations robust
- [ ] Large file support (>1MB)
- [ ] File permissions implemented
- [ ] Performance optimized
- [ ] Comprehensive testing suite

## Risks and Mitigations

### Technical Risks
1. **Memory corruption**: Add extensive testing, use sanitizers
2. **Performance issues**: Profile early, optimize bottlenecks
3. **Data loss**: Implement journaling/backup strategies

### Timeline Risks
1. **Feature creep**: Stick to core features first
2. **Technical debt**: Address as discovered, document decisions
3. **Integration issues**: Test with VFS layer continuously

## Resources Needed

### Development
- [ ] QEMU/UTM for testing
- [ ] ARM64 cross-compiler toolchain
- [ ] Debug environment (if available)

### Testing
- [ ] Various file sizes for testing
- [ ] Disk images for persistence testing
- [ ] Test automation scripts

## Notes

### Memory Barrier Patterns
Use these patterns in SFS code:

```c
// Before memory operation
__asm__ volatile("dmb ish" ::: "memory");

// Memory operation
memcpy(dest, src, size);

// After memory operation
__asm__ volatile("dmb ish" ::: "memory");
```

### Debug Output Pattern
```c
#ifdef DEBUG_SFS
early_print("DEBUG: Function entry\n");
// ... function code
early_print("DEBUG: Function exit\n");
#endif
```

---

*Last Updated: 2025-10-03*
*Priority: Fix file creation crash immediately*
*Owner: SFS Development Team*
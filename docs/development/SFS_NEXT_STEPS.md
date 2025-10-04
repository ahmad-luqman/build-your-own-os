# SFS (Simple File System) - Next Steps for Development

## Current Status

### ✅ Completed
- [x] Fixed critical SIMD vectorization crash (PC: 0x600003C5)
- [x] kmalloc now returns 16-byte aligned addresses
- [x] Anti-vectorization flags prevent SIMD generation
- [x] Exception handling reports accurate PC/SP values
- [x] System boots through file system initialization
- [x] Comprehensive test infrastructure created

### ❌ Critical Issues Remaining
- [ ] System hangs at RAM disk creation (during SFS initialization)
- [ ] RAM disk allocation issue (size calculation)
- [x] Directory traversal now works (no longer crashes after `cd /sfs`)
- [x] SIMD vectorization crash fixed

## Immediate Priority (Next 1-2 days)

### 1. Fix RAM Disk Creation Hang - CRITICAL
**Status**: System hangs after "Creating RAM disk: ramdisk0 (4MB)"
**Location**: `src/fs/block/ramdisk.c` - `ramdisk_create()` function
**Symptoms**: System hangs during block allocation loop

**Action Plan**:
1. Investigate RAM disk size calculation (4MB vs 32KB actual allocation)
2. Add debug output to `ramdisk_create()` and memory allocation
3. Check if memory allocation is causing the hang
4. Verify block device registration logic
5. Test with smaller RAM disk size
6. Ensure proper error handling in allocation

### 2. Resume File Operations Testing
**Status**: Blocked by RAM disk creation
**Next Steps**:
1. After fixing RAM disk, test file creation commands
2. Verify `echo "text" > file` works
3. Test `touch` command
4. Implement `cat` for file reading
5. Test file persistence across mount/unmount

### 3. Complete Basic File Operations
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

### Day 1: Fix RAM Disk Creation
1. Investigate size discrepancy (4MB vs 32KB)
2. Add debug output to `ramdisk_create()`
3. Check memory allocation loop
4. Test with smaller RAM disk sizes
5. Fix block allocation logic

### Day 2: Test File Operations
1. Verify RAM disk creation works
2. Test file creation commands
3. Verify SFS formatting works
4. Test basic file operations
5. Implement `cat` command support

## Files to Modify Immediately

### Critical
1. `src/fs/block/ramdisk.c`
   - Investigate `ramdisk_create()` function
   - Fix size calculation (4MB vs 32KB)
   - Add debug output to allocation loops

2. `src/fs/sfs/sfs_core.c`
   - Prepare for file operation testing
   - Ensure memory barriers are in place
   - Review file creation and write functions

3. `src/shell/commands/builtin.c`
   - Prepare file operation commands
   - File creation commands (`touch`, `echo >`)
   - File read commands (`cat`)
   - File deletion commands (`rm`)

### Important
4. `src/fs/vfs/vfs_core.c`
   - VFS file operations interface
   - Path resolution improvements
   - File descriptor management

## Testing Commands for Verification

### First (after RAM disk fix)
```bash
# Verify RAM disk creation works
help
sysinfo
# Should see RAM disk listed

# Basic boot verification
echo "System boot successful"
exit
```

### Once RAM disk works
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

*Last Updated: 2025-10-04*
*Priority: Fix RAM disk creation hang immediately*
*Owner: SFS Development Team*
*Note: SIMD vectorization crash (PC: 0x600003C5) has been fixed!*
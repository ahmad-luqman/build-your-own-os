# SFS Fix Plan: Achieving Parity with RAMFS

**Goal:** Bring SFS to 100% feature parity with RAMFS
**Current Status:** 70% complete (9/14 tests passing)
**Target:** All core operations working reliably

---

## Phase 1: Fix Critical Bugs (Priority 1)

### 🔴 Task 1.1: Fix Nested Directory Creation Hang

**Issue:** #9 - System hangs when creating nested directories

**Current Behavior:**
```bash
mkdir /sfs/testdir           # ✅ Works
mkdir /sfs/testdir/subdir    # ❌ Hangs (infinite loop)
```

**Investigation Steps:**

1. **Add Debug Logging**
   ```c
   // In sfs_create_directory() at src/fs/sfs/sfs_core.c:1326
   int sfs_create_directory(struct file_system *fs, const char *path, uint32_t mode)
   {
       early_print("sfs_create_directory: Entry, path=");
       early_print(path);
       early_print("\n");

       // ... existing code ...

       early_print("sfs_create_directory: Extracting parent\n");
       int result = sfs_extract_parent(fs, path, &parent, name);
       early_print("sfs_create_directory: Parent extracted, result=");
       // print result

       // ... rest of function ...
   }
   ```

2. **Check `sfs_extract_parent()` Function**
   - Look for infinite loops
   - Check path parsing logic
   - Verify parent directory lookup

3. **Test Cases** (in order):
   ```bash
   # Test 1: Single level (already works)
   mkdir /sfs/test1

   # Test 2: Two levels
   mkdir /sfs/test1/sub1

   # Test 3: Three levels
   mkdir /sfs/test1/sub1/deep1
   ```

4. **Likely Fixes:**
   - Add proper termination condition in path walking
   - Fix parent directory inode lookup
   - Handle "/" correctly as root directory

**Success Criteria:**
- ✅ Can create nested directories 3+ levels deep
- ✅ No hangs or infinite loops
- ✅ Parent directories correctly resolved

**Estimated Time:** 1-2 hours

---

### 🔴 Task 1.2: Implement/Fix File Rename Operation

**Issue:** #15 - `mv` command fails on SFS

**Current Behavior:**
```bash
mv /sfs/file1.txt /sfs/file1_renamed.txt
# Error: Failed to move/rename file: Error: /sfs/file1.txtError:
```

**Investigation Steps:**

1. **Check VFS Rename Implementation**
   ```bash
   grep -A 30 "int vfs_rename" src/fs/vfs/vfs_core.c
   ```
   - Understand how VFS handles rename
   - Check if it calls SFS-specific function

2. **Check if SFS Has Rename**
   ```bash
   grep "sfs_rename\|sfs_file_rename" src/fs/sfs/sfs_core.c
   ```

3. **Implementation Options:**

   **Option A: Direct Rename** (if supported)
   ```c
   int sfs_rename(struct file_system *fs, const char *oldpath, const char *newpath)
   {
       // 1. Resolve old inode
       // 2. Update directory entry name
       // 3. Update parent if moving between directories
       // 4. Update inode metadata (mod time)
       // 5. Sync to disk
   }
   ```

   **Option B: Copy + Delete** (fallback)
   ```c
   int sfs_rename(struct file_system *fs, const char *oldpath, const char *newpath)
   {
       // 1. Copy file content to new location
       // 2. Delete old file
       // 3. More expensive but guaranteed to work
   }
   ```

4. **Hook into VFS**
   ```c
   // In sfs_core.c, update file_system_type
   struct file_system_type sfs_fs_type = {
       .name = "sfs",
       .mount = sfs_mount,
       .unmount = sfs_unmount,
       .format = sfs_format,
       .rename = sfs_rename,  // ADD THIS
       .file_ops = &sfs_file_ops,
       .dir_ops = &sfs_dir_ops
   };
   ```

**Test Cases:**
```bash
# Test 1: Rename in same directory
mv /sfs/test.txt /sfs/renamed.txt

# Test 2: Move to subdirectory
mv /sfs/file.txt /sfs/subdir/file.txt

# Test 3: Move to parent directory
mv /sfs/subdir/file.txt /sfs/file.txt

# Test 4: Rename and move
mv /sfs/old.txt /sfs/newdir/new.txt
```

**Success Criteria:**
- ✅ Rename within same directory works
- ✅ Move between directories works
- ✅ File content preserved
- ✅ Old file removed

**Estimated Time:** 2-3 hours

---

## Phase 2: Complete Core Operations (Priority 2)

### 🟡 Task 2.1: Test and Fix File Deletion

**Issue:** #7 - File deletion not tested yet

**Implementation Check:**
```bash
grep -n "sfs_file_delete\|sfs_remove\|sfs_unlink" src/fs/sfs/sfs_core.c
```

**If Missing, Implement:**
```c
int sfs_file_delete(struct file_system *fs, const char *path)
{
    // 1. Resolve file inode
    // 2. Check no processes have file open
    // 3. Free all data blocks
    // 4. Free inode
    // 5. Remove directory entry
    // 6. Update parent directory
    // 7. Sync to disk
}
```

**Test Cases:**
```bash
# Create test files
echo "test" > /sfs/delete_me.txt
echo "keep" > /sfs/keep_me.txt

# Test deletion
rm /sfs/delete_me.txt

# Verify
ls /sfs  # Should show only keep_me.txt
cat /sfs/keep_me.txt  # Should still work
```

**Success Criteria:**
- ✅ File deleted from directory listing
- ✅ Inode freed and available for reuse
- ✅ Data blocks freed
- ✅ Other files unaffected

**Estimated Time:** 2 hours

---

### 🟡 Task 2.2: Test and Fix Directory Deletion

**Issue:** #10 - Directory deletion not tested yet

**Test Cases:**
```bash
# Test 1: Empty directory
mkdir /sfs/empty_dir
rmdir /sfs/empty_dir  # Should work

# Test 2: Non-empty directory (should fail)
mkdir /sfs/full_dir
touch /sfs/full_dir/file.txt
rmdir /sfs/full_dir  # Should fail with error

# Test 3: Recursive delete (if supported)
rm -rf /sfs/full_dir  # Should delete dir and contents
```

**Implementation:**
```c
int sfs_dir_remove(struct file_system *fs, const char *path)
{
    // 1. Resolve directory inode
    // 2. Check directory is empty
    // 3. Remove directory entry from parent
    // 4. Free directory inode
    // 5. Sync to disk
}
```

**Success Criteria:**
- ✅ Can remove empty directories
- ✅ Refuses to remove non-empty directories
- ✅ Parent directory updated correctly

**Estimated Time:** 1-2 hours

---

### 🟡 Task 2.3: Test Persistence Across Remount

**Test Script:**
```bash
# Write test data
echo "Persistence test" > /sfs/persist.txt
echo "Multi-line" > /sfs/multi.txt
echo "data" >> /sfs/multi.txt
mkdir /sfs/testdir
echo "nested" > /sfs/testdir/nested.txt

# List before unmount
ls /sfs
cat /sfs/persist.txt

# Unmount
cd /
umount /sfs

# Remount
mount ramdisk0 /sfs sfs

# Verify all data present
ls /sfs  # Should show all files
cat /sfs/persist.txt  # Should show "Persistence test"
cat /sfs/multi.txt  # Should show both lines
cat /sfs/testdir/nested.txt  # Should show "nested"
```

**Potential Issues to Check:**
1. Superblock updates on unmount
2. Dirty data not flushed
3. Cache invalidation on remount

**Success Criteria:**
- ✅ All files present after remount
- ✅ File contents unchanged
- ✅ Directory structure intact

**Estimated Time:** 1 hour (if working), 3 hours (if bugs found)

---

## Phase 3: Advanced Features (Priority 3)

### 🟢 Task 3.1: Enhance Directory Listing

**Issue:** #8 - Directory listing works but may need enhancements

**Test Cases:**
```bash
# Test 1: Basic ls (already works)
ls /sfs

# Test 2: Long listing
ls -l /sfs  # Should show permissions, size, etc.

# Test 3: Recursive listing
ls -R /sfs  # Should show nested directories

# Test 4: Hidden files (if supported)
ls -a /sfs
```

**Enhancements:**
- Add file size to ls output
- Add timestamps
- Add permissions display
- Improve formatting

**Estimated Time:** 2 hours

---

### 🟢 Task 3.2: Cross-Filesystem Operations

**Test Cases:**
```bash
# RAMFS to SFS
cp /tmp/ramfs_file.txt /sfs/from_ramfs.txt

# SFS to RAMFS
cp /sfs/sfs_file.txt /tmp/from_sfs.txt

# Verify both directions work
cat /sfs/from_ramfs.txt
cat /tmp/from_sfs.txt
```

**Success Criteria:**
- ✅ Copy from RAMFS to SFS works
- ✅ Copy from SFS to RAMFS works
- ✅ Content preserved

**Estimated Time:** 1 hour (likely already works)

---

## Phase 4: Testing and Validation (Priority 4)

### 🔵 Task 4.1: Create Comprehensive Test Suite

**File:** `tests/test_sfs_operations.c`

```c
// Unit tests for SFS operations
void test_sfs_file_create();
void test_sfs_file_write();
void test_sfs_file_read();
void test_sfs_file_delete();
void test_sfs_file_rename();
void test_sfs_directory_create();
void test_sfs_directory_delete();
void test_sfs_persistence();
void test_sfs_stress();  // Create 100+ files
```

**Estimated Time:** 4 hours

---

### 🔵 Task 4.2: Stress Testing

**Test Script:** `scripts/testing/test_sfs_stress.sh`

```bash
# Create many files
for i in {1..100}; do
    echo "File $i" > /sfs/file_$i.txt
done

# Verify all created
ls /sfs | wc -l  # Should be 100

# Read all files
for i in {1..100}; do
    cat /sfs/file_$i.txt > /dev/null
done

# Delete half
for i in {1..50}; do
    rm /sfs/file_$i.txt
done

# Verify deletion
ls /sfs | wc -l  # Should be 50
```

**Success Criteria:**
- ✅ Can create 100+ files
- ✅ No memory leaks
- ✅ No crashes
- ✅ Performance acceptable

**Estimated Time:** 2 hours

---

## Implementation Priority

### Week 1: Critical Fixes
- **Day 1-2:** Fix nested mkdir hang (Task 1.1)
- **Day 3-4:** Implement file rename (Task 1.2)
- **Day 5:** Test file deletion (Task 2.1)

### Week 2: Core Completeness
- **Day 1:** Test directory deletion (Task 2.2)
- **Day 2:** Test persistence (Task 2.3)
- **Day 3-4:** Enhance directory listing (Task 3.1)
- **Day 5:** Cross-FS operations (Task 3.2)

### Week 3: Testing and Polish
- **Day 1-3:** Unit tests (Task 4.1)
- **Day 4-5:** Stress testing (Task 4.2)

---

## Success Metrics

| Metric | Current | Target |
|--------|---------|--------|
| Test Pass Rate | 64% (9/14) | 100% (14/14) |
| Features Working | 70% | 100% |
| Critical Bugs | 2 | 0 |
| Issues Closed | 3/9 | 9/9 |
| RAMFS Parity | ~70% | 100% |

---

## Risk Mitigation

### Risk 1: Nested mkdir hang hard to fix
**Mitigation:** Add extensive logging, create minimal test case, debug step-by-step

### Risk 2: Data corruption during testing
**Mitigation:** Always test on ramdisk, never on real storage; backup test data

### Risk 3: Performance issues with many files
**Mitigation:** Implement proper caching, optimize block access patterns

---

## Development Guidelines

1. **Always test in QEMU first** - Don't commit untested code
2. **Add debug logging** - Every function should have entry/exit logs
3. **Write tests for each fix** - Prevent regressions
4. **Update documentation** - Keep CLAUDE.md and issue comments current
5. **Commit frequently** - Small, focused commits with clear messages

---

## Final Checklist

Before closing each issue, verify:
- ✅ Test passes in QEMU
- ✅ No regressions in other tests
- ✅ Code reviewed for edge cases
- ✅ Documentation updated
- ✅ Issue comment shows test evidence

---

## Contacts & Resources

- **Test Scripts:** `scripts/testing/test_sfs_*.sh`
- **SFS Implementation:** `src/fs/sfs/sfs_core.c`
- **VFS Layer:** `src/fs/vfs/vfs_core.c`
- **GitHub Project:** https://github.com/users/ahmad-luqman/projects/9
- **Analysis Doc:** `SFS_TEST_ANALYSIS.md`

---

**Next Immediate Action:** Start with Task 1.1 (Fix nested mkdir hang) as it's blocking all other directory tests.

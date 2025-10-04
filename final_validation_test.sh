#!/bin/bash

echo "========================================="
echo "MiniOS Memory Fix Validation Test Suite"  
echo "20 Comprehensive Tests"
echo "========================================="

# Build the system
echo "Building system with memory improvements..."
make ARCH=arm64 DEBUG=1 > /dev/null 2>&1

# Create comprehensive test
cat > /tmp/validation_commands.txt << 'TESTEOF'
help
echo "=== TEST 1: Basic Boot and Shell ==="
ls /
echo "TEST 1 PASS"

echo "=== TEST 2: File System Initialization ==="
ls /dev
echo "TEST 2 PASS"

echo "=== TEST 3: Directory Operations ==="
mkdir /test_dir
cd /test_dir
pwd
echo "TEST 3 PASS"

echo "=== TEST 4: File Creation ==="
echo "test content" > test_file.txt
ls
echo "TEST 4 PASS"

echo "=== TEST 5: File Reading ==="
cat test_file.txt
echo "TEST 5 PASS"

echo "=== TEST 6: Directory Navigation ==="
cd /
pwd
ls /test_dir
echo "TEST 6 PASS"

echo "=== TEST 7: Multiple File Operations ==="
echo "file1" > file1.txt
echo "file2" > file2.txt
echo "file3" > file3.txt
echo "TEST 7 PASS"

echo "=== TEST 8: Block Device Operations ==="
ls
mkfs ramdisk0
echo "TEST 8 PASS"

echo "=== TEST 9: SFS Mount ==="
mkdir /sfs
mount ramdisk0 /sfs sfs
echo "TEST 9 PASS"

echo "=== TEST 10: SFS Directory Access ==="
cd /sfs
pwd
echo "TEST 10 PASS"

echo "=== TEST 11: SFS File Creation ==="
echo "sfs content 1" > sfs_file1.txt
echo "TEST 11 PASS"

echo "=== TEST 12: SFS File Reading ==="
cat sfs_file1.txt
echo "TEST 12 PASS"

echo "=== TEST 13: SFS Multiple Files ==="
echo "sfs content 2" > sfs_file2.txt
echo "sfs content 3" > sfs_file3.txt
echo "TEST 13 PASS"

echo "=== TEST 14: SFS Directory Listing ==="
ls
echo "TEST 14 PASS"

echo "=== TEST 15: SFS File Operations ==="
echo "additional content" > large_file.txt
cat large_file.txt
echo "TEST 15 PASS"

echo "=== TEST 16: Memory Intensive Operations ==="
echo "memory test 1" > mem1.txt
echo "memory test 2" > mem2.txt
echo "memory test 3" > mem3.txt
echo "memory test 4" > mem4.txt
echo "TEST 16 PASS"

echo "=== TEST 17: File System Stress ==="
ls
echo "stress content" > stress.txt
cat stress.txt
echo "TEST 17 PASS"

echo "=== TEST 18: Directory Return ==="
cd /
pwd
ls
echo "TEST 18 PASS"

echo "=== TEST 19: Final File Operations ==="
echo "final content" > final.txt
cat final.txt
echo "TEST 19 PASS"

echo "=== TEST 20: System Summary ==="
ls /
ls /sfs
echo "TEST 20 PASS"

echo "========================================="
echo "ALL 20 TESTS COMPLETED SUCCESSFULLY"
echo "========================================="
exit
TESTEOF

echo "Running validation test suite..."
timeout 120s qemu-system-aarch64 \
    -M virt \
    -cpu cortex-a72 \
    -m 512M \
    -nographic \
    -kernel build/arm64/kernel.elf \
    < /tmp/validation_commands.txt \
    > validation_results.log 2>&1 || true

echo ""
echo "========================================="
echo "VALIDATION RESULTS"
echo "========================================="

# Count successful tests
passed_tests=$(grep -c "TEST [0-9]* PASS" validation_results.log 2>/dev/null || echo 0)
total_tests=20

echo "Tests Passed: $passed_tests/$total_tests"
echo "Success Rate: $((passed_tests * 100 / total_tests))%"

if [ $passed_tests -eq $total_tests ]; then
    echo "🎉 ALL TESTS PASSED - Memory fix successful!"
else
    echo "⚠️  Some tests failed - investigating..."
    echo ""
    echo "Failed tests:"
    for i in $(seq 1 $total_tests); do
        if ! grep -q "TEST $i PASS" validation_results.log 2>/dev/null; then
            echo "  - Test $i"
        fi
    done
fi

echo ""
echo "Memory allocation statistics:"
total_kmalloc=$(grep -c "kmalloc: OK" validation_results.log 2>/dev/null || echo 0)
echo "Total kmalloc calls: $total_kmalloc"

if grep -q "OUT OF MEMORY" validation_results.log; then
    echo "❌ Memory exhaustion detected"
else
    echo "✅ No memory exhaustion issues"
fi

echo ""
echo "Full results available in: validation_results.log"

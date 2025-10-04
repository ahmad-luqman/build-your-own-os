#!/bin/bash

echo "=== MiniOS Memory Exhaustion Test ==="
echo "Building system..."
make ARCH=arm64 DEBUG=1 > /dev/null 2>&1

echo "Creating comprehensive test script..."
cat > /tmp/memory_test_commands.txt << 'TESTEOF'
help
echo "=== Test 1: Basic Operations ==="
ls /
mkdir /test1
echo "Basic operations done"

echo "=== Test 2: File Creation Stress ==="
echo "file1" > /test1/file1.txt
echo "file2" > /test1/file2.txt
echo "file3" > /test1/file3.txt
echo "File creation stress done"

echo "=== Test 3: Directory Operations ==="
mkdir /test2
mkdir /test3
mkdir /test4
cd /test1
pwd
cd /
echo "Directory operations done"

echo "=== Test 4: SFS Operations ==="
mkfs ramdisk0
mkdir /sfs
mount ramdisk0 /sfs sfs
cd /sfs
echo "SFS mounted and accessed"

echo "=== Test 5: File System Stress ==="
echo "sfsfile1" > file1.sfs
echo "sfsfile2" > file2.sfs
echo "sfsfile3" > file3.sfs
ls
cat file1.sfs
echo "SFS file operations done"

echo "=== Test 6-20: Memory Intensive Operations ==="
i=6
while [ $i -le 20 ]; do
  echo "=== Test $i: Creating file$i ==="
  echo "Content for file$i with some longer text to use more memory" > "file$i.txt"
  ls > "listing$i.txt" 
  echo "Test $i completed"
  i=$((i+1))
done

echo "All 20 tests completed"
exit
TESTEOF

echo "Running memory exhaustion test..."
timeout 90s qemu-system-aarch64 \
    -M virt \
    -cpu cortex-a72 \
    -m 512M \
    -nographic \
    -kernel build/arm64/kernel.elf \
    < /tmp/memory_test_commands.txt \
    > memory_exhaustion_output.log 2>&1 || true

echo "Analyzing memory usage..."
echo "Total kmalloc calls:"
grep -c "kmalloc: OK" memory_exhaustion_output.log 2>/dev/null || echo "0"
echo ""
echo "Memory exhaustion errors:"
grep -n "OUT OF MEMORY\|memory.*exhaust\|failed.*alloc" memory_exhaustion_output.log || echo "None found in main output"
echo ""
echo "Last successful operations:"
tail -30 memory_exhaustion_output.log

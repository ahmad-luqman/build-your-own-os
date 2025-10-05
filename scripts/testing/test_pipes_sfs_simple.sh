#!/bin/bash
set -euo pipefail

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
COMMANDS_FILE="tmp/pipe_sfs_commands_${TIMESTAMP}.txt"
LOG_FILE="tmp/pipe_sfs_${TIMESTAMP}.log"

# Create test commands for SFS pipe testing
cat > "$COMMANDS_FILE" <<'CMDS'
help
echo "=== Pipe Testing on SFS ==="

echo "RAMFS pipe test"
echo "hello from RAMFS" | cat

echo "Setting up SFS"
mkdir /sfs
mkfs ramdisk0
mount ramdisk0 /sfs sfs

echo "SFS pipe test"
cd /sfs
echo "hello from SFS" | cat

echo "Cross-FS pipe test"
echo "cross-FS content" | cat
ls /sfs | cat

echo "Cleanup and exit"
cd /
umount /sfs
exit
CMDS

printf 'Running SFS pipe test, logging to %s\n' "$LOG_FILE"

# Run the test using the same approach as working SFS tests
set +e
timeout 45 qemu-system-aarch64 \
    -M virt \
    -cpu cortex-a72 \
    -m 512M \
    -nographic \
    -kernel build/arm64/kernel.elf \
    -serial mon:stdio \
    < "$COMMANDS_FILE" 2>&1 | tee "$LOG_FILE"
status=${PIPESTATUS[0]}
set -e

if [ "$status" -ne 0 ]; then
    if [ "$status" -eq 124 ] && grep -q 'Shell exited' "$LOG_FILE"; then
        printf 'QEMU timeout after shell exit; treating as success. See %s\n' "$LOG_FILE"
    else
        printf 'Test completed with failure; see %s for details\n' "$LOG_FILE"
        exit 1
    fi
else
    printf 'Test completed; see %s for details\n' "$LOG_FILE"
fi

# Analyze results
echo ""
echo "========================================="
echo "SFS Pipe Test Results"
echo "========================================="

if grep -q "hello from RAMFS" "$LOG_FILE"; then
    echo "✅ RAMFS pipe works"
else
    echo "❌ RAMFS pipe failed"
fi

if grep -q "Mounted file system: sfs" "$LOG_FILE"; then
    echo "✅ SFS mounted successfully"
else
    echo "❌ SFS mount failed"
fi

if grep -q "hello from SFS" "$LOG_FILE"; then
    echo "✅ SFS pipe works"
else
    echo "❌ SFS pipe failed"
fi

if grep -q "cross-FS content" "$LOG_FILE"; then
    echo "✅ Cross-FS pipe works"
else
    echo "❌ Cross-FS pipe failed"
fi

echo ""
echo "Sample output:"
grep -E "(hello from|Mounted|cross-FS)" "$LOG_FILE" || echo "No sample output found"
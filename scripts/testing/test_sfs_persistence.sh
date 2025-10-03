#!/bin/bash
set -euo pipefail

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
COMMANDS_FILE="tmp/sfs_persistence_commands_${TIMESTAMP}.txt"
LOG_FILE="tmp/sfs_persistence_${TIMESTAMP}.log"

cat > "$COMMANDS_FILE" <<'CMDS'
help
mkdir /sfs
mkfs ramdisk0 sfs
mount ramdisk0 /sfs sfs
cd /sfs
echo "Persistence Test" > test.txt
cat test.txt
cd /
umount /sfs
mount ramdisk0 /sfs sfs
cat /sfs/test.txt
exit
CMDS

printf 'Running SFS persistence test, logging to %s\n' "$LOG_FILE"

set +e
timeout 30 qemu-system-aarch64 \
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

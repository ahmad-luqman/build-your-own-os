#!/usr/bin/env bash
#
# SFS mount / write / copy / remount scenario on x86-64 and ARM64 (issue #27).
#
# Boots each kernel in QEMU, types the scenario into the serial console, and
# checks that:
#   - the boot-time SFS format completed and the shell started
#   - the file written before umount reads back as "persist" after remount
#   - `ls /sfs/a` lists both f.txt and g.txt
#   - no fatal CPU exceptions were taken (x86: #PF/#DF, ARM64: aborts/undef)
#
# Usage:
#   scripts/testing/test_sfs_both_arches.sh [x86_64|arm64|all]   (default: all)
#
# Prerequisites (from the repo root):
#   make ARCH=x86_64 kernel && ./tools/create-grub-iso.sh
#   make ARCH=arm64 kernel
#
# Tunables (environment): BOOT_WAIT_X86, BOOT_WAIT_ARM, CMD_DELAY, QEMU_TIMEOUT,
# LOG_DIR.
#
# Must run under bash: the command list relies on bash arrays. zsh word
# splitting mangles the same loop when written inline.

set -u

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
LOG_DIR=${LOG_DIR:-$ROOT/build/test-logs}
BOOT_WAIT_X86=${BOOT_WAIT_X86:-8}
BOOT_WAIT_ARM=${BOOT_WAIT_ARM:-5}
CMD_DELAY=${CMD_DELAY:-0.8}
QEMU_TIMEOUT=${QEMU_TIMEOUT:-60}

COMMANDS=(
    "mkdir /sfs"
    "mount ramdisk0 /sfs sfs"
    "mkdir /sfs/a"
    "echo persist > /sfs/a/f.txt"
    "cp /sfs/a/f.txt /sfs/a/g.txt"
    "umount /sfs"
    "mount ramdisk0 /sfs sfs"
    "cat /sfs/a/f.txt"
    "ls /sfs/a"
)

mkdir -p "$LOG_DIR"

# Feed the scenario to QEMU's stdin, then quit with Ctrl-A x.
feed_commands() {
    local boot_wait=$1
    sleep "$boot_wait"
    local c
    for c in "${COMMANDS[@]}"; do
        printf '%s\r' "$c"
        sleep "$CMD_DELAY"
    done
    sleep 1
    printf '\001x'
}

# Print the console output that follows the LAST echo of a given command,
# with CRs and ANSI escape sequences removed.
output_after() {
    local log=$1 cmd=$2
    tr -d '\r' < "$log" | sed $'s/\x1b\\[[0-9;]*[A-Za-z]//g' |
        awk -v cmd="$cmd" '
            index($0, cmd) { buf = ""; found = 1; next }
            found          { buf = buf $0 "\n" }
            END            { printf "%s", buf }'
}

run_arch() {
    local arch=$1
    local log="$LOG_DIR/sfs_${arch}_serial.log"
    local qlog="$LOG_DIR/sfs_${arch}_qemu.log"
    local boot_wait
    local -a qemu

    case "$arch" in
        x86_64)
            local iso="$ROOT/build/x86_64/minios.iso"
            [ -f "$iso" ] || { echo "[$arch] missing $iso (run: make ARCH=x86_64 kernel && ./tools/create-grub-iso.sh)"; return 1; }
            qemu=(qemu-system-x86_64 -m 512M -cdrom "$iso" -boot d)
            boot_wait=$BOOT_WAIT_X86
            ;;
        arm64)
            local elf="$ROOT/build/arm64/kernel.elf"
            [ -f "$elf" ] || { echo "[$arch] missing $elf (run: make ARCH=arm64 kernel)"; return 1; }
            qemu=(qemu-system-aarch64 -machine virt -cpu cortex-a72 -m 512M -kernel "$elf")
            boot_wait=$BOOT_WAIT_ARM
            ;;
        *)
            echo "unknown arch: $arch"; return 1 ;;
    esac
    qemu+=(-nographic -serial mon:stdio -no-reboot -d int,cpu_reset -D "$qlog")

    echo "[$arch] booting: ${qemu[*]}"
    rm -f "$log" "$qlog"

    feed_commands "$boot_wait" | "${qemu[@]}" > "$log" 2>&1 &
    local qpid=$!
    # macOS has no `timeout`; kill QEMU if it outlives the scenario.
    ( sleep "$QEMU_TIMEOUT"; kill "$qpid" 2>/dev/null ) &
    local wpid=$!
    wait "$qpid" 2>/dev/null
    kill "$wpid" 2>/dev/null
    wait "$wpid" 2>/dev/null

    local fail=0
    check() {  # check <description> <command...>
        local desc=$1; shift
        if "$@"; then
            echo "  PASS  $desc"
        else
            echo "  FAIL  $desc"
            fail=1
        fi
    }

    check "SFS format complete at boot" grep -aq "SFS format complete" "$log"
    check "shell started (MiniOS Shell v1.0)" grep -aq "MiniOS Shell v1.0" "$log"
    check "cat after remount prints 'persist'" \
        bash -c 'grep -qx "persist" <<< "$1"' _ "$(output_after "$log" "cat /sfs/a/f.txt")"
    local ls_out
    ls_out=$(output_after "$log" "ls /sfs/a")
    check "ls /sfs/a lists f.txt" grep -q "f\.txt" <<< "$ls_out"
    check "ls /sfs/a lists g.txt" grep -q "g\.txt" <<< "$ls_out"

    local exc
    if [ "$arch" = x86_64 ]; then
        # grep -c prints 0 but exits 1 on no match; the count is what matters.
        exc=$(grep -c "v=0e\|v=08" "$qlog" 2>/dev/null || true)
        check "no #PF/#DF in QEMU log (count=${exc:-0})" [ "${exc:-0}" -eq 0 ]
    else
        exc=$(grep -c "Taking exception \(1\|3\|4\) " "$qlog" 2>/dev/null || true)
        check "no UNDEF/prefetch/data abort in QEMU log (count=${exc:-0})" [ "${exc:-0}" -eq 0 ]
    fi

    echo "  logs: $log"
    echo "        $qlog"
    return $fail
}

target=${1:-all}
case "$target" in
    all)          arches=(x86_64 arm64) ;;
    x86_64|arm64) arches=("$target") ;;
    *) echo "usage: $0 [x86_64|arm64|all]"; exit 2 ;;
esac

overall=0
for a in "${arches[@]}"; do
    run_arch "$a" || overall=1
done

if [ $overall -eq 0 ]; then
    echo "RESULT: PASS (${arches[*]})"
else
    echo "RESULT: FAIL"
fi
exit $overall

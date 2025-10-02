#!/bin/bash
# RAMFS + Shell integration test

set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
TIMEOUT_SECONDS=${TIMEOUT_SECONDS:-90}
BOOT_WAIT=${BOOT_WAIT:-6}
COMMAND_DELAY=${COMMAND_DELAY:-2}
POST_EXIT_WAIT=${POST_EXIT_WAIT:-3}

ARCH_LIST=("${@}")
if [ ${#ARCH_LIST[@]} -eq 0 ]; then
    ARCH_LIST=("arm64")
fi

BLUE='\033[0;34m'
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

log() { echo -e "${BLUE}[RAMFS-TEST]${NC} $1"; }
success() { echo -e "${GREEN}[RAMFS-TEST]${NC} ✅ $1"; }
warn() { echo -e "${YELLOW}[RAMFS-TEST]${NC} ⚠️  $1"; }
fail() { echo -e "${RED}[RAMFS-TEST]${NC} ❌ $1"; }

check_qemu() {
    local arch="$1"
    local qemu_bin=""
    case "$arch" in
        arm64) qemu_bin="qemu-system-aarch64" ;;
        x86_64) qemu_bin="qemu-system-x86_64" ;;
        *) return 1 ;;
    esac

    if ! command -v "$qemu_bin" >/dev/null 2>&1; then
        fail "${qemu_bin} not found in PATH"
        return 1
    fi
    return 0
}

run_for_arch() {
    local arch="$1"
    log "=== Testing architecture: $arch ==="

    if ! check_qemu "$arch"; then
        return 1
    fi

    log "Building MiniOS for $arch..."
    if ! make -C "$PROJECT_ROOT" ARCH="$arch" clean all >/dev/null 2>&1; then
        fail "Build failed for $arch"
        return 1
    fi

    local kernel_file="$BUILD_DIR/$arch/kernel.elf"
    if [ ! -f "$kernel_file" ]; then
        fail "Kernel ELF not found: $kernel_file"
        return 1
    fi

    local qemu_cmd=""
    if [ "$arch" = "arm64" ]; then
        qemu_cmd="qemu-system-aarch64 -machine virt -cpu cortex-a72 -m 512M -kernel $kernel_file -nographic -serial mon:stdio"
    else
        qemu_cmd="qemu-system-x86_64 -m 512M -kernel $kernel_file -nographic -serial mon:stdio"
    fi

    local log_file="$BUILD_DIR/test-ramfs-shell-$arch.log"
    mkdir -p "$(dirname "$log_file")"

    local commands=(
        "ls"
        "mkdir testdir"
        "ls"
        "cp welcome.txt testdir/copy.txt"
        "ls testdir"
        "mv testdir/copy.txt testdir/renamed.txt"
        "ls testdir"
        "cat testdir/renamed.txt"
        "rm testdir/renamed.txt"
        "ls testdir"
        "rmdir testdir"
        "ls"
    )

    log "Launching QEMU (timeout ${TIMEOUT_SECONDS}s)"
    set +e
    {
        sleep "$BOOT_WAIT"
        for cmd in "${commands[@]}"; do
            echo "$cmd"
            sleep "$COMMAND_DELAY"
        done
        echo "exit"
        sleep "$COMMAND_DELAY"
        sleep "$POST_EXIT_WAIT"
    } | timeout "$TIMEOUT_SECONDS" bash -c "$qemu_cmd" >"$log_file" 2>&1
    local qemu_status=$?
    set -e

    if [ $qemu_status -eq 124 ]; then
        warn "QEMU timed out after ${TIMEOUT_SECONDS}s (treated as success after shell exit)"
    elif [ $qemu_status -ne 0 ]; then
        fail "QEMU exited with status $qemu_status"
        return 1
    fi

    log "Analyzing shell output..."
    local expected_patterns=(
        "MiniOS Shell"
        "Directory created: /testdir"
        "testdir"
        "Copied /welcome.txt to /testdir/copy.txt"
        "copy.txt"
        "Moved /testdir/copy.txt to /testdir/renamed.txt"
        "renamed.txt"
        "Welcome to MiniOS!"
        "File removed: /testdir/renamed.txt"
        "(empty directory)"
        "Directory removed: /testdir"
    )

    local missing=0
    for pattern in "${expected_patterns[@]}"; do
        if ! grep -q "$pattern" "$log_file"; then
            fail "Missing expected output: $pattern"
            missing=1
        else
            success "Found: $pattern"
        fi
    done

    if grep -qi "kernel panic\|UNHANDLED EXCEPTION" "$log_file"; then
        fail "Detected critical error in QEMU log"
        missing=1
    fi

    log "Last 10 lines of output:"
    tail -10 "$log_file" | sed 's/^/  /'

    if [ $missing -ne 0 ]; then
        return 1
    fi

    success "RAMFS shell operations succeeded on $arch"
    return 0
}

overall=0
for arch in "${ARCH_LIST[@]}"; do
    if ! run_for_arch "$arch"; then
        overall=1
    fi
    echo ""
done
if [ $overall -ne 0 ]; then
    fail "RAMFS shell test FAILED"
else
    success "RAMFS shell test PASSED for all architectures"
fi

exit $overall

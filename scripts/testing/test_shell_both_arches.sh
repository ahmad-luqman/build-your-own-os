#!/usr/bin/env bash
#
# Shell regression scenario on x86-64 and ARM64 (issues #3, #29, #30, #31).
#
# Boots each kernel in QEMU, types the scenario into the serial console, and
# checks the console output of each command:
#   #31  a failed built-in does not also print "Command not found"
#   #3   each error is exactly one line: "Error: <what>: <path>[: <reason>]"
#   #29  `>` redirection and pipes work for any built-in, not just echo
#   #30  mv across directories works on SFS (files and directories), refuses
#        bad moves, and persists across umount/mount; RAMFS mv across dirs
#
# Usage:
#   scripts/testing/test_shell_both_arches.sh [x86_64|arm64|all]   (default: all)
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
QEMU_TIMEOUT=${QEMU_TIMEOUT:-120}

# Every command string must be unique: checks find a command's output by
# matching its prompt line exactly.
COMMANDS=(
    # --- #31 / #3: error reporting ---
    "cat /nope"
    "ls /missing"
    "cd /nowhere"
    "mkdir /tmp"
    "nosuchcmd"
    "cat"

    # --- #29: redirection and pipes ---
    "mkdir /tmp/d"
    "echo hi > /tmp/d/a.txt"
    "ls /tmp/d > /tmp/ls.txt"
    "cat /tmp/ls.txt"
    "ls /tmp/d | cat"
    "echo hello | cat"
    "echo chain | cat | cat"
    "ls /tmp"

    # --- #30: RAMFS mv ---
    "mkdir /tmp/e"
    "mv /tmp/d/a.txt /tmp/e/a.txt"
    "cat /tmp/e/a.txt"
    "mv /tmp/e/a.txt /tmp/e/a.txt"
    "ls /tmp/e"

    # --- #30: SFS mv ---
    "mkdir /sfs"
    "mount ramdisk0 /sfs sfs"
    "mkdir /sfs/a"
    "mkdir /sfs/a/b"
    "echo moved > /sfs/a/g.txt"
    "mv /sfs/a/g.txt /sfs/a/b/h.txt"
    "ls /sfs/a"
    "echo other > /sfs/a/x.txt"
    "mv /sfs/a/x.txt /sfs/a/b/h.txt"
    "mv /sfs/a/x.txt /sfs/nope/y.txt"
    "mkdir /sfs/c"
    "mv /sfs/a/b /sfs/c/b"
    "mv /sfs/c /sfs/c/b/c2"
    "mv /sfs/a/x.txt /sfs/c"
    "umount /sfs"
    "mount ramdisk0 /sfs sfs"
    "cat /sfs/c/b/h.txt"
    "cat /sfs/c/x.txt"
    "ls /sfs/a/"
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

# Print the console output of one command: the lines after its prompt line
# ("...MiniOS> <cmd>") up to the next prompt, with CRs and ANSI escapes removed.
block() {
    local log=$1 cmd=$2
    tr -d '\r' < "$log" | sed $'s/\x1b\\[[0-9;]*[A-Za-z]//g' |
        awk -v cmd="$cmd" -v p="MiniOS> " '
            found && index($0, p) { found = 0 }
            {
                want = p cmd
                if (length($0) >= length(want) &&
                    substr($0, length($0) - length(want) + 1) == want) {
                    buf = ""; found = 1; next
                }
            }
            found { buf = buf $0 "\n" }
            END   { printf "%s", buf }'
}

# --- assertions on a block of text (passed as $1) ---
has()        { grep -q -- "$2" <<< "$1"; }
lacks()      { ! grep -q -- "$2" <<< "$1"; }
empty()      { [ -z "$(tr -d ' \n' <<< "$1")" ]; }
# Exactly one non-blank line, which starts with "Error: ", contains "Error:"
# only once, and contains every further argument.
one_error()  {
    local text=$1; shift
    local lines
    lines=$(grep -c '[^[:space:]]' <<< "$text")
    [ "$lines" -eq 1 ] || return 1
    grep -q '^Error: ' <<< "$text" || return 1
    [ "$(grep -o 'Error:' <<< "$text" | wc -l | tr -d ' ')" -eq 1 ] || return 1
    local s
    for s in "$@"; do grep -q -- "$s" <<< "$text" || return 1; done
}

run_arch() {
    local arch=$1
    local log="$LOG_DIR/shell_${arch}_serial.log"
    local qlog="$LOG_DIR/shell_${arch}_qemu.log"
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
    b() { block "$log" "$1"; }

    check "shell started (MiniOS Shell v1.0)" grep -aq "MiniOS Shell v1.0" "$log"

    echo "  -- #31 failed built-in vs unknown command"
    check "cat /nope: no 'Command not found'"   lacks "$(b "cat /nope")" "Command not found"
    check "ls /missing: no 'Command not found'" lacks "$(b "ls /missing")" "Command not found"
    check "nosuchcmd: 'Command not found: nosuchcmd'" \
        has "$(b "nosuchcmd")" "Command not found: nosuchcmd"

    echo "  -- #3 one-line error messages"
    check "cat /nope: one line, path + reason"    one_error "$(b "cat /nope")" "/nope" "not found"
    check "ls /missing: one line, path + reason"  one_error "$(b "ls /missing")" "/missing" "not found"
    check "cd /nowhere: one line, path + reason"  one_error "$(b "cd /nowhere")" "/nowhere" "not found"
    check "mkdir /tmp: one line, path + reason"   one_error "$(b "mkdir /tmp")" "/tmp" "exists"
    check "nosuchcmd: one line"                   one_error "$(b "nosuchcmd")" "nosuchcmd"
    check "cat (no args): one line, usage hint"   one_error "$(b "cat")" "cat" "help cat"

    echo "  -- #29 redirection and pipes"
    check "ls > file: nothing on the console"      empty "$(b "ls /tmp/d > /tmp/ls.txt")"
    check "ls > file: file lists a.txt"            has "$(b "cat /tmp/ls.txt")" "a\.txt"
    local p
    p=$(b "ls /tmp/d | cat")
    check "ls | cat: prints a.txt"                 has "$p" "a\.txt"
    check "ls | cat: no error"                     lacks "$p" "Error\|Command not found"
    check "echo | cat: prints hello"               has "$(b "echo hello | cat")" "^hello$"
    p=$(b "echo chain | cat | cat")
    check "echo | cat | cat: prints chain"         has "$p" "^chain$"
    check "echo | cat | cat: no error"             lacks "$p" "Error"
    check "pipe temp files are removed"            lacks "$(b "ls /tmp")" "pipe_"

    echo "  -- #30 mv across directories (RAMFS)"
    check "mv /tmp/d/a.txt /tmp/e/a.txt succeeds"  lacks "$(b "mv /tmp/d/a.txt /tmp/e/a.txt")" "Error"
    check "moved RAMFS file reads back 'hi'"       has "$(b "cat /tmp/e/a.txt")" "^hi$"
    check "mv onto itself is refused"              one_error "$(b "mv /tmp/e/a.txt /tmp/e/a.txt")" "/tmp/e/a.txt"
    check "mv onto itself keeps the file"          has "$(b "ls /tmp/e")" "a\.txt"

    echo "  -- #30 mv across directories (SFS)"
    p=$(b "mv /sfs/a/g.txt /sfs/a/b/h.txt")
    check "mv file into subdirectory succeeds"     has "$p" "Moved"
    check "mv file into subdirectory: no error"    lacks "$p" "Error"
    check "source entry removed (ls /sfs/a)"       lacks "$(b "ls /sfs/a")" "g\.txt"
    check "mv onto existing file is refused"       one_error "$(b "mv /sfs/a/x.txt /sfs/a/b/h.txt")" "exists"
    check "mv into missing parent is refused"      one_error "$(b "mv /sfs/a/x.txt /sfs/nope/y.txt")" "not found"
    p=$(b "mv /sfs/a/b /sfs/c/b")
    check "mv directory across directories"        has "$p" "Moved"
    check "mv directory: no error"                 lacks "$p" "Error"
    check "mv directory into its own subtree is refused" \
        one_error "$(b "mv /sfs/c /sfs/c/b/c2")" "/sfs/c"
    check "mv file onto a directory moves it inside" has "$(b "mv /sfs/a/x.txt /sfs/c")" "/sfs/c/x.txt"
    check "after remount: moved file reads 'moved'" has "$(b "cat /sfs/c/b/h.txt")" "^moved$"
    check "after remount: file moved into dir reads 'other'" has "$(b "cat /sfs/c/x.txt")" "^other$"
    p=$(b "ls /sfs/a/")
    check "after remount: /sfs/a no longer lists b, g.txt, x.txt" \
        bash -c '! grep -qwE "b|g\.txt|x\.txt" <<< "$1"' _ "$p"

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

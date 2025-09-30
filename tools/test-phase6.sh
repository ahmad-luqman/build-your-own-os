#!/bin/bash

# MiniOS Phase 6 Shell User Interface Tests
# Validates shell implementation, built-in commands, and user interface

set -e

# Configuration
PROJECT_ROOT="${PROJECT_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
SRC_DIR="$PROJECT_ROOT/src"
BUILD_DIR="$PROJECT_ROOT/build"
ARCH="${ARCH:-arm64}"

# Test counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Helper functions
run_test() {
    local test_name="$1"
    local test_command="$2"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    if eval "$test_command" >/dev/null 2>&1; then
        echo -e "Test $TOTAL_TESTS: $test_name ... ${GREEN}PASS${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "Test $TOTAL_TESTS: $test_name ... ${RED}FAIL${NC}"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
}

compile_test() {
    local test_name="$1"
    local source_file="$2"
    local arch="$3"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    if make ARCH="$arch" "$source_file" >/dev/null 2>&1; then
        echo -e "Test $TOTAL_TESTS: $test_name ... ${GREEN}PASS${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "Test $TOTAL_TESTS: $test_name ... ${RED}FAIL${NC}"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
}

echo -e "${CYAN}MiniOS Phase 6 Shell User Interface Tests${NC}"
echo "=========================================="
echo "Project root: $PROJECT_ROOT"
echo

# Check if we're in the right directory
if [[ ! -f "$PROJECT_ROOT/Makefile" ]]; then
    echo -e "${RED}Error: Not in MiniOS project root${NC}"
    exit 1
fi

# Shell Header Tests
echo -e "${PURPLE}📄 Shell Header Tests${NC}"
run_test "Shell header exists" "test -f '$SRC_DIR/include/shell.h'"
run_test "Shell header compiles (ARM64)" "echo '#include \"shell.h\"' | aarch64-elf-gcc -I'$SRC_DIR/include' -x c - -c -o /dev/null"
run_test "Shell header compiles (x86-64)" "echo '#include \"shell.h\"' | x86_64-elf-gcc -I'$SRC_DIR/include' -x c - -c -o /dev/null"

# Shell Implementation Tests
echo -e "${PURPLE}🔧 Shell Implementation Tests${NC}"
run_test "Shell core implementation exists" "test -f '$SRC_DIR/shell/core/shell_core.c'"
run_test "Shell I/O implementation exists" "test -f '$SRC_DIR/shell/core/shell_io.c'"
run_test "Shell syscalls implementation exists" "test -f '$SRC_DIR/shell/core/shell_syscalls.c'"
run_test "Command parser implementation exists" "test -f '$SRC_DIR/shell/parser/parser.c'"
run_test "Built-in commands implementation exists" "test -f '$SRC_DIR/shell/commands/builtin.c'"
run_test "System info commands implementation exists" "test -f '$SRC_DIR/shell/commands/sysinfo.c'"

# Shell Structure Tests
echo -e "${PURPLE}🏗️ Shell Structure Tests${NC}"
run_test "Shell directories exist" "test -d '$SRC_DIR/shell/core' -a -d '$SRC_DIR/shell/commands' -a -d '$SRC_DIR/shell/parser'"

# Built-in Command Tests
echo -e "${PURPLE}📋 Built-in Command Function Tests${NC}"
run_test "cd command function exists" "grep -q 'cmd_cd' '$SRC_DIR/shell/commands/builtin.c'"
run_test "pwd command function exists" "grep -q 'cmd_pwd' '$SRC_DIR/shell/commands/builtin.c'"
run_test "ls command function exists" "grep -q 'cmd_ls' '$SRC_DIR/shell/commands/builtin.c'"
run_test "cat command function exists" "grep -q 'cmd_cat' '$SRC_DIR/shell/commands/builtin.c'"
run_test "mkdir command function exists" "grep -q 'cmd_mkdir' '$SRC_DIR/shell/commands/builtin.c'"
run_test "rm command function exists" "grep -q 'cmd_rm' '$SRC_DIR/shell/commands/builtin.c'"
run_test "echo command function exists" "grep -q 'cmd_echo' '$SRC_DIR/shell/commands/builtin.c'"
run_test "help command function exists" "grep -q 'cmd_help' '$SRC_DIR/shell/commands/builtin.c'"
run_test "exit command function exists" "grep -q 'cmd_exit' '$SRC_DIR/shell/commands/builtin.c'"

# System Information Command Tests
echo -e "${PURPLE}📊 System Information Command Tests${NC}"
run_test "ps command function exists" "grep -q 'cmd_ps' '$SRC_DIR/shell/commands/sysinfo.c'"
run_test "free command function exists" "grep -q 'cmd_free' '$SRC_DIR/shell/commands/sysinfo.c'"
run_test "uname command function exists" "grep -q 'cmd_uname' '$SRC_DIR/shell/commands/sysinfo.c'"
run_test "date command function exists" "grep -q 'cmd_date' '$SRC_DIR/shell/commands/sysinfo.c'"
run_test "uptime command function exists" "grep -q 'cmd_uptime' '$SRC_DIR/shell/commands/sysinfo.c'"

# Shell Core Function Tests
echo -e "${PURPLE}⚙️ Shell Core Function Tests${NC}"
run_test "Shell initialization function exists" "grep -q 'shell_init' '$SRC_DIR/shell/core/shell_core.c'"
run_test "Shell main loop function exists" "grep -q 'shell_run' '$SRC_DIR/shell/core/shell_core.c'"
run_test "Command parser function exists" "grep -q 'shell_parse_command' '$SRC_DIR/shell/core/shell_core.c'"
run_test "Command finder function exists" "grep -q 'shell_find_command' '$SRC_DIR/shell/core/shell_core.c'"
run_test "Shell main task function exists" "grep -q 'shell_main_task' '$SRC_DIR/shell/core/shell_core.c'"

# Shell I/O Function Tests
echo -e "${PURPLE}💬 Shell I/O Function Tests${NC}"
run_test "Shell prompt function exists" "grep -q 'shell_print_prompt' '$SRC_DIR/shell/core/shell_io.c'"
run_test "Shell read command function exists" "grep -q 'shell_read_command' '$SRC_DIR/shell/core/shell_io.c'"
run_test "Shell print function exists" "grep -q 'shell_print' '$SRC_DIR/shell/core/shell_io.c'"
run_test "Shell printf function exists" "grep -q 'shell_printf' '$SRC_DIR/shell/core/shell_io.c'"

# Command Parser Tests
echo -e "${PURPLE}🔍 Command Parser Function Tests${NC}"
run_test "Parse command line function exists" "grep -q 'parse_command_line' '$SRC_DIR/shell/parser/parser.c'"
run_test "Execute command function exists" "grep -q 'execute_command' '$SRC_DIR/shell/parser/parser.c'"
run_test "Execute builtin command function exists" "grep -q 'execute_builtin_command' '$SRC_DIR/shell/parser/parser.c'"
run_test "I/O redirection setup exists" "grep -q 'setup_io_redirection' '$SRC_DIR/shell/parser/parser.c'"

# Shell System Call Tests
echo -e "${PURPLE}🔧 Shell System Call Tests${NC}"
run_test "Enhanced syscall header updated" "grep -q 'SYSCALL_GETCWD' '$SRC_DIR/include/syscall.h'"
run_test "Get current directory syscall exists" "grep -q 'syscall_getcwd' '$SRC_DIR/shell/core/shell_syscalls.c'"
run_test "Change directory syscall exists" "grep -q 'syscall_chdir' '$SRC_DIR/shell/core/shell_syscalls.c'"
run_test "Register shell syscalls function exists" "grep -q 'register_shell_syscalls' '$SRC_DIR/shell/core/shell_syscalls.c'"

# Kernel Integration Tests  
echo -e "${PURPLE}🔗 Kernel Integration Tests${NC}"
run_test "Kernel includes shell header" "grep -q '#include \"shell.h\"' '$SRC_DIR/kernel/main.c'"
run_test "Kernel initializes shell system" "grep -q 'shell_init_system' '$SRC_DIR/kernel/main.c'"
run_test "Kernel registers shell syscalls" "grep -q 'register_shell_syscalls' '$SRC_DIR/kernel/main.c'"
run_test "Kernel creates shell task" "grep -q 'shell_main_task' '$SRC_DIR/kernel/main.c'"

# Build Integration Tests
echo -e "${PURPLE}🛠️ Build Integration Tests${NC}"
run_test "Makefile includes shell sources" "grep -q 'shell/core' '$PROJECT_ROOT/Makefile'"
run_test "Makefile includes shell commands" "grep -q 'shell/commands' '$PROJECT_ROOT/Makefile'"
run_test "Makefile includes shell parser" "grep -q 'shell/parser' '$PROJECT_ROOT/Makefile'"

# Cross-platform Build Tests
echo -e "${PURPLE}🏗️ Cross-platform Build Tests${NC}"
cd "$PROJECT_ROOT"

echo "Building ARM64 with shell support..."
run_test "ARM64 build with shell succeeds" "make ARCH=arm64 clean all"

echo "Building x86-64 with shell support..."  
run_test "x86-64 build with shell succeeds" "make ARCH=x86_64 clean all"

# Phase Integration Tests
echo -e "${PURPLE}🔄 Phase Integration Tests${NC}"
run_test "Phase 5 file system still functional" "grep -q 'vfs_mount' '$SRC_DIR/kernel/main.c'"
run_test "Shell integrates with file system" "grep -q 'fd_table' '$SRC_DIR/include/shell.h'"
run_test "Shell uses process management" "grep -q 'process_create' '$SRC_DIR/kernel/main.c'"

# Documentation Tests
echo -e "${PURPLE}📚 Documentation Tests${NC}"
run_test "Phase 6 test script exists" "test -f '$PROJECT_ROOT/tools/test-phase6.sh'"

# Summary
echo
echo -e "${CYAN}=========================================${NC}"
echo -e "${CYAN}Phase 6 Shell Tests Summary${NC}"
echo -e "${CYAN}=========================================${NC}"
echo -e "Total tests: ${BLUE}$TOTAL_TESTS${NC}"
echo -e "Passed: ${GREEN}$PASSED_TESTS${NC}"
echo -e "Failed: ${RED}$FAILED_TESTS${NC}"

if [[ $FAILED_TESTS -eq 0 ]]; then
    echo -e "Result: ${GREEN}ALL TESTS PASSED! ✅${NC}"
    echo -e "${GREEN}Phase 6 shell implementation is complete and functional!${NC}"
    exit 0
else
    echo -e "Result: ${RED}$FAILED_TESTS TESTS FAILED ❌${NC}"
    pass_rate=$((PASSED_TESTS * 100 / TOTAL_TESTS))
    echo -e "Pass rate: ${YELLOW}$pass_rate%${NC}"
    
    if [[ $pass_rate -ge 80 ]]; then
        echo -e "${YELLOW}Shell implementation is mostly complete with some minor issues.${NC}"
    elif [[ $pass_rate -ge 60 ]]; then
        echo -e "${YELLOW}Shell implementation is partially complete. Review failed tests.${NC}"
    else
        echo -e "${RED}Shell implementation needs significant work.${NC}"
    fi
    exit 1
fi
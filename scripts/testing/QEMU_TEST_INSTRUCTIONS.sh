#!/bin/bash
# HOWTO: Test Echo Redirection Fix in QEMU

cat << 'EOF'
========================================
MiniOS - Echo Redirection Fix Testing
========================================

BUG #1 HAS BEEN FIXED!

The output redirection bug (echo Hello > file.txt) has been patched.
Now you need to test it in QEMU to confirm it works.

========================================
STEP 1: Build the OS (if not already done)
========================================

cd /Users/ahmadluqman/src/build-your-own-os
make clean
make ARCH=arm64 all

This builds:
  ✓ bootloader.elf
  ✓ kernel.elf  (this is what we boot)
  ✓ minios.img

========================================
STEP 2: Start QEMU
========================================

Run this command:

./qemu_test.sh

OR manually:

qemu-system-aarch64 \
    -machine virt \
    -cpu cortex-a72 \
    -m 512M \
    -nographic \
    -serial mon:stdio \
    -kernel build/arm64/kernel.elf

========================================
STEP 3: Test the Echo Redirection
========================================

Once you see the "MiniOS>" prompt, type these commands:

MiniOS> ls
MiniOS> echo Hello World > test.txt
MiniOS> ls
MiniOS> cat test.txt

========================================
EXPECTED RESULTS (if fix works):
========================================

✅ After "echo Hello World > test.txt":
   - NO OUTPUT on screen (completely silent)
   - This is correct! Output went to file

✅ After "ls":
   - You should see "test.txt" in the file listing

✅ After "cat test.txt":
   - Should display: "Hello World"

========================================
WHAT IT LOOKED LIKE BEFORE (the bug):
========================================

❌ After "echo Hello World > test.txt":
   - "Hello World" appeared on screen (BUG!)
   - File was empty or not created

========================================
TO EXIT QEMU:
========================================

Press: Ctrl+A
Then press: X

(That's Control-A, release, then X)

========================================
TROUBLESHOOTING:
========================================

If QEMU doesn't start:
  - Check QEMU is installed: which qemu-system-aarch64
  - Install if needed: brew install qemu

If no shell prompt appears:
  - Wait 5-10 seconds for boot
  - Check build succeeded: ls -lh build/arm64/kernel.elf
  - Try rebuild: make clean && make ARCH=arm64 all

If you get stuck in QEMU:
  - Force quit: Ctrl+A then X
  - Or close terminal window

========================================
FILES MODIFIED FOR THIS FIX:
========================================

1. src/include/shell.h
   - Added: char *output_redirect_file to shell_context

2. src/shell/core/shell_core.c
   - Initialize output_redirect_file = NULL

3. src/shell/parser/parser.c
   - Store cmd->output_redirect in ctx->output_redirect_file
   - Restore after command execution

4. src/shell/commands/builtin.c
   - Check ctx->output_redirect_file instead of argv
   - Write to file when redirection is active

========================================
DOCUMENTATION:
========================================

Full details: BUG1_FIX_SUMMARY.md
Bug analysis: URGENT_BUGS_TEST_REPORT.md
This guide: QEMU_TEST_INSTRUCTIONS.sh

========================================

Ready to test! Run: ./qemu_test.sh

EOF

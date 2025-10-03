#!/bin/bash
# Test script to verify echo redirection bug fix
# This creates test commands and shows how to manually test

echo "========================================="
echo "Echo Redirection Bug Fix - Test Guide"
echo "========================================="
echo ""
echo "✅ Fix has been applied to the following files:"
echo "   1. src/include/shell.h - Added output_redirect_file to shell_context"
echo "   2. src/shell/core/shell_core.c - Initialize output_redirect_file"
echo "   3. src/shell/parser/parser.c - Store redirection in context"
echo "   4. src/shell/commands/builtin.c - Check context instead of argv"
echo ""
echo "========================================="
echo "How the Fix Works:"
echo "========================================="
echo ""
echo "BEFORE (Broken):"
echo "  1. Parser sees 'echo Hello > file.txt'"
echo "  2. Parser removes '>' and 'file.txt' from argv"
echo "  3. Parser calls cmd_echo with just ['echo', 'Hello']"
echo "  4. cmd_echo looks for '>' in argv... doesn't find it!"
echo "  5. cmd_echo prints to console ❌"
echo ""
echo "AFTER (Fixed):"
echo "  1. Parser sees 'echo Hello > file.txt'"
echo "  2. Parser stores 'file.txt' in ctx->output_redirect_file"
echo "  3. Parser removes '>' and 'file.txt' from argv"
echo "  4. Parser calls cmd_echo with just ['echo', 'Hello']"
echo "  5. cmd_echo checks ctx->output_redirect_file"
echo "  6. cmd_echo writes to file, NOT console ✅"
echo ""
echo "========================================="
echo "Test Commands (run in QEMU manually):"
echo "========================================="
echo ""
cat << 'EOF'
# Start QEMU:
qemu-system-aarch64 -M virt -cpu cortex-a72 -m 512M -nographic \
    -kernel build/arm64/bootloader.elf -serial mon:stdio

# Once you see MiniOS> prompt, type:

ls
echo "This should go to file" > testfile.txt
ls
cat testfile.txt

# EXPECTED RESULTS (if fix works):
# - After "echo ... > testfile.txt": NO OUTPUT (silent)
# - After "ls": you should see "testfile.txt" listed
# - After "cat testfile.txt": should show "This should go to file"

# To exit QEMU: Ctrl+A then X
EOF

echo ""
echo "========================================="
echo "Code Changes Summary:"
echo "========================================="
echo ""

echo "1. Added to shell_context (shell.h line ~52):"
echo "   char *output_redirect_file;  // NULL if no redirection"
echo ""

echo "2. Initialize in shell_init (shell_core.c line ~87):"
echo "   ctx->output_redirect_file = NULL;"
echo ""

echo "3. Set in execute_command (parser.c line ~87):"
echo "   ctx->output_redirect_file = cmd->output_redirect;"
echo ""

echo "4. Check in cmd_echo (builtin.c line ~432):"
echo "   if (ctx->output_redirect_file) {"
echo "       // Write to file"
echo "   } else {"
echo "       // Write to console"
echo "   }"
echo ""

echo "========================================="
echo "Build Status:"
echo "========================================="
if [ -f "build/arm64/bootloader.elf" ]; then
    echo "✅ Build successful: build/arm64/bootloader.elf"
    ls -lh build/arm64/bootloader.elf
else
    echo "❌ Build file not found - run: make ARCH=arm64"
fi

echo ""
echo "Ready to test! See instructions above."

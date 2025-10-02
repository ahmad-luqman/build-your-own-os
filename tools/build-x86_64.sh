#!/bin/bash
# Quick test script for x86_64 with GRUB ISO boot

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "Building x86_64 kernel..."
make -C "$PROJECT_ROOT" ARCH=x86_64 clean all

echo "Creating bootable ISO..."
"$PROJECT_ROOT/tools/create-grub-iso.sh"

echo ""
echo "====================================================================="
echo "  x86_64 kernel built and ISO created successfully!"
echo "====================================================================="
echo ""
echo "To test in QEMU:"
echo "  qemu-system-x86_64 -m 512M -cdrom build/x86_64/minios.iso -boot d -nographic -serial mon:stdio"
echo ""
echo "To run automated test:"
echo "  ./tests/test_ramfs_shell.sh x86_64"
echo ""
echo "ISO location: build/x86_64/minios.iso"
echo "====================================================================="

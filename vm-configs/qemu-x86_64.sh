#!/bin/bash
# QEMU x86-64 Configuration for MiniOS

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"

# Try ISO first, then IMG
if [ -f "$BUILD_DIR/x86_64/minios.iso" ]; then
    IMAGE_FILE="$BUILD_DIR/x86_64/minios.iso"
    IMAGE_TYPE="iso"
elif [ -f "$BUILD_DIR/x86_64/minios.img" ]; then
    IMAGE_FILE="$BUILD_DIR/x86_64/minios.img"
    IMAGE_TYPE="img"
else
    echo "Error: No x86-64 image found"
    echo "Run 'make ARCH=x86_64' first"
    exit 1
fi

echo "Starting MiniOS x86-64 in QEMU..."
echo "Image: $IMAGE_FILE (type: $IMAGE_TYPE)"

# QEMU x86-64 configuration
if [ "$IMAGE_TYPE" = "iso" ]; then
    exec qemu-system-x86_64 \
        -m 512M \
        -cdrom "$IMAGE_FILE" \
        -boot d \
        -netdev user,id=net0 \
        -device rtl8139,netdev=net0 \
        -nographic \
        -serial mon:stdio \
        -append "console=ttyS0"
else
    exec qemu-system-x86_64 \
        -m 512M \
        -drive file="$IMAGE_FILE",format=raw \
        -netdev user,id=net0 \
        -device rtl8139,netdev=net0 \
        -nographic \
        -serial mon:stdio \
        -append "console=ttyS0"
fi
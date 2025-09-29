#!/bin/bash
# QEMU ARM64 Configuration for MiniOS

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
IMAGE_FILE="$BUILD_DIR/arm64/minios.img"

# Check if image exists
if [ ! -f "$IMAGE_FILE" ]; then
    echo "Error: ARM64 image not found: $IMAGE_FILE"
    echo "Run 'make ARCH=arm64' first"
    exit 1
fi

echo "Starting MiniOS ARM64 in QEMU..."
echo "Image: $IMAGE_FILE"

# QEMU ARM64 configuration
exec qemu-system-aarch64 \
    -machine virt \
    -cpu cortex-a72 \
    -m 512M \
    -drive file="$IMAGE_FILE",format=raw,if=virtio \
    -netdev user,id=net0 \
    -device virtio-net-device,netdev=net0 \
    -nographic \
    -serial mon:stdio \
    -append "console=ttyAMA0"
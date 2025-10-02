#!/bin/bash
# Create a GRUB-based ISO image for x86_64 with multiboot2 support

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build/x86_64"
KERNEL_ELF="$BUILD_DIR/kernel.elf"
ISO_OUTPUT="$BUILD_DIR/minios.iso"

if [ ! -f "$KERNEL_ELF" ]; then
    echo "Error: Kernel not found at $KERNEL_ELF"
    echo "Run 'make ARCH=x86_64' first"
    exit 1
fi

# Create ISO directory structure
ISO_DIR="$BUILD_DIR/iso"
rm -rf "$ISO_DIR"
mkdir -p "$ISO_DIR/boot/grub"

# Copy kernel
cp "$KERNEL_ELF" "$ISO_DIR/boot/kernel.elf"

# Create GRUB configuration
cat > "$ISO_DIR/boot/grub/grub.cfg" << 'EOF'
set timeout=0
set default=0

terminal_input console
terminal_output console
insmod vga_text
insmod vbe

menuentry "MiniOS" {
    insmod multiboot2
    multiboot2 /boot/kernel.elf
    boot
}
EOF

# Check if grub-mkrescue is available
GRUB_MKRESCUE=""
if command -v i686-elf-grub-mkrescue &> /dev/null; then
    GRUB_MKRESCUE="i686-elf-grub-mkrescue"
elif command -v x86_64-elf-grub-mkrescue &> /dev/null; then
    GRUB_MKRESCUE="x86_64-elf-grub-mkrescue"
elif command -v grub-mkrescue &> /dev/null; then
    GRUB_MKRESCUE="grub-mkrescue"
else
    echo "Error: grub-mkrescue not found"
    echo "Install GRUB tools: brew install i686-elf-grub"
    exit 1
fi

echo "Using $GRUB_MKRESCUE"

# Create ISO
echo "Creating ISO image with GRUB..."
$GRUB_MKRESCUE -o "$ISO_OUTPUT" "$ISO_DIR" 2>&1 || {
    echo "Error: grub-mkrescue failed"
    echo "Trying alternative approach..."
    
    # Try using xorriso directly if grub-mkrescue fails
    if command -v xorriso &> /dev/null; then
        echo "Using xorriso..."
        xorriso -as mkisofs -R -J -c boot/boot.cat -b boot/grub/i386-pc/eltorito.img \
            -no-emul-boot -boot-load-size 4 -boot-info-table -o "$ISO_OUTPUT" "$ISO_DIR"
    else
        echo "Error: Neither grub-mkrescue nor xorriso worked"
        exit 1
    fi
}

# Cleanup
rm -rf "$ISO_DIR"

echo "ISO created: $ISO_OUTPUT"
ls -lh "$ISO_OUTPUT"

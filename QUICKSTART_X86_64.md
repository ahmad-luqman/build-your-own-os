# Quick Start: x86_64 MiniOS on QEMU

## Prerequisites
```bash
brew install i686-elf-grub xorriso qemu
```

## Build & Run

### Single Command
```bash
./tools/build-x86_64.sh && \
qemu-system-x86_64 -m 512M -cdrom build/x86_64/minios.iso -boot d -nographic -serial mon:stdio
```

### Step by Step
```bash
# 1. Build kernel
make ARCH=x86_64 clean all

# 2. Create bootable ISO
./tools/create-grub-iso.sh

# 3. Run in QEMU
qemu-system-x86_64 -m 512M -cdrom build/x86_64/minios.iso -boot d -nographic -serial mon:stdio
```

## Debug Mode
```bash
qemu-system-x86_64 -m 512M -cdrom build/x86_64/minios.iso -boot d \
  -nographic -serial file:serial.log -debugcon file:debug.log \
  -d int,cpu_reset -D qemu-trace.log
```

## Expected Behavior

### What Works ✅
- QEMU boots from ISO
- GRUB loads and displays menu
- Kernel loads via multiboot2
- Kernel enters 64-bit mode
- Debug output on port 0xE9

### What Doesn't Work Yet ⚠️
- Kernel crashes before serial output
- No shell prompt
- Need to fix multiboot2 info parsing

## Files Generated
```
build/x86_64/
├── kernel.elf       # Kernel binary
└── minios.iso       # Bootable ISO (14MB)
```

## Exit QEMU
- Press `Ctrl-A` then `X` (when using `-nographic`)
- Or type `quit` in QEMU monitor

## Troubleshooting

### "No such command: i686-elf-grub-mkrescue"
```bash
brew install i686-elf-grub
```

### "ISO file not found"
```bash
./tools/create-grub-iso.sh
```

### "Kernel keeps rebooting"
This is expected. The kernel boots but crashes early.
See `NEXT_SESSION_X86_64_DEBUG.md` for how to fix.

## Compare with ARM64
```bash
# ARM64 (simpler, direct kernel load)
make ARCH=arm64 clean all
qemu-system-aarch64 -machine virt -cpu cortex-a72 -m 512M \
  -kernel build/arm64/kernel.elf -nographic -serial mon:stdio
```

## More Info
- Full details: `HANDOFF_X86_64_BOOT_COMPLETE.md`
- Debug guide: `NEXT_SESSION_X86_64_DEBUG.md`
- Technical docs: `X86_64_QEMU_BOOT_FIX.md`

# NEXT SESSION: x86_64 Kernel Debugging Guide

## What Was Fixed This Session

✅ **x86_64 QEMU Boot Issue - COMPLETELY RESOLVED**

The kernel now boots successfully via GRUB multiboot2! See `SESSION_SUMMARY_X86_64_BOOT.md` for full details.

## Current Situation

The x86_64 kernel:
- ✅ Boots from GRUB multiboot2 ISO
- ✅ Loads correctly into memory
- ✅ Enters 64-bit long mode successfully
- ⚠️ Crashes early during initialization (before any serial output)

## Quick Start

```bash
# Build and create ISO for x86_64
./tools/build-x86_64.sh

# Run in QEMU
qemu-system-x86_64 -m 512M -cdrom build/x86_64/minios.iso -boot d -nographic -serial mon:stdio

# Or with debug output
qemu-system-x86_64 -m 512M -cdrom build/x86_64/minios.iso -boot d \
  -nographic -serial file:serial.log -debugcon file:debug.log
```

## What Needs Fixing

The kernel crashes early because:

1. **Multiboot2 Boot Info Not Parsed**
   - `kernel_main()` expects a custom boot_info structure
   - GRUB passes a multiboot2 info structure instead
   - Need to parse multiboot2 tags and convert to internal format

2. **Memory Map Initialization**
   - ARM64 uses a simple hardcoded memory map
   - x86_64 needs to parse multiboot2 memory map
   - Memory allocator may fail without proper memory regions

3. **UART Initialization**
   - 16550 UART at 0x3F8 should work but might not be initialized
   - No serial output suggests early crash before UART init

## Debugging Steps

### 1. Add More Debug Output in kernel_entry.asm

```nasm
; In long_mode_entry, after each major step:
mov dx, 0xE9
mov al, 'A'    ; After entering 64-bit mode
out dx, al
mov al, 'B'    ; After BSS cleared
out dx, al
mov al, 'C'    ; After boot_info setup
out dx, al
mov al, 'D'    ; Before calling kernel_main
out dx, al
```

### 2. Check Multiboot2 Info Structure

In `src/arch/x86_64/kernel_entry.asm`:
- EBX register contains pointer to multiboot2 info structure
- Need to parse it before calling kernel_main
- Convert multiboot2 tags to internal boot_info format

### 3. Add Early Serial Output

In `kernel_main()`:
```c
// Add at very start, before any other code
volatile uint16_t *uart = (uint16_t *)0x3F8;
*uart = 'K';  // Kernel started marker
```

### 4. Check Memory Setup

- Verify page tables are correctly set up
- Check that kernel is loaded at correct address (1MB)
- Ensure BSS is properly zeroed

## Key Files to Check

- `src/arch/x86_64/kernel_entry.asm` - Entry point and 64-bit setup
- `src/kernel/main.c` - kernel_main() function
- `src/arch/x86_64/init.c` - x86_64-specific initialization
- `src/drivers/uart/16550.c` - UART driver for x86_64

## Useful Commands

```bash
# Check kernel symbols
x86_64-elf-nm build/x86_64/kernel.elf | grep -E "(kernel_main|_start)"

# Disassemble entry point
x86_64-elf-objdump -d build/x86_64/kernel.elf | less

# Check section layout
x86_64-elf-readelf -S build/x86_64/kernel.elf

# Debug with QEMU
qemu-system-x86_64 -m 512M -cdrom build/x86_64/minios.iso -boot d \
  -nographic -serial mon:stdio -d int,cpu_reset -D qemu-debug.log
```

## References

- **Multiboot2 Specification**: https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html
- **OS Dev Wiki x86_64**: https://wiki.osdev.org/X86-64
- **UART 16550**: https://wiki.osdev.org/Serial_Ports

## Expected Outcome

Once fixed, you should see:
```
MiniOS v1.0.0 (x86-64)
Initializing kernel...
Memory: 512 MB
VFS: Mounting root filesystem...
Shell: Starting interactive shell...
MiniOS Shell v1.0>
```

## Success Criteria

- [x] Kernel boots via GRUB ← **DONE THIS SESSION**
- [ ] Kernel reaches kernel_main() 
- [ ] Serial output appears
- [ ] Memory allocator initializes
- [ ] Shell prompt appears
- [ ] Can run ls, cat, mkdir commands (like ARM64)

Good luck! The hard part (boot path) is done. Now it's "just" kernel debugging! 🚀

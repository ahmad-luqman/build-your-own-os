# MiniOS Copilot Instructions

## Project Architecture

MiniOS is a **cross-platform educational operating system** supporting ARM64 (primary) and x86-64 architectures. Currently at **Phase 5 completion** (File System Implementation) with 71% overall progress.

### Key Structural Patterns

- **Hardware Abstraction Layer**: Clean separation in `src/arch/{arm64,x86_64}/` with shared interfaces in `src/include/`
- **Phase-based Development**: 7 phases from foundation to polish, each building incrementally
- **Cross-platform Build System**: Architecture-specific configs in `tools/build/arch-*.mk` with unified Makefile
- **Microkernel Design**: Modular components (VFS, drivers, memory, processes) with clear boundaries

## Critical Build System Knowledge

### Architecture Selection & Cross-Compilation
```bash
make ARCH=arm64    # ARM64 (default) - aarch64-elf-gcc toolchain
make ARCH=x86_64   # x86-64 - x86_64-elf-gcc + NASM toolchain  
```

### CRITICAL ARM64 Compiler Workaround
**Never remove these flags from `tools/build/arch-arm64.mk`:**
```makefile
-fno-tree-vectorize -fno-slp-vectorize -fno-tree-loop-vectorize
```
GCC's SIMD vectorization causes **stack corruption** in exception context. SFS and VFS components use forced `-O0` compilation to prevent this.

### Debug vs Release Builds
- `DEBUG=1`: Enables debug symbols, logging, and `-O0` optimization
- Release: Uses `-O2` but with vectorization disabled on ARM64
- Log levels: DEBUG(0), INFO(1), WARN(2), ERROR(3), FATAL(4)

## Essential Development Workflows

### Testing Hierarchy
1. **Phase Tests**: `tools/test-phase*.sh` (official comprehensive tests)
2. **Component Tests**: `scripts/testing/` (development/debugging tests)  
3. **VM Testing**: `make test` runs architecture-specific VM testing

### File System Development Patterns
- **VFS Layer**: `src/fs/vfs/` - Virtual File System abstraction
- **SFS Implementation**: `src/fs/sfs/` - Simple File System (block-based)
- **RAMFS**: `src/fs/ramfs/` - RAM-based file system
- **Block Devices**: `src/fs/block/` - Storage abstraction

### Memory Management Architecture
- **ARM64**: 4KB pages, TTBR0/TTBR1 separation, 256TB address space
- **x86-64**: 4-level paging with NX bit, 128TB address space
- Physical allocators use bitmap-based allocation (64MB ARM64, 128MB x86-64)

## Project-Specific Conventions

### Header Organization
- `src/include/`: Shared cross-platform headers (kernel.h, vfs.h, memory.h)
- `src/arch/*/include/`: Architecture-specific headers
- Include order: kernel.h first, then component headers

### System Call Interface
- **ARM64**: `svc #0`, args in x0-x5, return in x0
- **x86-64**: `syscall`, args in rdi,rsi,rdx,rcx,r8,r9, return in rax
- Unified syscall numbers in `src/include/syscall.h`

### Error Handling Patterns
```c
// VFS error codes (not standard errno)
#define VFS_SUCCESS        0
#define VFS_ERROR_NOTFOUND -1
#define VFS_ERROR_NOMEM    -2
// Always check return values, especially in file system operations
```

### Kernel Entry Points
- **ARM64**: `src/arch/arm64/kernel_start.S` (direct boot entry)
- **x86-64**: Multiboot2 entry in bootloader
- Memory layout defined in `src/arch/*/linker.ld`

## Integration Patterns

### Cross-Architecture Device Drivers
- Timer: ARM64 Generic Timer vs x86-64 PIT/APIC
- UART: ARM64 PL011 vs x86-64 16550
- Interrupts: ARM64 GIC vs x86-64 IDT/PIC
- Unified interfaces in `src/include/device.h`

### Boot Protocol Abstraction
- `src/include/boot_protocol.h` provides unified boot_info structure
- ARM64 UEFI vs x86-64 Multiboot2 -> common kernel initialization

### Testing Integration Points
- VM configs in `vm-configs/` for UTM (ARM64) and QEMU (x86-64)
- Test outputs logged to `tmp/` directory (gitignored)
- Phase validation through `tools/test-phase*.sh` scripts

## Documentation References

- **CLAUDE.md**: Comprehensive development guide and current patterns
- **docs/development/CURRENT_STATUS.md**: Project progress and phase completion
- **docs/development/SIMD_VECTORIZATION_FIX.md**: Critical ARM64 compiler fix details
- **scripts/testing/README.md**: Testing script reference and usage patterns
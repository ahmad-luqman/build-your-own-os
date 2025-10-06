# SFS Crash Analysis – RAMFS Mount Alignment Fault (Updated Feb 2025)

## Current Status
- Release and debug ARM64 builds now boot through Phase 5 and reach the shell without crashing.
- The synchronous exception previously observed at PC `0x600003C5` during RAMFS mount has been eliminated.
- Anti-vectorization compiler flags remain in `tools/build/arch-arm64.mk` as a defensive measure, but allocator alignment is the decisive fix.

## Root Cause
GCC legitimately emits 128-bit NEON stores (`str qX`) while copying RAMFS metadata structures at `-O2`. Our custom `kmalloc` implementation returned buffers that were only 8-byte aligned, so any caller that received one of those buffers and later triggered a 16-byte store caused a hardware alignment fault. The crash manifested while mounting RAMFS because directory entries allocated during Phase 5 hit this misalignment and faulted immediately after `ramfs_mount` transitioned into its copy loops.

## Fix Overview
1. **Allocator Alignment** – `kmalloc` now rounds allocation sizes and returned addresses up to a 16-byte boundary (`src/kernel/memory.c`), guaranteeing that NEON stores land on legal boundaries.
2. **Exception Context Preservation** – The ARM64 exception stubs were rebuilt so that saved context no longer clobbers working registers, allowing the trap frame to report the true PC/SP of any future faults.
3. **Compiler Guards (Optional)** – The existing `-fno-*vectorize` flags are left in place for now; they are no longer required for correctness but reduce the risk of reintroducing wide stores in other subsystems while tighter testing is absent.

## Validation
- Rebuilt with `make ARCH=arm64` (release) and exercised boot-to-shell flow in QEMU; no traps encountered.
- Verified RAMFS mount and subsequent shell commands (`ls`, `cat`, `touch`) execute normally.
- Inspected exception logs to confirm the context dump reflects accurate register values after the stub rewrite.

## Historical Notes
Earlier investigations focused on suppressing SIMD generation altogether. While that reduced the frequency of faults, it masked the underlying allocator alignment bug and left RAMFS mount unstable. Retaining a record of the older vectors-related analysis can still help explain why defensive compiler flags are present, but the actionable fix is allocator alignment.

## Follow-Up Items
- Add an automated boot smoke test that runs `ramfs_mount` and basic file operations under QEMU to prevent regressions.
- Backfill unit or integration coverage for the new exception save/restore paths so that future changes to the trap frame layout are caught early.
- Revisit the need for the aggressive `-fno-*vectorize` flags once automated coverage is in place.

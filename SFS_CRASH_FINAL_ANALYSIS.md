# SFS Crash Final Analysis – RAMFS Phase 5 Exception

## Executive Summary
ARM64 boots of MiniOS previously terminated with a synchronous exception at PC `0x600003C5` while mounting RAMFS during Phase 5. The fault stemmed from `kmalloc` returning buffers that were not 16-byte aligned; GCC emitted 128-bit NEON stores into those buffers, triggering an alignment fault. After updating the allocator to guarantee 16-byte alignment and refreshing the exception stubs to preserve PC/SP correctly, both release and debug builds boot cleanly to the shell. The earlier assumption of a compiler vectorization bug is now superseded by this allocator fix.

## Investigation Timeline
- **Initial Observation** – Phase 5 RAMFS mount crashed consistently on ARM64, with trap records pointing near `ramfs_mount` but containing inconsistent register dumps.
- **Vectorization Hypothesis** – Early objdump analysis revealed NEON stores and led to disabling GCC auto-vectorization. This reduced symptoms but did not eliminate the crash and obscured the root cause.
- **Allocator Review** – Instrumenting allocations showed RAMFS structures coming from the simple bump allocator in `src/kernel/memory.c`, which only respected 8-byte alignment despite callers performing 16-byte accesses.
- **Resolution** – `kmalloc` and the static heap now align both the allocation size and returned pointer to 16 bytes, and the exception save/restore path was rewritten so trap frames remain trustworthy for future debugging.

## Technical Findings
- **Fault Type** – Data abort / synchronous exception caused by misaligned 128-bit store during RAMFS metadata initialization.
- **Reproduction** – Occurred deterministically when RAMFS mounted immediately after Phase 4; not observed on x86_64 due to different alignment rules.
- **Disassembly Insight** – Even with anti-vectorization flags, GCC emits paired stores for structure copies when it can prove 16-byte alignment. The allocator contract must therefore provide that alignment guarantee.
- **Exception Stubs** – The previous stubs overwrote saved registers, making backtraces unreliable. Updated stubs mirror `exception_context` layout and keep scratch registers intact, allowing accurate GDB/Lldb sessions.

## Fix Implementation
1. **Allocator Alignment** – `src/kernel/memory.c`: simple heap storage aligned via `__attribute__((aligned(16)))`, request size rounding to 16 bytes, and offset alignment before returning pointers.
2. **Context Save/Resto** – `src/arch/arm64/process/context.S` and exception entry paths refreshed to spill/restores without clobbering the active frame, ensuring reliable diagnostics.
3. **Safety Flags** – `tools/build/arch-arm64.mk` retains `-fno-tree-*vectorize` flags; these are precautionary until automated coverage is in place.

## Validation & Testing
- `make ARCH=arm64` (release) → QEMU boot-to-shell smoke test with RAMFS mount, directory creation, list, and cat commands.
- `make ARCH=arm64 DEBUG=1` → QEMU debug build run to confirm no traps and that context dumps show matching PC/SP when manually triggered.
- Manual LLDB session attaches via QEMU `-s -S` and confirms exception frames now contain the correct registers after forced traps.

## Remaining Risks & Follow-Up
- **Automation Gap** – No scripted regression yet to exercise RAMFS mount and ensure allocator alignment stays intact.
- **Exception Coverage** – Need targeted tests for the new `save_context` / `restore_context` routines to catch layout drift.
- **Build Flags Review** – Reassess the necessity of the anti-vectorization flags once automated coverage exists; keep them for now to minimize churn.

## Recommended Next Steps
1. Add a QEMU-based CI smoke test that boots the ARM64 image, runs RAMFS file operations, and fails the build on any trap.
2. Extend unit tests or lightweight integration checks for the exception context helpers, especially around 16-byte spill slots.
3. Document allocator alignment guarantees in developer onboarding notes to avoid future regressions when touching memory subsystems.

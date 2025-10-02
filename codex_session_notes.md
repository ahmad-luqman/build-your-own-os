# Codex Session Notes - MiniOS Project

## Repository Snapshot
- Location: `/Users/ahmadluqman/src/build-your-own-os`
- Branch: `main` (ahead of `origin/main` by 1 commit `ba70f2b`)
- Latest local commit: "FIX: ls command now works - disabled optimization for ramfs_core.c" (build rule compiles `fs/ramfs/ramfs_core.c` with `-O0`).

## High-Level Status
- Official documentation is inconsistent:
  - `PROJECT_STATUS.md` claims MiniOS v1.0.0 with all seven phases complete plus Phase 8 optional roadmap.
  - `README.md` and `CURRENT_STATUS.md` still report Phase 5 as current focus (71% complete) and show shell pending.
- Filesystem and shell subsystems exist in source and appear integrated (kernel launches shell via `shell_main_task` in `src/kernel/main.c`).
- User programs under `src/userland/` have placeholder I/O (`user_printf` stub), suggesting execution path may need syscall wiring despite docs claiming full support.

## Key Documentation
- `README.md`: Intro, project goals, getting started (Phase 1 focus).
- `CURRENT_STATUS.md`: Sept 30 2024 snapshot; Phase 5 flagged complete but only 28/44 tests passing and Phase 6 marked "Ready to Begin".
- `PROJECT_STATUS.md`: Post-v1.0 celebration with build/test commands and Phase 8 expansion roadmap pointers.
- `docs/ARCHITECTURE.md`: System design overview (memory maps, process structure, VFS interfaces, driver framework).
- `docs/PHASE6_SHELL_DOCUMENTATION.md`: Shell layout (core/commands/parser/advanced), command reference, VM boot + testing instructions.
- Extensive handoff/context files (e.g., `PHASE_PROGRESS_TRACKER.md`, `PHASE4_SESSION_PROMPT.md`, `HANDOVER_*`) remain for historical trace.

## Code Structure Highlights
- `src/kernel/main.c`: Phase-gated initialization pipeline (memory → devices → filesystem → shell). Ends with `shell_main_task(NULL)` when fully enabled.
- `src/fs/vfs/vfs_core.c`: VFS lifecycle (register, mount, stats); logs heavily during boot, expects pre-registered FS types (ramfs/sfs).
- `src/shell/core/shell_core.c`: Registers 15+ built-in commands (ls, cat, ps, free, help, exit, etc.), maintains history, loops on `shell_read_command`.
- `src/kernel/process/process.c`: Static task pool + scheduler integration using round-robin slices; relies on `arch_setup_task_context` to seed stacks.
- `src/drivers/`: Timer (`arm64_timer.c`, `x86_64_timer.c`), UART (`pl011.c`, `16550.c`), interrupts (`gic.c`, `pic.c`, `idt.c`).
- `src/userland/`: `bin/`, `utils/`, `games/`, `lib/`; programs compile but current printf/syscall hooks are stubs.

## Build & Tooling
- `Makefile`: Cross-arch builds with arch-specific includes, integrates shell and filesystem sources, creates user programs + libc archive.
- Special rule forces `ramfs_core.c` to compile without optimizations (addresses `ls` failure).
- Scripts: `test_phase_3.sh`, `test_phase_4.sh`, `test_all_phases.sh`, `tools/test-vm.sh` for QEMU/UTM automation.
- Added `tests/test_ramfs_shell.sh` for automated RAMFS CRUD validation; expects timeout exit (handled internally).

## Recent History Overview (selected)
1. Phase releases tagged `v0.1.0` through `v1.0.0`; Phase 8.1 tagged `v8.1.0` for optional enhancements.
2. Phase 6 development commits (`bf0e786`, `bb00767`, `4eefce2`, `2910888`, `759fd2b`) implement interactive shell, parser, VFS integration.
3. Phase 5 stabilization (`895d207` → `788e045`) fixes RAMFS, block devices, and FD subsystem.
4. Latest series (`527e6f5`, `f56f1a7`, `ba70f2b`) resolves `ls` regression by disabling optimization for RAMFS.

## Observations / Action Items
- ✅ **FIXED**: x86_64 QEMU boot now works via GRUB multiboot2 ISO
- ✅ Created `tools/create-grub-iso.sh` for building bootable ISO images
- ✅ Updated `tests/test_ramfs_shell.sh` to use ISO for x86_64
- ⚠️  Kernel boots but crashes early in initialization (separate issue from boot path)
- Harmonize status/documentation files to reflect true current phase & capabilities
- RAMFS shell tests pass on ARM64 (`tests/test_ramfs_shell.sh arm64`)
- Confirm userland programs execute end-to-end (syscall layer may need finishing touches despite documentation claims)
- Consider trimming verbose boot logs or gating them behind debug flags for usability

## Recent Changes (Oct 2, 2024)
1. Installed `i686-elf-grub` and `xorriso` for ISO creation
2. Created `tools/create-grub-iso.sh` - builds GRUB2 multiboot bootable ISO
3. Modified `src/arch/x86_64/linker.ld` - improved .note section placement  
4. Updated `tests/test_ramfs_shell.sh` - x86_64 uses ISO instead of direct kernel
5. Kernel successfully boots via GRUB, enters 64-bit mode, but crashes early

See `X86_64_QEMU_BOOT_FIX.md` for complete details on the boot fix.

## Useful Commands
```bash
make ARCH=arm64 clean all
make ARCH=x86_64 clean all
./tools/test_phase_4.sh
./tools/test_phases.sh
bash tools/test-vm.sh arm64 20 true  # boots headless shell
```

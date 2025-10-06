# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

MiniOS is an educational operating system project supporting ARM64 and x86-64 architectures. The project follows a phased development approach with 7 main phases, currently at Phase 5 completion (File System Implementation). This is a learning OS designed to demonstrate core OS concepts clearly.

## GitHub Project Management

### Project Information
- **GitHub Project**: #9 (https://github.com/users/ahmad-luqman/projects/9)
- **Repository**: ahmad-luqman/build-your-own-os
- **Issue Tracking**: All development tasks are tracked as GitHub issues with labels
- **Milestones**: Phase-based milestones for tracking progress

### Essential gh CLI Commands

#### View Project Status
```bash
# Quick dashboard view
./project_dashboard.sh

# Detailed status
./project_status.sh

# List all issues
gh issue list --repo ahmad-luqman/build-your-own-os

# List high priority issues
gh issue list --label "high-priority" --state open

# List bugs
gh issue list --label "bug" --state open
```

#### Working with Issues
```bash
# Create a new issue
gh issue create --title "Title" --body "Description" --label "bug,shell,high-priority"

# View an issue
gh issue view <number>

# Edit an issue (add labels)
gh issue edit <number> --add-label "in-progress"

# Assign issue to yourself
gh issue edit <number> --add-assignee @me

# Close an issue when complete
gh issue close <number> --comment "Fixed in commit <hash>"

# Reopen an issue
gh issue reopen <number>
```

#### Project Management
```bash
# Add issue to project
gh project item-add 9 --owner ahmad-luqman --url https://github.com/ahmad-luqman/build-your-own-os/issues/<number>

# View project
open https://github.com/users/ahmad-luqman/projects/9
```

### Label System
Issues are organized with the following labels:

**Priority Labels:**
- `high-priority` - Critical issues needing immediate attention
- `medium-priority` - Important but not urgent
- `low-priority` - Nice to have improvements

**Type Labels:**
- `bug` - Something isn't working
- `feature` - New functionality
- `enhancement` - Improvements to existing features
- `documentation` - Documentation updates
- `testing` - Test-related tasks

**Component Labels:**
- `shell` - Shell/CLI related
- `sfs` - SFS filesystem
- `memory` - Memory management
- `performance` - Performance optimizations

**Phase Labels:**
- `phase-5` - File System
- `phase-6` - User Interface
- `phase-7` - Polish & Documentation

### Current High Priority Issues

When starting work, check these first:
1. **Bug Issues** - Always fix bugs before adding features
2. **High Priority Features** - Critical functionality needed
3. **Test Failures** - Keep tests passing

Use: `gh issue list --label "bug" --state open` or `./project_dashboard.sh`

### Development Workflow

1. **Starting Work on an Issue:**
   ```bash
   # Pick an issue
   gh issue list --label "bug" --state open

   # Assign to yourself
   gh issue edit <number> --add-assignee @me

   # Add in-progress label
   gh issue edit <number> --add-label "in-progress"

   # Create a branch (optional)
   git checkout -b issue-<number>-description
   ```

2. **During Development:**
   ```bash
   # Comment on progress
   gh issue comment <number> --body "Working on fix, found that..."

   # Link commits by mentioning issue
   git commit -m "Fix: Description (#<number>)"
   ```

3. **Completing an Issue:**
   ```bash
   # Close with comment
   gh issue close <number> --comment "Fixed in commit <hash>"

   # Or close via commit message
   git commit -m "Fixes #<number>: Description"
   ```

### Project Scripts

The following helper scripts are available:

- **`./project_dashboard.sh`** - Visual dashboard of project status
- **`./project_status.sh`** - Detailed status report
- **`./setup_github_labels.sh`** - Create/update labels (only run once)
- **`./setup_github_project.sh`** - Initial project setup (already complete)

## Build Commands

### Basic Build Operations
```bash
# Build for ARM64 (default)
make

# Build for x86-64
make ARCH=x86_64

# Clean build artifacts
make clean

# Build and test in VM
make test

# Build debug version with symbols and logging
make DEBUG=1

# Display build information
make info

# Quick build and test
make quick-test
```

### Testing Commands
```bash
# Phase-specific testing
./tools/test-phase1.sh      # Phase 1 foundation testing
./tools/test-phase3.sh      # Phase 3 memory testing
./tools/test-phase4.sh      # Phase 4 system services testing

# Multi-phase testing
./test_phases.sh            # Test all completed phases
./test_all_phases.sh        # Comprehensive testing
./demo_phases_12.sh         # Demo phases 1-2

# Debug session
make debug                  # Start debug session
```

## Architecture

### Cross-Platform Support
- **Primary Target**: ARM64 (AArch64) for UTM Virtual Machine on macOS
- **Secondary Target**: x86-64 for broader compatibility via QEMU
- **Build System**: Uses GNU Make with architecture-specific configuration files in `tools/build/arch-*.mk`

### Key Architectural Patterns
1. **Hardware Abstraction Layer (HAL)**: Clean separation between architecture-specific and independent code
2. **Microkernel-inspired design**: Minimal kernel with modular components
3. **Phase-based development**: Each phase builds upon previous functionality

### Source Code Structure
```
src/
├── arch/                 # Architecture-specific code (arm64/, x86_64/)
├── boot/                 # Bootloader implementations
├── kernel/               # Core kernel functionality
├── drivers/              # Device drivers (uart/, timer/, interrupt/)
├── fs/                   # File system implementations (vfs/, sfs/, block/, ramfs/)
├── shell/                # Shell implementation (core/, commands/, parser/, advanced/)
└── userland/             # User space applications and libraries
```

## Current Implementation Status

**Phase 5 Complete - File System Implementation**
- ✅ Phase 1: Foundation Setup - Cross-platform build system
- ✅ Phase 2: Bootloader Implementation - UEFI (ARM64) and Multiboot2 (x86-64)
- ✅ Phase 3: Memory Management - MMU/Paging with virtual memory
- ✅ Phase 4: System Services - Device drivers, processes, interrupts, syscalls
- ✅ Phase 5: File System - VFS layer, SFS implementation, block devices
- 🔄 Phase 6: User Interface - Shell implementation and CLI utilities
- ⏳ Phase 7: Polish & Documentation

## Development Workflow

### Cross-Compilation Setup
The build system automatically selects appropriate toolchains:
- **ARM64**: `aarch64-elf-gcc`, `aarch64-elf-ld`, `aarch64-elf-as`
- **x86-64**: `x86_64-elf-gcc`, `x86_64-elf-ld`, `nasm`

### Build Process
1. Architecture detection and toolchain selection
2. Source file compilation with architecture-specific flags
3. Linking using architecture-specific linker scripts
4. Bootloader build and image creation
5. VM testing and debugging

### Key Files to Understand
- `Makefile`: Main build configuration
- `src/kernel/kernel.h`: Core kernel structures and interfaces
- `src/include/`: Central header definitions
- `docs/ARCHITECTURE.md`: Detailed system architecture
- `docs/BUILD.md`: Build system documentation

## Important Implementation Details

### Memory Management
- Virtual memory with 4KB pages
- 4-level page tables on both architectures
- User/kernel space separation
- ARM64: 256TB address spaces, x86-64: 128TB address spaces

### System Call Interface
- ARM64: `svc #0` instruction, arguments in x0-x5, return in x0
- x86-64: `syscall` instruction, arguments in rdi, rsi, rdx, rcx, r8, r9, return in rax

### File System
- VFS abstraction layer supporting multiple file systems
- SFS (Simple File System) implementation
- Block device abstraction for storage
- **IMPORTANT**: RAMFS is mounted at `/` (root), not `/ramfs`

### Process Management
- Round-robin scheduling with priority levels
- Preemptive multitasking via timer interrupts
- Process structures with virtual memory spaces

## Known Issues and Bugs

### Current Bugs (Check GitHub for latest)
1. **Tab Completion Cursor Bug** (#1) - `ec[TAB]` produces `ccho` instead of `echo`
2. **Smoke Test Mount Path** (#2) - Tests expect `/ramfs` but RAMFS is mounted at `/`

### Working Features
- ✅ File operations: touch, cat, echo > file, ls, pwd, cd
- ✅ mv command (rename/move files)
- ✅ cp command (copy files)
- ✅ Basic tab completion (with known cursor bug)
- ✅ Directory operations on RAMFS

## Debugging and Development Tips

### Debug Builds
Use `make DEBUG=1` for:
- Debug symbols (-g)
- No optimization (-O0)
- Debug logging enabled (-DDEBUG)
- Log level set to DEBUG (0)

### VM Testing
- ARM64: Uses UTM-compatible disk images (.img)
- x86-64: Uses ISO images (.iso)
- Default timeout: 30 seconds for automated testing

### Common Development Tasks
- When adding new kernel features: Update appropriate files in `src/kernel/`
- Architecture-specific changes: Modify files in `src/arch/<arch>/`
- New device drivers: Add to `src/drivers/` with appropriate abstractions
- User applications: Add to `src/userland/` and update Makefile targets

## Code Contribution Guidelines

### Commit Message Standards
- **DO NOT** add any Claude Code attribution or signatures in commit messages
- Write clear, descriptive commit messages following conventional commit format
- Reference issues: `Fixes #<number>` or `Related to #<number>`
- Focus on what was changed and why, not who made the change
- Examples of good commit messages:
  - "Fix: Tab completion cursor position for unique matches (#1)"
  - "Feature: Add SFS file write support (#5)"
  - "Test: Update smoke test for correct RAMFS mount path (#2)"

### Development Principles
- Prioritize system stability and avoid breaking existing functionality
- Use test-driven development approach when implementing new features
- Test on both ARM64 and x86-64 architectures when applicable
- Maintain clean, readable code with proper documentation
- Always check high-priority bugs before adding new features

### Before Starting Work
1. Run `./project_dashboard.sh` to see current priorities
2. Check for assigned issues: `gh issue list --assignee @me`
3. Look for bugs first: `gh issue list --label "bug" --state open`
4. Ensure tests pass: `make test`

      IMPORTANT: this context may or may not be relevant to your tasks. You should not respond to this context unless it is highly relevant to your task.
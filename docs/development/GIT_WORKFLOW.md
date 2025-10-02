# Git Branching Strategy for MiniOS Development

## Overview

This document defines the git branching strategy for MiniOS development, ensuring clean separation between phases, easy collaboration, and the ability to work on different features independently.

## Branch Structure

### Main Branches

#### `main` (Production Branch)
- **Purpose**: Stable, working versions of completed phases
- **Content**: Only fully completed and tested phases
- **Protection**: Should be protected from direct pushes
- **Usage**: `git checkout main`

#### `develop` (Integration Branch)  
- **Purpose**: Integration of completed features and phases
- **Content**: Working but not necessarily polished code
- **Usage**: Base branch for creating feature branches

### Phase Branches

#### Phase-Specific Development Branches
Each major phase gets its own dedicated branch for development:

```
phase-1-foundation     ✅ COMPLETE - Foundation Setup
phase-2-bootloader     📋 READY    - Bootloader Implementation  
phase-3-kernel-core    📋 READY    - Kernel Core
phase-4-system-services 📋 READY   - System Services
phase-5-filesystem     📋 READY    - File System
phase-6-user-interface 📋 READY    - User Interface & Shell
phase-7-polish         📋 READY    - Polish & Documentation
```

#### Architecture-Specific Branches (Optional)
For parallel development on different architectures:

```
feature/arm64-optimization    - ARM64-specific improvements
feature/x86_64-optimization   - x86-64-specific improvements
feature/cross-arch-abstraction - Cross-platform improvements
```

### Feature Branches

#### Component-Specific Branches
For specific components within phases:

```
feature/memory-management     - Memory management implementation
feature/process-scheduler     - Process scheduling system
feature/device-drivers       - Device driver framework
feature/fat32-filesystem     - FAT32 implementation
feature/shell-implementation  - Shell and CLI tools
```

## Branch Naming Conventions

### Phase Branches
```
phase-{number}-{description}
Examples:
- phase-1-foundation
- phase-2-bootloader
- phase-3-kernel-core
```

### Feature Branches
```
feature/{component}-{description}
Examples:
- feature/memory-paging
- feature/uart-driver
- feature/elf-loader
```

### Bugfix Branches
```
bugfix/{issue}-{description}
Examples:
- bugfix/arm64-boot-hang
- bugfix/makefile-dependencies
```

### Documentation Branches
```
docs/{topic}-{description}
Examples:
- docs/phase2-implementation
- docs/api-reference
```

## Workflow for Phase Development

### Starting a New Phase

1. **Create Phase Branch from Main**
```bash
# Ensure main is up to date
git checkout main
git pull origin main

# Create new phase branch
git checkout -b phase-2-bootloader
git push -u origin phase-2-bootloader
```

2. **Set Up Phase Documentation**
```bash
# Create phase-specific documentation
touch docs/PHASE2_IMPLEMENTATION.md
touch docs/PHASE2_TESTING.md
touch PHASE2_PROGRESS.md

# Initial commit
git add docs/PHASE2_*.md PHASE2_PROGRESS.md
git commit -m "Phase 2: Initialize bootloader implementation phase

- Set up phase-specific documentation structure
- Ready for bootloader development
- Based on completed Phase 1 foundation"
```

### Working on Phase Features

3. **Create Feature Branches from Phase Branch**
```bash
# Start working on specific feature
git checkout phase-2-bootloader
git checkout -b feature/uefi-bootloader

# Work on feature
# Make commits
git add src/boot/uefi/
git commit -m "Implement UEFI bootloader entry point

- Add UEFI application structure
- Initialize boot services
- Set up memory map detection"

# Push feature branch
git push -u origin feature/uefi-bootloader
```

4. **Merge Feature into Phase Branch**
```bash
# Switch back to phase branch
git checkout phase-2-bootloader

# Merge completed feature
git merge feature/uefi-bootloader

# Push updated phase branch
git push origin phase-2-bootloader

# Clean up feature branch (optional)
git branch -d feature/uefi-bootloader
git push origin --delete feature/uefi-bootloader
```

### Completing a Phase

5. **Merge Phase into Main**
```bash
# Ensure phase is complete and tested
git checkout phase-2-bootloader
make test  # Run all tests
./tools/test-phase2.sh  # Phase-specific tests

# Switch to main and merge
git checkout main
git merge phase-2-bootloader

# Tag the completion
git tag -a v0.2.0 -m "Phase 2 Complete: Bootloader Implementation

Features:
- UEFI bootloader for ARM64
- Multiboot2 bootloader for x86-64
- Boot information structure
- Memory map detection
- Graphics mode initialization"

# Push everything
git push origin main
git push origin v0.2.0
```

## Current Branch Setup

### Phase 1 Completion (Retrospective Branching)

Since Phase 1 is already complete on main, let's create the proper branch structure:

```bash
# Create phase-1 branch from the Phase 1 completion commit
git checkout -b phase-1-foundation 8296f3d

# Tag Phase 1 completion
git tag -a v0.1.0 -m "Phase 1 Complete: Foundation Setup"

# Create develop branch
git checkout main
git checkout -b develop

# Push all branches and tags
git push -u origin phase-1-foundation
git push -u origin develop
git push origin v0.1.0
```

## Branch Protection Rules (Recommended)

### Main Branch Protection
```yaml
Protection Rules for 'main':
- Require pull request reviews
- Require status checks to pass
- Require branches to be up to date
- Restrict pushes to admins only
- Require signed commits (optional)
```

### Phase Branch Protection  
```yaml
Protection Rules for 'phase-*':
- Require pull request reviews (1 reviewer)
- Require status checks to pass
- Allow force pushes (for development flexibility)
```

## Commit Message Conventions

### Phase Commits
```
Phase {N}: {Brief description}

{Detailed description of changes}
{List of major features/components}
{References to issues or requirements}

Closes: #{issue-number}
Phase: {N}
```

### Feature Commits
```
{Component}: {Brief description}

{Detailed description}
{Technical details}
{Testing notes}

Feature: {feature-name}
Phase: {N}
```

### Example Commit Messages
```bash
# Phase completion
"Phase 2: Complete bootloader implementation

- UEFI bootloader with proper handoff
- Multiboot2 support for legacy systems  
- Memory map detection and validation
- Graphics mode initialization
- Cross-architecture boot protocol

Closes: #15, #16, #17
Phase: 2"

# Feature implementation
"bootloader: Add UEFI memory map detection

Implement EFI_BOOT_SERVICES->GetMemoryMap() to detect
available memory regions during boot process.

- Parse EFI memory descriptors
- Convert to internal memory map format
- Validate memory regions
- Pass to kernel via boot info structure

Feature: memory-detection
Phase: 2
Testing: Verified on QEMU and UTM"
```

## Documentation Per Branch

### Branch-Specific Documentation Structure
```
docs/
├── PHASE{N}_IMPLEMENTATION.md  # Implementation guide for phase
├── PHASE{N}_TESTING.md        # Testing procedures for phase
├── PHASE{N}_PROGRESS.md       # Progress tracking
└── branching/
    ├── BRANCH_SETUP.md        # How to set up branches
    ├── WORKFLOW.md            # Development workflow
    └── MERGE_STRATEGY.md      # Merge and release process
```

### Progress Tracking Files
Each phase branch should have:
```
PHASE{N}_PROGRESS.md           # Current status and todos
PHASE{N}_CHANGELOG.md          # Changes made during development
PHASE{N}_DECISIONS.md          # Design decisions and rationale
```

## Integration with Development Workflow

### Daily Development Workflow
```bash
# Start of day - sync with latest
git checkout phase-2-bootloader
git pull origin phase-2-bootloader

# Create feature branch for today's work
git checkout -b feature/boot-info-structure

# Work, commit, push
git add .
git commit -m "boot: Define boot information structure"
git push -u origin feature/boot-info-structure

# End of day - merge if feature is complete
git checkout phase-2-bootloader
git merge feature/boot-info-structure
git push origin phase-2-bootloader
```

### Testing Integration
```bash
# Before merging any branch
make clean
make test
./tools/test-phase{N}.sh

# Before merging to main
make clean
make test ARCH=arm64
make test ARCH=x86_64
./tools/test-all-phases.sh
```

## Release Management

### Version Numbering
```
v0.{phase}.{increment}

Examples:
v0.1.0  - Phase 1 complete
v0.1.1  - Phase 1 hotfix
v0.2.0  - Phase 2 complete
v0.7.0  - All phases complete (MVP)
v1.0.0  - Production ready
```

### Release Branches (For Major Milestones)
```
release/v0.2.0-bootloader    # Phase 2 release preparation
release/v0.7.0-mvp          # MVP release preparation  
release/v1.0.0-production   # Production release
```

## Benefits of This Strategy

### For Development
- **Parallel work**: Multiple phases can be developed simultaneously
- **Clean history**: Each phase has its own development history
- **Easy rollback**: Can revert to any phase state
- **Feature isolation**: Features developed in isolation, merged when ready

### For Learning
- **Progressive complexity**: Each phase builds on previous
- **Clear milestones**: Easy to track progress and achievements
- **Documentation**: Each phase fully documented
- **Reproducible**: Can recreate any phase state for learning

### for Collaboration
- **Clear ownership**: Phases can be assigned to different people
- **Review process**: Pull requests for quality control
- **Integration testing**: Automated testing at each merge
- **Release management**: Formal releases with proper versioning

This branching strategy provides structure while maintaining flexibility for the educational nature of the MiniOS project.
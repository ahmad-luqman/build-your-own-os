#!/bin/bash

# Setup GitHub Labels for MiniOS Project

echo "=== Setting up GitHub Labels ==="
echo ""

REPO="ahmad-luqman/build-your-own-os"

# Create labels with colors
echo "Creating labels..."

# Bug/Feature labels
gh label create "bug" --description "Something isn't working" --color "d73a4a" --repo $REPO --force || true
gh label create "enhancement" --description "New feature or request" --color "a2eeef" --repo $REPO --force || true
gh label create "feature" --description "New functionality" --color "0075ca" --repo $REPO --force || true

# Component labels
gh label create "shell" --description "Shell-related issues" --color "fbca04" --repo $REPO --force || true
gh label create "sfs" --description "SFS filesystem issues" --color "1d76db" --repo $REPO --force || true
gh label create "testing" --description "Testing-related tasks" --color "5319e7" --repo $REPO --force || true
gh label create "memory" --description "Memory management issues" --color "e99695" --repo $REPO --force || true
gh label create "reliability" --description "Reliability improvements" --color "b60205" --repo $REPO --force || true

# Priority labels
gh label create "high-priority" --description "High priority issue" --color "ff0000" --repo $REPO --force || true
gh label create "medium-priority" --description "Medium priority issue" --color "ff9900" --repo $REPO --force || true
gh label create "low-priority" --description "Low priority issue" --color "ffeb3b" --repo $REPO --force || true

# Phase labels
gh label create "phase-5" --description "Phase 5: File System" --color "c5def5" --repo $REPO --force || true
gh label create "phase-6" --description "Phase 6: User Interface" --color "bfd4f2" --repo $REPO --force || true
gh label create "phase-7" --description "Phase 7: Polish" --color "d4c5f9" --repo $REPO --force || true

# Other labels
gh label create "documentation" --description "Documentation improvements" --color "0052cc" --repo $REPO --force || true
gh label create "performance" --description "Performance optimizations" --color "684BFF" --repo $REPO --force || true
gh label create "tech-debt" --description "Technical debt items" --color "fef2c0" --repo $REPO --force || true
gh label create "code-quality" --description "Code quality improvements" --color "84b6eb" --repo $REPO --force || true
gh label create "architecture" --description "Architecture improvements" --color "006b75" --repo $REPO --force || true
gh label create "integration" --description "Integration testing" --color "0e8a16" --repo $REPO --force || true
gh label create "tools" --description "Development tools" --color "f9d0c4" --repo $REPO --force || true

echo "✅ Labels created"
echo ""

echo "=== Label Setup Complete! ==="
echo ""
echo "You can now run ./setup_github_project.sh to create issues with proper labels"
echo "Or view your existing project at: https://github.com/users/ahmad-luqman/projects/8"
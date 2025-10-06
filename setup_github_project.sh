#!/bin/bash

# MiniOS GitHub Project Setup Script
# Run this after: gh auth refresh -s project

set -e

echo "=== MiniOS GitHub Project Setup ==="
echo ""

# Check if gh is authenticated with project scope
echo "Checking GitHub CLI authentication..."
if ! gh auth status 2>&1 | grep -q "project"; then
    echo "❌ Missing project permissions!"
    echo "Please run: gh auth refresh -s project"
    exit 1
fi

echo "✅ GitHub CLI authenticated with project permissions"
echo ""

# Create the project
echo "Creating GitHub Project..."
PROJECT_NUMBER=$(gh project create \
    --owner ahmad-luqman \
    --title "MiniOS Development Tracker" \
    --format json | jq -r '.number')

if [ -z "$PROJECT_NUMBER" ]; then
    echo "❌ Failed to create project"
    exit 1
fi

echo "✅ Created project #$PROJECT_NUMBER"
echo ""

# Create issues for each item
echo "Creating GitHub Issues..."

# Bug Issues
gh issue create \
    --title "🐛 Tab Completion Cursor Bug" \
    --body "**Issue**: \`ec[TAB]\` produces \`ccho\` instead of \`echo\`\n\n**Location**: src/shell/core/shell_io.c\n\n**Type**: Bug\n**Priority**: High\n**Phase**: 6" \
    --label "bug,shell,phase-6,high-priority" || true

gh issue create \
    --title "🐛 Smoke Test Mount Path Issue" \
    --body "**Issue**: Test expects \`/ramfs\` but RAMFS is mounted at \`/\`\n\n**Location**: scripts/testing/test_arm64_ramfs_smoke.sh\n\n**Type**: Bug\n**Priority**: Medium" \
    --label "bug,testing,medium-priority" || true

# Shell Features
gh issue create \
    --title "✨ Enhanced Error Messages" \
    --body "**Goal**: User-friendly, contextual error messages\n\n**Examples**: \n- \"file 'test.txt' not found in '/current/dir'\"\n- Add command usage hints when arguments missing\n\n**Priority**: Medium" \
    --label "enhancement,shell,phase-6,medium-priority" || true

# SFS Core Operations
gh issue create \
    --title "📁 SFS File Creation Support" \
    --body "- Fix \`sfs_file_create()\` crash\n- Implement \`touch\` command support\n- Add proper inode allocation\n- Test file creation in root and subdirectories\n\n**Priority**: High" \
    --label "feature,sfs,phase-5,high-priority" || true

gh issue create \
    --title "📁 SFS File Write Operations" \
    --body "- Fix \`echo \"text\" > file\` crash on SFS\n- Implement safe write operations\n- Add block allocation for file data\n- Handle partial writes and buffering\n\n**Priority**: High" \
    --label "feature,sfs,phase-5,high-priority" || true

gh issue create \
    --title "📁 SFS File Read Operations" \
    --body "- Implement \`cat\` command support for SFS\n- Add file content reading\n- Handle file position/offset\n- Implement read buffering\n\n**Priority**: High" \
    --label "feature,sfs,phase-5,high-priority" || true

gh issue create \
    --title "📁 SFS File Deletion" \
    --body "- Implement \`rm\` command for SFS\n- Free allocated blocks\n- Update directory entries\n- Handle inode cleanup\n\n**Priority**: Medium" \
    --label "feature,sfs,phase-5,medium-priority" || true

# Directory Operations
gh issue create \
    --title "📂 SFS Directory Listing" \
    --body "- Fix \`ls\` command on SFS mounts\n- Parse directory entries correctly\n- Display file metadata (size, permissions)\n- Support \`ls -l\` for detailed view\n\n**Priority**: High" \
    --label "feature,sfs,phase-5,high-priority" || true

gh issue create \
    --title "📂 SFS Directory Creation" \
    --body "- Implement \`mkdir\` for SFS\n- Allocate directory inodes\n- Initialize directory blocks\n- Update parent directory\n\n**Priority**: Medium" \
    --label "feature,sfs,phase-5,medium-priority" || true

gh issue create \
    --title "📂 SFS Directory Deletion" \
    --body "- Implement \`rmdir\` for SFS\n- Check for empty directories\n- Recursive deletion option\n- Handle non-empty directory errors\n\n**Priority**: Medium" \
    --label "feature,sfs,phase-5,medium-priority" || true

# Reliability Issues
gh issue create \
    --title "🔧 Comprehensive Error Checking" \
    --body "- Add error checking to all SFS operations\n- Implement graceful error recovery\n- Add filesystem consistency checks\n- Handle disk full scenarios\n\n**Priority**: Medium" \
    --label "enhancement,reliability,sfs,medium-priority" || true

gh issue create \
    --title "🔧 Memory Management Audit" \
    --body "- Audit all kmalloc/kfree calls\n- Add memory leak detection\n- Implement block cache management\n- Add memory usage statistics\n\n**Priority**: High" \
    --label "enhancement,memory,reliability,high-priority" || true

# Testing
gh issue create \
    --title "🧪 SFS Unit Test Suite" \
    --body "- Create comprehensive test suite for SFS\n- Test all filesystem operations\n- Edge case testing\n- Error condition testing\n\n**Priority**: Medium" \
    --label "testing,sfs,medium-priority" || true

gh issue create \
    --title "🧪 Integration Tests" \
    --body "- Test with shell commands\n- Test with multiple users/processes\n- Test mount/unmount cycles\n- Test filesystem limits\n\n**Priority**: Medium" \
    --label "testing,integration,medium-priority" || true

echo "✅ Issues created"
echo ""

# Add issues to project
echo "Adding issues to project board..."
ISSUES=$(gh issue list --limit 100 --json number -q '.[].number')

for ISSUE in $ISSUES; do
    gh project item-add $PROJECT_NUMBER --owner ahmad-luqman --url "https://github.com/ahmad-luqman/build-your-own-os/issues/$ISSUE" || true
done

echo "✅ Issues added to project"
echo ""

# Create milestones
echo "Creating milestones..."

gh api repos/ahmad-luqman/build-your-own-os/milestones \
    --method POST \
    -f title="Phase 6: Shell Completion" \
    -f description="Complete shell features and UI improvements" \
    -f due_on="2025-10-15T00:00:00Z" || true

gh api repos/ahmad-luqman/build-your-own-os/milestones \
    --method POST \
    -f title="SFS Core Operations" \
    -f description="Basic file and directory operations for SFS" \
    -f due_on="2025-10-20T00:00:00Z" || true

gh api repos/ahmad-luqman/build-your-own-os/milestones \
    --method POST \
    -f title="SFS Reliability" \
    -f description="Error handling and robustness improvements" \
    -f due_on="2025-10-27T00:00:00Z" || true

echo "✅ Milestones created"
echo ""

echo "=== Setup Complete! ==="
echo ""
echo "📊 View your project at: https://github.com/users/ahmad-luqman/projects/$PROJECT_NUMBER"
echo ""
echo "Next steps:"
echo "1. Visit the project URL above"
echo "2. Configure the project board columns (Backlog, In Progress, Review, Done)"
echo "3. Set up automation rules for moving cards"
echo "4. Start working on high-priority issues!"
echo ""
echo "To add more issues later, use:"
echo "  gh issue create --title \"Title\" --body \"Description\" --label \"labels\""
echo "  gh project item-add $PROJECT_NUMBER --owner ahmad-luqman --url <issue-url>"
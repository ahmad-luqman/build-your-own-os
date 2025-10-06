#!/bin/bash

# MiniOS Project Status Viewer

echo "=== MiniOS Development Status ==="
echo ""

REPO="ahmad-luqman/build-your-own-os"
PROJECT_NUMBER=9

# Show recent issues
echo "📋 Recent Issues Created:"
gh issue list --repo $REPO --limit 10 --state all --json number,title,labels,state --jq '.[] | "  #\(.number): \(.title) [\(.state)]"'
echo ""

# Show project info
echo "📊 Project Board: https://github.com/users/ahmad-luqman/projects/$PROJECT_NUMBER"
echo ""

# Show milestones
echo "🎯 Milestones:"
gh api repos/$REPO/milestones --jq '.[] | "  - \(.title) (Due: \(.due_on // "No due date"))"'
echo ""

# Count issues by state
echo "📈 Issue Statistics:"
OPEN_COUNT=$(gh issue list --repo $REPO --state open --json number --jq '. | length')
CLOSED_COUNT=$(gh issue list --repo $REPO --state closed --json number --jq '. | length')
echo "  Open Issues: $OPEN_COUNT"
echo "  Closed Issues: $CLOSED_COUNT"
echo ""

# Show high priority issues
echo "🔴 High Priority Issues:"
gh issue list --repo $REPO --label "high-priority" --state open --json number,title --jq '.[] | "  #\(.number): \(.title)"'
echo ""

echo "=== Quick Actions ==="
echo "• Add label to existing issues: gh issue edit <number> --add-label \"label1,label2\""
echo "• View project: open https://github.com/users/ahmad-luqman/projects/$PROJECT_NUMBER"
echo "• Create new issue: gh issue create --title \"Title\" --body \"Body\""
echo "• Add issue to project: gh project item-add $PROJECT_NUMBER --owner ahmad-luqman --url <issue-url>"
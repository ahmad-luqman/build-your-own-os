#!/bin/bash

# MiniOS Project Management Dashboard

REPO="ahmad-luqman/build-your-own-os"
PROJECT_NUMBER=9

# Disable pager for all gh commands
export PAGER=cat

clear
echo "╔════════════════════════════════════════════════════════════════════════╗"
echo "║                    MiniOS Development Dashboard                         ║"
echo "╚════════════════════════════════════════════════════════════════════════╝"
echo ""

# Project Link
echo "📊 Project Board: https://github.com/users/ahmad-luqman/projects/$PROJECT_NUMBER"
echo ""

# Issue Statistics
echo "┌─────────────────────────── Issue Statistics ───────────────────────────┐"
OPEN_COUNT=$(gh issue list --repo $REPO --state open --json number --jq '. | length')
CLOSED_COUNT=$(gh issue list --repo $REPO --state closed --json number --jq '. | length')
TOTAL_COUNT=$((OPEN_COUNT + CLOSED_COUNT))
if [ $TOTAL_COUNT -gt 0 ]; then
    PROGRESS=$((CLOSED_COUNT * 100 / TOTAL_COUNT))
else
    PROGRESS=0
fi
echo "│ 📈 Open: $OPEN_COUNT | Closed: $CLOSED_COUNT | Total: $TOTAL_COUNT | Progress: $PROGRESS%"
echo "└─────────────────────────────────────────────────────────────────────────┘"
echo ""

# High Priority Issues
echo "┌─────────────────────────── 🔴 High Priority ──────────────────────────┐"
HIGH_PRIORITY=$(gh issue list --repo $REPO --label "high-priority" --state open --json number,title --jq '.[] | "│ #\(.number): \(.title)"' | head -5)
if [ -z "$HIGH_PRIORITY" ]; then
    echo "│ No high priority issues"
else
    echo "$HIGH_PRIORITY"
fi
echo "└─────────────────────────────────────────────────────────────────────────┘"
echo ""

# Current Bugs
echo "┌──────────────────────────── 🐛 Active Bugs ───────────────────────────┐"
BUGS=$(gh issue list --repo $REPO --label "bug" --state open --json number,title --jq '.[] | "│ #\(.number): \(.title)"' | head -5)
if [ -z "$BUGS" ]; then
    echo "│ No active bugs"
else
    echo "$BUGS"
fi
echo "└─────────────────────────────────────────────────────────────────────────┘"
echo ""

# Milestones
echo "┌──────────────────────────── 🎯 Milestones ────────────────────────────┐"
gh api repos/$REPO/milestones --jq '.[] | "│ \(.title) - Due: \(.due_on // "No due date" | split("T")[0])"' | head -5
echo "└─────────────────────────────────────────────────────────────────────────┘"
echo ""

# Recent Activity (last 5 issues)
echo "┌───────────────────────── 📋 Recent Activity ──────────────────────────┐"
gh issue list --repo $REPO --limit 5 --state all --json number,title,state,updatedAt --jq '.[] | "│ #\(.number): \(.title) [\(.state)]"'
echo "└─────────────────────────────────────────────────────────────────────────┘"
echo ""

# Quick Commands
echo "┌───────────────────────── 🚀 Quick Commands ───────────────────────────┐"
echo "│ • View project board:  open https://github.com/users/ahmad-luqman/projects/$PROJECT_NUMBER"
echo "│ • Create issue:        gh issue create"
echo "│ • Close issue:         gh issue close <number>"
echo "│ • Start work:          gh issue edit <number> --add-assignee @me"
echo "│ • View issue:          gh issue view <number>"
echo "└─────────────────────────────────────────────────────────────────────────┘"
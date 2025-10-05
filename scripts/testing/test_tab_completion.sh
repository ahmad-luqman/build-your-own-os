#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT=$(cd "$(dirname "$0")/../.." && pwd)
cd "$REPO_ROOT"

BUILD_DIR="tests/build"
mkdir -p "$BUILD_DIR"

# Compile unit tests for tab completion logic (host-side)
cc -Isrc -Isrc/include -std=c11 \
   tests/test_shell_tab_completion.c \
   src/shell/core/shell_completion.c \
   -o "$BUILD_DIR/tab_completion_tests"

# Run the unit tests
"$BUILD_DIR/tab_completion_tests"

echo "\nManual integration checklist:"
echo " 1. Build MiniOS and boot in QEMU."
echo " 2. At the shell prompt, press TAB on an empty line to list commands."
echo " 3. Type partial commands (e.g., 'ec') and press TAB to confirm auto-complete."
echo " 4. Verify multiple matches show options and no-match triggers bell/no-op."
echo " 5. Confirm existing shell features (redirection, pipes) still operate."

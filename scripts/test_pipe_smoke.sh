#!/bin/bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

echo "Testing basic pipe functionality..."

# Build and test pipes
make kernel ARCH=arm64

echo "Testing: echo 'smoke test' | cat"
cat <<'EOF' | ./tools/test-vm.sh arm64 45
MiniOS> echo "smoke test" | cat
MiniOS> ls /tmp | cat
MiniOS> echo "pipe test" | cat
MiniOS> exit
EOF

echo "Testing: ls | cat"
cat <<'EOF' | ./tools/test-vm.sh arm64 45
MiniOS> ls | cat
MiniOS> exit
EOF

echo "Testing: pwd | cat"
cat <<'EOF' | ./tools/test-vm.sh arm64 45
MiniOS> pwd | cat
MiniOS> exit
EOF

echo "Pipe smoke test complete!"
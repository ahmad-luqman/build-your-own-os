#!/bin/bash
# MiniOS Debug Script
# Automated debugging setup with GDB and QEMU

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"

# Default values
ARCH=${1:-arm64}
GDB_PORT=${2:-1234}

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log() {
    echo -e "${BLUE}[DEBUG]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[DEBUG]${NC} $1"
}

error() {
    echo -e "${RED}[DEBUG]${NC} $1"
}

success() {
    echo -e "${GREEN}[DEBUG]${NC} $1"
}

# Validate architecture and set file paths
case $ARCH in
    arm64)
        KERNEL_ELF="$BUILD_DIR/$ARCH/kernel.elf"
        IMAGE_FILE="$BUILD_DIR/$ARCH/minios.img"
        GDB_CMD="aarch64-elf-gdb"
        QEMU_CMD="qemu-system-aarch64"
        QEMU_ARGS="-machine virt -cpu cortex-a72 -m 512M"
        QEMU_ARGS="$QEMU_ARGS -drive file=$IMAGE_FILE,format=raw,if=virtio"
        ;;
    x86_64)
        KERNEL_ELF="$BUILD_DIR/$ARCH/kernel.elf"
        IMAGE_FILE="$BUILD_DIR/$ARCH/minios.iso"
        GDB_CMD="x86_64-elf-gdb"
        QEMU_CMD="qemu-system-x86_64"
        QEMU_ARGS="-m 512M"
        if [[ "$IMAGE_FILE" == *.iso ]]; then
            QEMU_ARGS="$QEMU_ARGS -cdrom $IMAGE_FILE -boot d"
        else
            QEMU_ARGS="$QEMU_ARGS -drive file=$IMAGE_FILE,format=raw"
        fi
        ;;
    *)
        error "Unknown architecture: $ARCH"
        echo "Usage: $0 {arm64|x86_64} [gdb_port]"
        exit 1
        ;;
esac

# Check prerequisites
check_prerequisites() {
    local missing=0
    
    # Check kernel ELF
    if [ ! -f "$KERNEL_ELF" ]; then
        error "Kernel ELF not found: $KERNEL_ELF"
        echo "Run 'make ARCH=$ARCH' first"
        missing=1
    fi
    
    # Check image
    if [ ! -f "$IMAGE_FILE" ]; then
        error "Image file not found: $IMAGE_FILE"
        echo "Run 'make ARCH=$ARCH' first"
        missing=1
    fi
    
    # Check QEMU
    if ! command -v "$QEMU_CMD" &> /dev/null; then
        error "$QEMU_CMD not found"
        echo "Install QEMU: brew install qemu"
        missing=1
    fi
    
    # Check GDB (optional)
    if ! command -v "$GDB_CMD" &> /dev/null; then
        warn "$GDB_CMD not found"
        echo "Install cross-GDB for better debugging experience"
        echo "  ARM64: brew install aarch64-elf-gdb"
        echo "  x86-64: brew install x86_64-elf-gdb"
    fi
    
    return $missing
}

# Create GDB init script
create_gdb_init() {
    local gdb_init="$BUILD_DIR/.gdbinit-$ARCH"
    
    log "Creating GDB init script: $gdb_init"
    
    cat > "$gdb_init" << EOF
# MiniOS GDB Configuration for $ARCH

# Connect to QEMU
target remote localhost:$GDB_PORT

# Load kernel symbols
file $KERNEL_ELF

# Architecture-specific settings
EOF

    case $ARCH in
        arm64)
            cat >> "$gdb_init" << EOF
set architecture aarch64

# Common ARM64 breakpoints
# b *0x40080000    # Kernel entry (adjust as needed)
# b kernel_main    # Main kernel function

EOF
            ;;
        x86_64)
            cat >> "$gdb_init" << EOF
set architecture i386:x86-64

# Common x86-64 breakpoints  
# b *0x100000      # Kernel entry (adjust as needed)
# b kernel_main    # Main kernel function

EOF
            ;;
    esac

    cat >> "$gdb_init" << EOF
# Useful commands
define reload
    file $KERNEL_ELF
    symbol-file $KERNEL_ELF
end

define reset
    monitor system_reset
end

# Display settings
set disassembly-flavor intel
set print pretty on
set pagination off

# Ready to debug
echo \nMiniOS Debug Session Ready!\n
echo Commands:\n
echo   continue  - Start/continue execution\n
echo   step      - Step one instruction\n
echo   info reg  - Show registers\n
echo   bt        - Show backtrace\n
echo   reload    - Reload symbols\n
echo   reset     - Reset system\n
echo \n
EOF

    echo "$gdb_init"
}

# Start QEMU with GDB server
start_qemu_debug() {
    local full_cmd="$QEMU_CMD $QEMU_ARGS"
    full_cmd="$full_cmd -gdb tcp::$GDB_PORT -S"
    full_cmd="$full_cmd -nographic -serial mon:stdio"
    
    log "Starting QEMU with GDB server"
    log "Command: $full_cmd"
    log "GDB server will listen on port $GDB_PORT"
    
    # Create log file
    local log_file="$BUILD_DIR/qemu-debug-$ARCH.log"
    mkdir -p "$(dirname "$log_file")"
    
    log "QEMU output will be logged to: $log_file"
    log ""
    log "QEMU is starting and waiting for GDB connection..."
    log "In another terminal, run:"
    log "  cd $PROJECT_ROOT"
    log "  $GDB_CMD -x $BUILD_DIR/.gdbinit-$ARCH"
    log ""
    log "Press Ctrl+C to stop QEMU"
    log "=" * 50
    
    # Start QEMU
    exec $full_cmd 2>&1 | tee "$log_file"
}

# Interactive debug session
start_interactive_debug() {
    local gdb_init
    gdb_init=$(create_gdb_init)
    
    # Create a wrapper script for easy reuse
    local debug_script="$BUILD_DIR/debug-$ARCH.sh"
    cat > "$debug_script" << EOF
#!/bin/bash
# Generated debug script for MiniOS ($ARCH)

# Start QEMU in background
echo "Starting QEMU..."
$QEMU_CMD $QEMU_ARGS -gdb tcp::$GDB_PORT -S -nographic -serial mon:stdio &
QEMU_PID=\$!

# Give QEMU time to start
sleep 2

# Start GDB
echo "Starting GDB..."
$GDB_CMD -x "$gdb_init"

# Clean up
echo "Stopping QEMU..."
kill \$QEMU_PID 2>/dev/null || true
EOF
    chmod +x "$debug_script"
    
    log "Created debug script: $debug_script"
    log "You can run it later with: bash $debug_script"
    
    # Ask user preference
    echo ""
    echo "Debug options:"
    echo "1. Start QEMU and wait for GDB connection (manual)"
    echo "2. Start integrated debug session (automatic)"
    echo "3. Show debug commands and exit"
    read -p "Choose option [1-3]: " choice
    
    case $choice in
        1)
            start_qemu_debug
            ;;
        2)
            log "Starting integrated debug session..."
            bash "$debug_script"
            ;;
        3)
            show_debug_help
            ;;
        *)
            log "Invalid choice, starting manual mode..."
            start_qemu_debug
            ;;
    esac
}

# Show debug help
show_debug_help() {
    cat << EOF

MiniOS Debug Help
================

Manual Debug Session:
1. Terminal 1: Start QEMU with GDB server
   $QEMU_CMD $QEMU_ARGS -gdb tcp::$GDB_PORT -S -nographic

2. Terminal 2: Connect GDB
   cd $PROJECT_ROOT
   $GDB_CMD -x $BUILD_DIR/.gdbinit-$ARCH

Common GDB Commands:
  (gdb) continue          # Start execution
  (gdb) break kernel_main # Set breakpoint
  (gdb) info registers   # Show CPU registers
  (gdb) x/10i \$pc        # Disassemble at PC
  (gdb) step             # Step one instruction
  (gdb) backtrace        # Show call stack

QEMU Monitor Commands (Ctrl+A, C):
  (qemu) info registers  # Show registers
  (qemu) info mem        # Show memory mapping
  (qemu) system_reset    # Reset system

Files Created:
  $BUILD_DIR/.gdbinit-$ARCH     # GDB configuration
  $BUILD_DIR/debug-$ARCH.sh     # Debug script
  $BUILD_DIR/qemu-debug-$ARCH.log # QEMU output

EOF
}

# Main execution
main() {
    log "MiniOS Debug Session ($ARCH architecture)"
    
    # Check prerequisites
    if ! check_prerequisites; then
        exit 1
    fi
    
    log "Files:"
    log "  Kernel: $KERNEL_ELF"
    log "  Image:  $IMAGE_FILE"
    log "  QEMU:   $QEMU_CMD"
    log "  GDB:    $GDB_CMD"
    
    # Handle arguments
    if [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
        show_debug_help
        exit 0
    fi
    
    # Start debug session
    start_interactive_debug
}

# Handle help
if [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
    echo "MiniOS Debug Script"
    echo ""
    echo "Usage: $0 [ARCH] [GDB_PORT]"
    echo ""
    echo "Arguments:"
    echo "  ARCH      Target architecture (arm64, x86_64) [default: arm64]"
    echo "  GDB_PORT  GDB server port [default: 1234]"
    echo ""
    echo "Examples:"
    echo "  $0                # Debug ARM64 on port 1234"
    echo "  $0 x86_64         # Debug x86-64 on port 1234"  
    echo "  $0 arm64 5678     # Debug ARM64 on port 5678"
    echo ""
    exit 0
fi

# Run main function
main "$@"
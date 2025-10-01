#!/bin/bash
# MiniOS VM Testing Script
# Automated testing in virtual machines

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
VM_CONFIG_DIR="$PROJECT_ROOT/vm-configs"

# Default values
ARCH=${1:-arm64}
TIMEOUT=${2:-30}
HEADLESS=${3:-true}

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log() {
    echo -e "${BLUE}[VM-TEST]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[VM-TEST]${NC} $1"
}

error() {
    echo -e "${RED}[VM-TEST]${NC} $1"
}

success() {
    echo -e "${GREEN}[VM-TEST]${NC} $1"
}

# Validate architecture
case $ARCH in
    arm64)
        IMAGE_FILE="$BUILD_DIR/$ARCH/minios.img"
        KERNEL_FILE="$BUILD_DIR/$ARCH/kernel.elf"
        ;;
    x86_64)
        IMAGE_FILE="$BUILD_DIR/$ARCH/minios.iso"
        KERNEL_FILE="$BUILD_DIR/$ARCH/kernel.elf"
        ;;
    *)
        error "Unknown architecture: $ARCH"
        echo "Usage: $0 {arm64|x86_64} [timeout_seconds] [headless]"
        exit 1
        ;;
esac

# Check if image exists
if [ ! -f "$IMAGE_FILE" ]; then
    error "Image file not found: $IMAGE_FILE"
    echo "Run 'make ARCH=$ARCH' first to build the image"
    exit 1
fi

# Check if kernel exists (for debugging info)
if [ ! -f "$KERNEL_FILE" ]; then
    warn "Kernel ELF not found: $KERNEL_FILE"
fi

log "Testing MiniOS ($ARCH architecture)"
log "Image: $IMAGE_FILE"
log "Timeout: ${TIMEOUT}s"
log "Headless: $HEADLESS"

# Build VM command based on architecture
build_vm_command() {
    local base_cmd=""
    local display_args=""
    
    if [ "$HEADLESS" = "true" ]; then
        display_args="-nographic -serial mon:stdio"
    else
        display_args="-serial stdio"
    fi
    
    case $ARCH in
        arm64)
            base_cmd="qemu-system-aarch64"
            base_cmd="$base_cmd -machine virt -cpu cortex-a72"
            base_cmd="$base_cmd -m 512M"
            # Use direct kernel boot for QEMU (faster and more reliable)
            if [ -f "$KERNEL_FILE" ]; then
                base_cmd="$base_cmd -kernel $KERNEL_FILE"
                base_cmd="$base_cmd -append 'console=uart,mmio,0x9000000'"
                echo "Using direct kernel boot with: $KERNEL_FILE"
            else
                base_cmd="$base_cmd -drive file=$IMAGE_FILE,format=raw,if=virtio"
                echo "Using disk image boot: $IMAGE_FILE"
            fi
            base_cmd="$base_cmd $display_args"
            ;;
        x86_64)
            base_cmd="qemu-system-x86_64"
            base_cmd="$base_cmd -m 512M"
            if [[ "$IMAGE_FILE" == *.iso ]]; then
                base_cmd="$base_cmd -cdrom $IMAGE_FILE -boot d"
            else
                base_cmd="$base_cmd -drive file=$IMAGE_FILE,format=raw"
            fi
            base_cmd="$base_cmd $display_args"
            ;;
    esac
    
    echo "$base_cmd"
}

# Check if QEMU is available
check_qemu() {
    local qemu_cmd=""
    case $ARCH in
        arm64) qemu_cmd="qemu-system-aarch64" ;;
        x86_64) qemu_cmd="qemu-system-x86_64" ;;
    esac
    
    if ! command -v "$qemu_cmd" &> /dev/null; then
        error "$qemu_cmd not found"
        echo "Install QEMU: brew install qemu"
        return 1
    fi
    
    log "Using: $qemu_cmd ($(command -v "$qemu_cmd"))"
    return 0
}

# Run VM with timeout and capture output
run_vm_test() {
    local vm_cmd="$1"
    local log_file="$BUILD_DIR/test-output-$ARCH.log"
    
    log "Starting VM test..."
    log "Command: $vm_cmd"
    log "Log file: $log_file"
    
    # Create log directory
    mkdir -p "$(dirname "$log_file")"
    
    # Run VM with timeout
    if timeout "$TIMEOUT" bash -c "$vm_cmd" 2>&1 | tee "$log_file"; then
        local exit_code=$?
        if [ $exit_code -eq 124 ]; then
            warn "VM test timed out after ${TIMEOUT}s"
            return 124
        else
            success "VM test completed successfully"
            return 0
        fi
    else
        local exit_code=$?
        error "VM test failed with exit code: $exit_code"
        return $exit_code
    fi
}

# Analyze test results
analyze_results() {
    local log_file="$BUILD_DIR/test-output-$ARCH.log"
    
    if [ ! -f "$log_file" ]; then
        warn "No log file found for analysis"
        return
    fi
    
    log "Analyzing test results..."

    # Look for common success/failure patterns
    if grep -q "kernel panic\|Kernel panic" "$log_file"; then
        error "Kernel panic detected!"
    fi

    if grep -q "UNHANDLED EXCEPTION" "$log_file"; then
        error "Unhandled exception detected!"
        # Show the exception context
        grep -A5 "UNHANDLED EXCEPTION" "$log_file"
    fi

    if grep -q "MiniOS" "$log_file"; then
        success "MiniOS banner found in output"
    fi

    if grep -q "Shell>" "$log_file"; then
        success "Shell prompt detected"
    fi

    if grep -q "SUCCESS: Phases 1-2" "$log_file"; then
        success "Phase 1-2 testing successful!"
    fi
    
    # Show last few lines of output
    log "Last 10 lines of output:"
    tail -10 "$log_file" | while IFS= read -r line; do
        echo "  $line"
    done
}

# Main execution
main() {
    log "Starting MiniOS VM test"
    
    # Check prerequisites
    if ! check_qemu; then
        exit 1
    fi
    
    # Build VM command
    local vm_command
    vm_command=$(build_vm_command)
    
    # Run test
    local test_result=0
    run_vm_test "$vm_command" || test_result=$?
    
    # Analyze results
    analyze_results
    
    # Report final status
    case $test_result in
        0)
            success "VM test PASSED"
            ;;
        124)
            warn "VM test TIMED OUT (this may be normal for interactive tests)"
            ;;
        *)
            error "VM test FAILED (exit code: $test_result)"
            ;;
    esac
    
    return $test_result
}

# Handle script arguments
if [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
    echo "MiniOS VM Testing Script"
    echo ""
    echo "Usage: $0 [ARCH] [TIMEOUT] [HEADLESS]"
    echo ""
    echo "Arguments:"
    echo "  ARCH     Target architecture (arm64, x86_64) [default: arm64]"
    echo "  TIMEOUT  Test timeout in seconds [default: 30]"
    echo "  HEADLESS Headless mode (true, false) [default: true]"
    echo ""
    echo "Examples:"
    echo "  $0                    # Test ARM64, 30s timeout, headless"
    echo "  $0 x86_64             # Test x86-64, 30s timeout, headless"
    echo "  $0 arm64 60           # Test ARM64, 60s timeout, headless"
    echo "  $0 arm64 30 false     # Test ARM64, 30s timeout, with GUI"
    echo ""
    echo "Prerequisites:"
    echo "  - QEMU installed (brew install qemu)"
    echo "  - Built MiniOS image (make ARCH=<arch>)"
    exit 0
fi

# Run main function
main "$@"
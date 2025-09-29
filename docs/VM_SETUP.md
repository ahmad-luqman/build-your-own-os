# Virtual Machine Setup for MiniOS Development

## Overview

This guide covers setting up virtual machines for testing MiniOS on both ARM64 (UTM on macOS) and x86-64 (QEMU) architectures.

## UTM Setup (ARM64 - Primary Development Target)

### Installation
```bash
# Install via Homebrew
brew install --cask utm

# Or download from Mac App Store or https://mac.getutm.app/
```

### Creating ARM64 VM for MiniOS

1. **Open UTM** and click "Create a New Virtual Machine"

2. **Select "Virtualize"** (for Apple Silicon Macs) or "Emulate" (for Intel Macs)

3. **Operating System**: Choose "Other"

4. **VM Configuration**:
   - **Architecture**: ARM64 (aarch64)
   - **System**: 
     - Boot: UEFI
     - Machine: virt-4.0 (or latest)
   - **CPU**: 
     - Cores: 1-2 (for development)
     - Enable hardware acceleration (if available)
   - **Memory**: 512MB - 1GB
   - **Storage**: 
     - Create new disk: 2GB
     - Interface: VirtIO

5. **Network**: 
   - Mode: Shared Network
   - Enable: Yes

6. **Display**:
   - Console Mode: Enable
   - Serial: Enable (for debug output)

### UTM VM Configuration File
Save this configuration as `vm-configs/minios-arm64.utm`:

```json
{
  "name": "MiniOS ARM64",
  "architecture": "aarch64",
  "machine": "virt-4.0",
  "cpu": {
    "cores": 1,
    "threads": 1
  },
  "memory": 512,
  "drives": [
    {
      "name": "disk0", 
      "type": "disk",
      "interface": "virtio",
      "size": 2048
    }
  ],
  "network": {
    "mode": "shared"
  },
  "display": {
    "type": "console"
  },
  "serial": {
    "enabled": true,
    "mode": "builtin"
  },
  "boot": {
    "type": "uefi"
  }
}
```

## QEMU Setup (Cross-Platform Testing)

### Installation
```bash
# macOS
brew install qemu

# Verify installation
qemu-system-aarch64 --version
qemu-system-x86_64 --version
```

### ARM64 QEMU Configuration

#### Basic ARM64 Test
```bash
#!/bin/bash
# vm-configs/qemu-arm64.sh

qemu-system-aarch64 \
    -machine virt \
    -cpu cortex-a72 \
    -m 512M \
    -nographic \
    -serial mon:stdio \
    -kernel path/to/kernel.elf \
    -initrd path/to/initrd.img \
    -append "console=ttyAMA0"
```

#### ARM64 with Graphics
```bash
#!/bin/bash  
# vm-configs/qemu-arm64-gui.sh

qemu-system-aarch64 \
    -machine virt \
    -cpu cortex-a72 \
    -m 512M \
    -device VGA \
    -kernel path/to/kernel.elf \
    -initrd path/to/initrd.img \
    -append "console=tty0"
```

### x86-64 QEMU Configuration

#### Basic x86-64 Test
```bash
#!/bin/bash
# vm-configs/qemu-x86_64.sh

qemu-system-x86_64 \
    -m 512M \
    -nographic \
    -serial mon:stdio \
    -kernel path/to/kernel.elf \
    -initrd path/to/initrd.img \
    -append "console=ttyS0"
```

#### x86-64 with ISO Boot
```bash
#!/bin/bash
# vm-configs/qemu-x86_64-iso.sh

qemu-system-x86_64 \
    -m 512M \
    -cdrom minios.iso \
    -boot d \
    -serial mon:stdio
```

## Development VM Configurations

### Debug Configuration (ARM64)
```bash
#!/bin/bash
# vm-configs/debug-arm64.sh

qemu-system-aarch64 \
    -machine virt \
    -cpu cortex-a72 \
    -m 512M \
    -nographic \
    -serial mon:stdio \
    -kernel build/kernel-arm64.elf \
    -initrd build/initrd.img \
    -gdb tcp::1234 \
    -S \
    -d guest_errors,unimp \
    -append "debug console=ttyAMA0"
```

### Debug Configuration (x86-64)
```bash
#!/bin/bash
# vm-configs/debug-x86_64.sh

qemu-system-x86_64 \
    -m 512M \
    -nographic \
    -serial mon:stdio \
    -kernel build/kernel-x86_64.elf \
    -initrd build/initrd.img \
    -gdb tcp::1234 \
    -S \
    -d guest_errors,unimp \
    -append "debug console=ttyS0"
```

## Testing Workflows

### Automated Testing Script
```bash
#!/bin/bash
# tools/test-vm.sh

set -e

ARCH=${1:-arm64}
BUILD_DIR="build"
TIMEOUT=${2:-30}

case $ARCH in
    arm64)
        VM_SCRIPT="vm-configs/qemu-arm64.sh"
        KERNEL="$BUILD_DIR/kernel-arm64.elf"
        ;;
    x86_64)
        VM_SCRIPT="vm-configs/qemu-x86_64.sh"
        KERNEL="$BUILD_DIR/kernel-x86_64.elf"
        ;;
    *)
        echo "Usage: $0 {arm64|x86_64} [timeout_seconds]"
        exit 1
        ;;
esac

# Check if kernel exists
if [ ! -f "$KERNEL" ]; then
    echo "Error: Kernel $KERNEL not found. Build first!"
    exit 1
fi

echo "Testing MiniOS on $ARCH architecture..."

# Start VM with timeout
timeout $TIMEOUT bash $VM_SCRIPT || {
    echo "VM test completed or timed out after ${TIMEOUT}s"
}
```

### Interactive Development Session
```bash
#!/bin/bash
# tools/dev-session.sh

ARCH=${1:-arm64}

# Build the OS
make clean && make ARCH=$ARCH

# Test in VM
case $ARCH in
    arm64)
        echo "Starting ARM64 development session..."
        bash vm-configs/debug-arm64.sh
        ;;
    x86_64)  
        echo "Starting x86-64 development session..."
        bash vm-configs/debug-x86_64.sh
        ;;
esac
```

## Debugging Setup

### GDB Remote Debugging

#### Setup GDB for ARM64
```bash
# Install ARM64 GDB
brew install aarch64-elf-gdb

# Create .gdbinit for ARM64
cat > .gdbinit-arm64 << EOF
target remote localhost:1234
set architecture aarch64
symbol-file build/kernel-arm64.elf
b kernel_main
continue
EOF
```

#### Setup GDB for x86-64
```bash
# Install x86-64 GDB  
brew install x86_64-elf-gdb

# Create .gdbinit for x86-64
cat > .gdbinit-x86_64 << EOF
target remote localhost:1234
set architecture i386:x86-64
symbol-file build/kernel-x86_64.elf
b kernel_main
continue
EOF
```

#### Debugging Workflow
```bash
# Terminal 1: Start VM with debugging
bash vm-configs/debug-arm64.sh

# Terminal 2: Connect GDB
aarch64-elf-gdb -x .gdbinit-arm64
```

### Serial Console Output

#### Capture Serial Output
```bash
# Redirect QEMU serial to file
qemu-system-aarch64 \
    ... other options ... \
    -serial file:serial_output.txt

# Or use named pipe for real-time monitoring
mkfifo serial_pipe
qemu-system-aarch64 \
    ... other options ... \
    -serial pipe:serial_pipe &

# Monitor in another terminal
cat serial_pipe
```

## VM Management Scripts

### Start/Stop Scripts
```bash
#!/bin/bash
# tools/vm-manager.sh

VM_NAME="minios"
PID_FILE="/tmp/minios-vm.pid"

start_vm() {
    local arch=$1
    local config_file="vm-configs/qemu-${arch}.sh"
    
    if [ -f "$PID_FILE" ]; then
        echo "VM already running (PID: $(cat $PID_FILE))"
        return 1
    fi
    
    bash "$config_file" &
    echo $! > "$PID_FILE"
    echo "VM started with PID $!"
}

stop_vm() {
    if [ -f "$PID_FILE" ]; then
        local pid=$(cat "$PID_FILE")
        kill "$pid" 2>/dev/null
        rm "$PID_FILE"
        echo "VM stopped (PID: $pid)"
    else
        echo "No VM running"
    fi
}

case ${1:-help} in
    start)
        start_vm ${2:-arm64}
        ;;
    stop)
        stop_vm
        ;;
    restart)
        stop_vm
        sleep 2
        start_vm ${2:-arm64}
        ;;
    *)
        echo "Usage: $0 {start|stop|restart} [arch]"
        echo "Architectures: arm64, x86_64"
        ;;
esac
```

## Troubleshooting

### Common Issues

#### UTM: "Failed to start VM"
- **Check**: ARM64 vs Intel Mac compatibility
- **Solution**: Use emulation mode on Intel Macs
- **Alternative**: Use QEMU directly

#### QEMU: "qemu: could not load kernel"
- **Check**: Kernel file exists and is correct format
- **Solution**: Verify build output and file paths
- **Debug**: Use `file build/kernel-*.elf` to check format

#### No Serial Output
- **Check**: Serial console enabled in VM config
- **Check**: Kernel has early serial initialization
- **Debug**: Add debug prints in bootloader

#### VM Hangs at Boot
- **Check**: Bootloader properly loads kernel
- **Check**: Kernel entry point is correct
- **Debug**: Use GDB to step through boot process

### Performance Issues

#### Slow VM Performance
- **UTM**: Enable hardware acceleration if available
- **QEMU**: Use KVM acceleration (Linux) or HAX (macOS/Windows)
- **Both**: Reduce memory size if host is low on RAM

#### High CPU Usage
- **Check**: VM not stuck in infinite loop
- **Limit**: Use QEMU CPU limits if needed
- **Monitor**: Use `htop` to monitor host CPU usage

### Network Issues

#### VM Network Not Working
- **UTM**: Check shared network mode is enabled
- **QEMU**: Verify network configuration
- **Host**: Check firewall settings

## VM Configuration Templates

### Minimal Configuration (Fast Boot)
```json
{
  "name": "MiniOS Minimal",
  "memory": 256,
  "cores": 1,
  "display": "console-only",
  "network": "disabled"
}
```

### Development Configuration (Full Features)
```json
{
  "name": "MiniOS Development", 
  "memory": 1024,
  "cores": 2,
  "display": "graphics",
  "network": "enabled",
  "debugging": "enabled"
}
```

### Testing Configuration (Automated)
```json
{
  "name": "MiniOS Testing",
  "memory": 512, 
  "cores": 1,
  "display": "console-only",
  "network": "disabled",
  "timeout": 30
}
```

## Next Steps

After VM setup:
1. **Test VM**: Boot a simple test kernel
2. **Configure Build**: Set up build system ([BUILD.md](BUILD.md))
3. **Start Development**: Begin Phase 1 implementation
4. **Test Regularly**: Use VMs for continuous testing

For build system setup, see [BUILD.md](BUILD.md).
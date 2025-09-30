#!/usr/bin/env python3

"""
Phase 5: File System Implementation Test Suite
Tests comprehensive file system functionality including VFS, block devices, SFS, and file descriptors.
"""

import os
import subprocess
import sys
import re
import tempfile
import time
import argparse

# Add project root to path
PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, PROJECT_ROOT)

class TestResult:
    def __init__(self):
        self.passed = 0
        self.failed = 0
        self.tests = []
    
    def add_test(self, name, success, message=""):
        self.tests.append({
            'name': name,
            'success': success,
            'message': message
        })
        if success:
            self.passed += 1
        else:
            self.failed += 1
    
    def print_summary(self):
        print(f"\n=== Phase 5 Test Results ===")
        print(f"Total Tests: {self.passed + self.failed}")
        print(f"Passed: {self.passed}")
        print(f"Failed: {self.failed}")
        print(f"Success Rate: {self.passed/(self.passed + self.failed)*100:.1f}%")
        
        if self.failed > 0:
            print(f"\nFailed Tests:")
            for test in self.tests:
                if not test['success']:
                    print(f"  ❌ {test['name']}: {test['message']}")
        
        print(f"\nPassed Tests:")
        for test in self.tests:
            if test['success']:
                print(f"  ✅ {test['name']}")

def run_command(cmd, cwd=None, timeout=30):
    """Run a command and return (success, stdout, stderr)"""
    try:
        result = subprocess.run(
            cmd, shell=True, cwd=cwd, 
            capture_output=True, text=True, timeout=timeout
        )
        return result.returncode == 0, result.stdout, result.stderr
    except subprocess.TimeoutExpired:
        return False, "", "Command timed out"
    except Exception as e:
        return False, "", str(e)

def test_build_system_integration(result):
    """Test that the build system correctly includes file system components"""
    print("Testing build system integration...")
    
    # Test ARM64 build
    success, stdout, stderr = run_command("make clean && make kernel ARCH=arm64", cwd=PROJECT_ROOT, timeout=120)
    result.add_test("ARM64 kernel build with file system", success, 
                   stderr if not success else "")
    
    if success:
        # Check kernel size (should be around 149KB)
        kernel_path = os.path.join(PROJECT_ROOT, "build/arm64/kernel.elf")
        if os.path.exists(kernel_path):
            kernel_size = os.path.getsize(kernel_path)
            size_ok = 140000 <= kernel_size <= 160000  # ~149KB ± 10KB
            result.add_test("ARM64 kernel size reasonable", size_ok,
                           f"Size: {kernel_size} bytes (expected ~149KB)")
    
    # Test x86_64 build
    success, stdout, stderr = run_command("make clean && make kernel ARCH=x86_64", cwd=PROJECT_ROOT, timeout=120)
    result.add_test("x86_64 kernel build with file system", success,
                   stderr if not success else "")
    
    # Test complete image builds
    success, stdout, stderr = run_command("make clean && make ARCH=arm64", cwd=PROJECT_ROOT, timeout=120)
    result.add_test("ARM64 complete build", success, stderr if not success else "")
    
    success, stdout, stderr = run_command("make clean && make ARCH=x86_64", cwd=PROJECT_ROOT, timeout=120)
    result.add_test("x86_64 complete build", success, stderr if not success else "")

def test_header_files(result):
    """Test that header files compile independently"""
    print("Testing header files...")
    
    headers_to_test = [
        # Phase 5 File System headers
        "src/include/vfs.h",
        "src/include/sfs.h", 
        "src/include/block_device.h",
        "src/include/fd.h",
        # Core kernel headers
        "src/include/kernel.h",
        "src/include/memory.h",
        "src/include/exceptions.h",
        "src/include/boot_protocol.h",
        "src/include/kernel_loader.h",
        # Phase 4 System Service headers
        "src/include/process.h",
        "src/include/syscall.h",
        "src/include/device.h",
        "src/include/driver.h",
        "src/include/timer.h",
        "src/include/uart.h",
        "src/include/interrupt.h"
    ]
    
    for header in headers_to_test:
        header_path = os.path.join(PROJECT_ROOT, header)
        if os.path.exists(header_path):
            # Create a test C file that includes the header
            test_content = f"""
#include "kernel.h"
#include "{os.path.basename(header)}"

int main() {{ return 0; }}
"""
            with tempfile.NamedTemporaryFile(mode='w', suffix='.c', delete=False) as f:
                f.write(test_content)
                test_file = f.name
            
            try:
                # Test with ARM64 compiler
                success, stdout, stderr = run_command(
                    f"aarch64-elf-gcc -ffreestanding -I{PROJECT_ROOT}/src/include "
                    f"-I{PROJECT_ROOT}/src/arch/arm64/include -c {test_file} -o /dev/null",
                    timeout=10
                )
                result.add_test(f"ARM64 compile {os.path.basename(header)}", success, stderr)
                
                # Test with x86_64 compiler  
                success, stdout, stderr = run_command(
                    f"x86_64-elf-gcc -ffreestanding -I{PROJECT_ROOT}/src/include "
                    f"-I{PROJECT_ROOT}/src/arch/x86_64/include -c {test_file} -o /dev/null",
                    timeout=10
                )
                result.add_test(f"x86_64 compile {os.path.basename(header)}", success, stderr)
            finally:
                os.unlink(test_file)
        else:
            result.add_test(f"Header exists: {header}", False, "File not found")

def test_source_files_compilation(result):
    """Test that file system source files compile correctly"""
    print("Testing source file compilation...")
    
    fs_sources = [
        "src/fs/vfs/vfs_core.c",
        "src/fs/sfs/sfs_core.c", 
        "src/fs/block/block_device.c",
        "src/fs/block/ramdisk.c",
        "src/kernel/fd/fd_table.c"
    ]
    
    for source in fs_sources:
        source_path = os.path.join(PROJECT_ROOT, source)
        if os.path.exists(source_path):
            # Test ARM64 compilation
            success, stdout, stderr = run_command(
                f"aarch64-elf-gcc -ffreestanding -Wall -Werror -std=c11 "
                f"-I{PROJECT_ROOT}/src/include -I{PROJECT_ROOT}/src/arch/arm64/include "
                f"-c {source_path} -o /dev/null",
                timeout=15
            )
            result.add_test(f"ARM64 compile {os.path.basename(source)}", success, stderr)
            
            # Test x86_64 compilation
            success, stdout, stderr = run_command(
                f"x86_64-elf-gcc -ffreestanding -Wall -Werror -std=c11 "
                f"-I{PROJECT_ROOT}/src/include -I{PROJECT_ROOT}/src/arch/x86_64/include "
                f"-c {source_path} -o /dev/null", 
                timeout=15
            )
            result.add_test(f"x86_64 compile {os.path.basename(source)}", success, stderr)
        else:
            result.add_test(f"Source exists: {source}", False, "File not found")

def test_vfs_layer_functionality(result):
    """Test VFS layer API completeness"""
    print("Testing VFS layer functionality...")
    
    vfs_header = os.path.join(PROJECT_ROOT, "src/include/vfs.h")
    if os.path.exists(vfs_header):
        with open(vfs_header, 'r') as f:
            content = f.read()
        
        # Check for essential VFS functions
        essential_functions = [
            "vfs_init",
            "vfs_register_filesystem", 
            "vfs_mount",
            "vfs_open",
            "vfs_read",
            "vfs_write", 
            "vfs_close",
            "vfs_mkdir",
            "vfs_readdir"
        ]
        
        for func in essential_functions:
            if re.search(rf'\b{func}\s*\(', content):
                result.add_test(f"VFS function declared: {func}", True)
            else:
                result.add_test(f"VFS function declared: {func}", False, "Function not found")
        
        # Check for essential structures
        essential_structs = [
            "struct file_system_type",
            "struct file_system", 
            "struct file",
            "struct inode",
            "struct vfs_mount"
        ]
        
        for struct in essential_structs:
            if struct in content:
                result.add_test(f"VFS structure defined: {struct}", True)
            else:
                result.add_test(f"VFS structure defined: {struct}", False, "Structure not found")
    else:
        result.add_test("VFS header exists", False, "vfs.h not found")

def test_sfs_implementation(result):
    """Test Simple File System implementation"""
    print("Testing SFS implementation...")
    
    sfs_header = os.path.join(PROJECT_ROOT, "src/include/sfs.h")
    if os.path.exists(sfs_header):
        with open(sfs_header, 'r') as f:
            content = f.read()
        
        # Check for SFS functions
        sfs_functions = [
            "sfs_init",
            "sfs_mount", 
            "sfs_format",
            "sfs_alloc_inode",
            "sfs_alloc_block",
            "sfs_read_file",
            "sfs_write_file"
        ]
        
        for func in sfs_functions:
            if re.search(rf'\b{func}\s*\(', content):
                result.add_test(f"SFS function declared: {func}", True)
            else:
                result.add_test(f"SFS function declared: {func}", False, "Function not found")
                
        # Check for SFS structures
        sfs_structs = [
            "struct sfs_superblock",
            "struct sfs_inode",
            "struct sfs_dirent"
        ]
        
        for struct in sfs_structs:
            if struct in content:
                result.add_test(f"SFS structure defined: {struct}", True)
            else:
                result.add_test(f"SFS structure defined: {struct}", False, "Structure not found")
    else:
        result.add_test("SFS header exists", False, "sfs.h not found")

def test_block_device_layer(result):
    """Test block device abstraction layer"""
    print("Testing block device layer...")
    
    block_header = os.path.join(PROJECT_ROOT, "src/include/block_device.h")
    if os.path.exists(block_header):
        with open(block_header, 'r') as f:
            content = f.read()
        
        # Check for block device functions
        block_functions = [
            "block_device_init",
            "block_device_register",
            "block_device_read",
            "block_device_write",
            "ramdisk_create"
        ]
        
        for func in block_functions:
            if re.search(rf'\b{func}\s*\(', content):
                result.add_test(f"Block device function: {func}", True)
            else:
                result.add_test(f"Block device function: {func}", False, "Function not found")
                
        # Check for block device structures
        if "struct block_device" in content:
            result.add_test("Block device structure defined", True)
        else:
            result.add_test("Block device structure defined", False, "Structure not found")
    else:
        result.add_test("Block device header exists", False, "block_device.h not found")

def test_file_descriptor_management(result):
    """Test file descriptor management system"""
    print("Testing file descriptor management...")
    
    fd_header = os.path.join(PROJECT_ROOT, "src/include/fd.h")
    if os.path.exists(fd_header):
        with open(fd_header, 'r') as f:
            content = f.read()
        
        # Check for FD functions
        fd_functions = [
            "fd_init",
            "fd_table_create",
            "fd_allocate", 
            "fd_open",
            "fd_read",
            "fd_write",
            "fd_close"
        ]
        
        for func in fd_functions:
            if re.search(rf'\b{func}\s*\(', content):
                result.add_test(f"FD function declared: {func}", True)
            else:
                result.add_test(f"FD function declared: {func}", False, "Function not found")
                
        # Check for FD structures
        fd_structs = [
            "struct file_descriptor",
            "struct fd_table"
        ]
        
        for struct in fd_structs:
            if struct in content:
                result.add_test(f"FD structure defined: {struct}", True)
            else:
                result.add_test(f"FD structure defined: {struct}", False, "Structure not found")
    else:
        result.add_test("FD header exists", False, "fd.h not found")

def test_kernel_integration(result):
    """Test that kernel properly integrates file system"""
    print("Testing kernel integration...")
    
    kernel_main = os.path.join(PROJECT_ROOT, "src/kernel/main.c")
    if os.path.exists(kernel_main):
        with open(kernel_main, 'r') as f:
            content = f.read()
        
        # Check for file system initialization calls
        fs_init_calls = [
            "block_device_init",
            "vfs_init",
            "sfs_init", 
            "fd_init"
        ]
        
        for call in fs_init_calls:
            if call in content:
                result.add_test(f"Kernel calls {call}", True)
            else:
                result.add_test(f"Kernel calls {call}", False, "Call not found")
                
        # Check for file system setup
        if "ramdisk_create" in content:
            result.add_test("Kernel creates RAM disk", True)
        else:
            result.add_test("Kernel creates RAM disk", False, "RAM disk creation not found")
            
        if "vfs_mount" in content:
            result.add_test("Kernel mounts root filesystem", True)
        else:
            result.add_test("Kernel mounts root filesystem", False, "Mount call not found")
    else:
        result.add_test("Kernel main exists", False, "main.c not found")

def test_cross_platform_consistency(result):
    """Test that file system works on both architectures"""
    print("Testing cross-platform consistency...")
    
    # Ensure both builds exist by building them if needed
    arm64_kernel = os.path.join(PROJECT_ROOT, "build/arm64/kernel.elf")
    x86_64_kernel = os.path.join(PROJECT_ROOT, "build/x86_64/kernel.elf")
    
    # Build ARM64 if missing
    if not os.path.exists(arm64_kernel):
        print("Building ARM64 kernel...")
        success, stdout, stderr = run_command("make kernel ARCH=arm64", cwd=PROJECT_ROOT, timeout=120)
        if not success:
            result.add_test("ARM64 kernel build for consistency check", False, "Build failed")
            return
    
    # Build x86_64 if missing  
    if not os.path.exists(x86_64_kernel):
        print("Building x86_64 kernel...")
        success, stdout, stderr = run_command("make kernel ARCH=x86_64", cwd=PROJECT_ROOT, timeout=120)
        if not success:
            result.add_test("x86_64 kernel build for consistency check", False, "Build failed")
            return
    
    # Now check if both exist
    result.add_test("Both kernel builds exist", 
                   os.path.exists(arm64_kernel) and os.path.exists(x86_64_kernel))
    
    if os.path.exists(arm64_kernel) and os.path.exists(x86_64_kernel):
        arm64_size = os.path.getsize(arm64_kernel)
        x86_64_size = os.path.getsize(x86_64_kernel)
        
        # Sizes should be within 70% of each other (different architectures can vary more)
        ratio = max(arm64_size, x86_64_size) / min(arm64_size, x86_64_size)
        size_consistent = ratio <= 1.7  # Allow for architectural differences
        
        result.add_test("Cross-platform kernel sizes consistent", size_consistent,
                       f"ARM64: {arm64_size}, x86_64: {x86_64_size}, ratio: {ratio:.2f}")
    else:
        result.add_test("Cross-platform kernel sizes consistent", False, "Missing kernel files")
    
    # Check/build images
    arm64_image = os.path.join(PROJECT_ROOT, "build/arm64/minios.img")
    x86_64_image = os.path.join(PROJECT_ROOT, "build/x86_64/minios.iso")
    
    # Build ARM64 image if missing
    if not os.path.exists(arm64_image):
        print("Building ARM64 image...")
        success, stdout, stderr = run_command("make image ARCH=arm64", cwd=PROJECT_ROOT, timeout=60)
        
    # Build x86_64 image if missing
    if not os.path.exists(x86_64_image):
        print("Building x86_64 image...")  
        success, stdout, stderr = run_command("make image ARCH=x86_64", cwd=PROJECT_ROOT, timeout=60)
    
    result.add_test("ARM64 image created", os.path.exists(arm64_image))
    result.add_test("x86_64 ISO created", os.path.exists(x86_64_image))

def main():
    parser = argparse.ArgumentParser(description='Phase 5 File System Test Suite')
    parser.add_argument('--verbose', '-v', action='store_true', help='Verbose output')
    parser.add_argument('--skip-build', action='store_true', help='Skip build tests')
    args = parser.parse_args()
    
    print("=" * 50)
    print("Phase 5: File System Implementation Tests")
    print("=" * 50)
    
    result = TestResult()
    
    try:
        if not args.skip_build:
            test_build_system_integration(result)
        
        test_header_files(result)
        test_source_files_compilation(result) 
        test_vfs_layer_functionality(result)
        test_sfs_implementation(result)
        test_block_device_layer(result)
        test_file_descriptor_management(result)
        test_kernel_integration(result)
        test_cross_platform_consistency(result)
        
    except KeyboardInterrupt:
        print("\nTests interrupted by user")
        return 1
    except Exception as e:
        print(f"\nTest suite error: {e}")
        return 1
    
    result.print_summary()
    
    # Return non-zero exit code if any tests failed
    return 1 if result.failed > 0 else 0

if __name__ == "__main__":
    sys.exit(main())
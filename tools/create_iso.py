#!/usr/bin/env python3
"""
MiniOS ISO Creation Tool

Creates bootable ISO images for x86-64 architecture using El Torito format.
"""

import argparse
import os
import sys
import tempfile
import subprocess
import shutil

def create_iso_structure(kernel_path, bootloader_path, temp_dir):
    """Create ISO directory structure."""
    
    # Create boot directory
    boot_dir = os.path.join(temp_dir, 'boot')
    os.makedirs(boot_dir)
    
    # Copy kernel
    kernel_dest = os.path.join(boot_dir, 'kernel.elf')
    shutil.copy2(kernel_path, kernel_dest)
    
    # Copy bootloader  
    bootloader_dest = os.path.join(boot_dir, 'bootloader.bin')
    shutil.copy2(bootloader_path, bootloader_dest)
    
    # Create GRUB configuration
    grub_dir = os.path.join(boot_dir, 'grub')
    os.makedirs(grub_dir)
    
    grub_cfg = os.path.join(grub_dir, 'grub.cfg')
    with open(grub_cfg, 'w') as f:
        f.write('''set timeout=5
set default=0

menuentry "MiniOS" {
    multiboot2 /boot/kernel.elf
    boot
}

menuentry "MiniOS (Debug)" {
    multiboot2 /boot/kernel.elf debug
    boot
}
''')
    
    print(f"Created ISO structure in: {temp_dir}")
    return True

def create_iso_with_grub(temp_dir, output_path):
    """Create ISO using GRUB (if available)."""
    
    try:
        # Check if grub-mkrescue is available
        result = subprocess.run(['which', 'grub-mkrescue'], 
                              capture_output=True, text=True)
        if result.returncode != 0:
            return False
            
        # Create ISO with GRUB
        cmd = ['grub-mkrescue', '-o', output_path, temp_dir]
        print(f"Running: {' '.join(cmd)}")
        
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode == 0:
            print("ISO created with GRUB successfully")
            return True
        else:
            print(f"GRUB error: {result.stderr}")
            return False
            
    except Exception as e:
        print(f"GRUB not available: {e}")
        return False

def create_iso_with_mkisofs(temp_dir, output_path, bootloader_path):
    """Create ISO using mkisofs/genisoimage (fallback)."""
    
    try:
        # Try mkisofs first, then genisoimage
        for cmd_name in ['mkisofs', 'genisoimage']:
            result = subprocess.run(['which', cmd_name], 
                                  capture_output=True, text=True)
            if result.returncode == 0:
                # Create El Torito bootable ISO
                cmd = [
                    cmd_name,
                    '-b', 'boot/bootloader.bin',  # Boot image
                    '-no-emul-boot',              # No emulation
                    '-boot-load-size', '4',       # Load 4 sectors
                    '-boot-info-table',           # Add boot info table
                    '-o', output_path,            # Output file
                    temp_dir                      # Source directory
                ]
                
                print(f"Running: {' '.join(cmd)}")
                result = subprocess.run(cmd, capture_output=True, text=True)
                
                if result.returncode == 0:
                    print(f"ISO created with {cmd_name} successfully")
                    return True
                else:
                    print(f"{cmd_name} error: {result.stderr}")
        
        return False
        
    except Exception as e:
        print(f"mkisofs/genisoimage error: {e}")
        return False

def create_simple_iso(kernel_path, bootloader_path, output_path):
    """Create a simple concatenated image as fallback."""
    
    print("Creating simple disk image (not bootable ISO)")
    print("Install grub-mkrescue or mkisofs for proper ISO creation")
    
    try:
        with open(output_path, 'wb') as f:
            # Write bootloader
            with open(bootloader_path, 'rb') as boot_f:
                bootloader_data = boot_f.read()
                f.write(bootloader_data)
            
            # Pad to sector boundary (512 bytes)
            current_pos = f.tell()
            sector_size = 512
            padding = sector_size - (current_pos % sector_size)
            if padding != sector_size:
                f.write(b'\x00' * padding)
            
            # Write kernel
            with open(kernel_path, 'rb') as kernel_f:
                kernel_data = kernel_f.read()
                f.write(kernel_data)
            
            # Pad to minimum size (2MB)
            current_pos = f.tell()
            min_size = 2 * 1024 * 1024
            if current_pos < min_size:
                f.write(b'\x00' * (min_size - current_pos))
        
        return True
        
    except Exception as e:
        print(f"Error creating simple image: {e}")
        return False

def main():
    parser = argparse.ArgumentParser(
        description='Create MiniOS bootable ISO images for x86-64',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
  %(prog)s --kernel kernel.elf --bootloader boot.bin --output minios.iso
  
Requirements:
  - grub-mkrescue (recommended) or mkisofs/genisoimage
  - On macOS: brew install grub cdrtools
        '''
    )
    
    parser.add_argument('--kernel', required=True,
                       help='Kernel ELF file path')
    parser.add_argument('--bootloader', required=True,
                       help='Bootloader binary file path')
    parser.add_argument('--output', required=True,
                       help='Output ISO file path')
    parser.add_argument('--verbose', '-v', action='store_true',
                       help='Verbose output')
    
    args = parser.parse_args()
    
    if args.verbose:
        print("MiniOS ISO Creation Tool")
        print("=" * 40)
    
    # Validate input files
    for file_path in [args.kernel, args.bootloader]:
        if not os.path.exists(file_path):
            print(f"Error: File not found: {file_path}")
            sys.exit(1)
    
    # Create output directory if needed
    output_dir = os.path.dirname(args.output)
    if output_dir and not os.path.exists(output_dir):
        os.makedirs(output_dir)
    
    success = False
    
    # Use temporary directory for ISO structure
    with tempfile.TemporaryDirectory() as temp_dir:
        # Create ISO directory structure
        if create_iso_structure(args.kernel, args.bootloader, temp_dir):
            # Try different methods to create ISO
            if create_iso_with_grub(temp_dir, args.output):
                success = True
            elif create_iso_with_mkisofs(temp_dir, args.output, args.bootloader):
                success = True
            else:
                # Fallback to simple image
                success = create_simple_iso(args.kernel, args.bootloader, args.output)
    
    if success:
        if os.path.exists(args.output):
            size = os.path.getsize(args.output)
            print(f"ISO creation successful: {args.output}")
            print(f"Size: {size} bytes ({size // (1024*1024)} MB)")
        sys.exit(0)
    else:
        print("ISO creation failed!")
        sys.exit(1)

if __name__ == '__main__':
    main()
#!/usr/bin/env python3
"""
MiniOS Image Creation Tool

Creates bootable disk images for ARM64 and x86-64 architectures.
"""

import argparse
import struct
import os
import sys

def create_arm64_image(kernel_path, bootloader_path, output_path):
    """Create ARM64 disk image for UTM/QEMU."""
    
    print(f"Creating ARM64 image: {output_path}")
    print(f"  Kernel: {kernel_path}")
    print(f"  Bootloader: {bootloader_path}")
    
    # Read kernel and bootloader
    try:
        with open(kernel_path, 'rb') as f:
            kernel_data = f.read()
        print(f"  Kernel size: {len(kernel_data)} bytes")
    except FileNotFoundError:
        print(f"Error: Kernel file not found: {kernel_path}")
        return False
    
    try:
        with open(bootloader_path, 'rb') as f:
            bootloader_data = f.read()
        print(f"  Bootloader size: {len(bootloader_data)} bytes")
    except FileNotFoundError:
        print(f"Error: Bootloader file not found: {bootloader_path}")
        return False
    
    # Create image
    try:
        with open(output_path, 'wb') as f:
            # Write bootloader at the beginning (sector 0)
            f.write(bootloader_data)
            
            # Pad to 1MB boundary for kernel
            current_size = len(bootloader_data)
            padding_needed = (1024 * 1024) - (current_size % (1024 * 1024))
            if padding_needed < (1024 * 1024):
                f.write(b'\x00' * padding_needed)
            
            # Write kernel at 1MB offset
            kernel_offset = f.tell()
            f.write(kernel_data)
            print(f"  Kernel offset: 0x{kernel_offset:08x}")
            
            # Pad to minimum image size (16MB)
            current_size = f.tell()
            min_size = 16 * 1024 * 1024
            if current_size < min_size:
                f.write(b'\x00' * (min_size - current_size))
            
            final_size = f.tell()
            print(f"  Final image size: {final_size} bytes ({final_size // (1024*1024)} MB)")
            
    except IOError as e:
        print(f"Error creating image: {e}")
        return False
    
    return True

def create_x86_64_image(kernel_path, bootloader_path, output_path):
    """Create x86-64 disk image (raw format)."""
    
    print(f"Creating x86-64 image: {output_path}")
    print(f"  Note: Use create_iso.py for bootable ISO")
    
    # For now, create a simple raw image similar to ARM64
    return create_arm64_image(kernel_path, bootloader_path, output_path)

def main():
    parser = argparse.ArgumentParser(
        description='Create MiniOS bootable disk images',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
  %(prog)s --arch arm64 --kernel kernel.elf --bootloader boot.elf --output minios.img
  %(prog)s --arch x86_64 --kernel kernel.elf --bootloader boot.bin --output minios.img
        '''
    )
    
    parser.add_argument('--arch', required=True, 
                       choices=['arm64', 'x86_64'],
                       help='Target architecture')
    parser.add_argument('--kernel', required=True,
                       help='Kernel ELF file path')
    parser.add_argument('--bootloader', required=True,
                       help='Bootloader file path')
    parser.add_argument('--output', required=True,
                       help='Output image file path')
    parser.add_argument('--verbose', '-v', action='store_true',
                       help='Verbose output')
    
    args = parser.parse_args()
    
    if args.verbose:
        print("MiniOS Image Creation Tool")
        print(f"Architecture: {args.arch}")
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
    
    # Create image based on architecture
    success = False
    if args.arch == 'arm64':
        success = create_arm64_image(args.kernel, args.bootloader, args.output)
    elif args.arch == 'x86_64':
        success = create_x86_64_image(args.kernel, args.bootloader, args.output)
    
    if success:
        print("Image creation successful!")
        sys.exit(0)
    else:
        print("Image creation failed!")
        sys.exit(1)

if __name__ == '__main__':
    main()
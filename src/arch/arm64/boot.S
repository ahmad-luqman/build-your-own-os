/*
 * MiniOS ARM64 Bootloader
 * UEFI entry point for AArch64 architecture
 */

.section ".text.start"
.global _start

_start:
    // UEFI entry point
    // x0 = EFI_HANDLE image_handle
    // x1 = EFI_SYSTEM_TABLE *system_table
    
    // Disable interrupts
    msr daifset, #0xf
    
    // Set up initial stack (use end of first 64KB as stack)
    ldr x2, =stack_top
    mov sp, x2
    
    // Clear BSS section
    ldr x2, =__bss_start
    ldr x3, =__bss_end
    sub x3, x3, x2        // Calculate size
    mov x4, #0
clear_bss:
    cbz x3, bss_cleared
    str x4, [x2], #8
    sub x3, x3, #8
    b clear_bss

bss_cleared:
    // Call C bootloader main (preserve UEFI parameters)
    // x0 = image_handle, x1 = system_table
    bl _start_c
    
    // Should never return, but halt if it does
halt:
    wfi
    b halt

// Initial stack (4KB)
.section ".bss"
.align 16
stack_bottom:
    .skip 4096
stack_top:
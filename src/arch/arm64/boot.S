/*
 * MiniOS ARM64 Bootloader
 * Initial boot code for AArch64 architecture
 */

.section ".text.start"
.global _start

_start:
    // We assume we're loaded by UEFI or similar at EL1
    // For now, just set up a minimal stack and jump to kernel
    
    // Disable interrupts
    msr daifset, #0xf
    
    // Set up initial stack (use end of first 64KB as stack)
    ldr x0, =stack_top
    mov sp, x0
    
    // Clear BSS section
    ldr x0, =__bss_start
    ldr x1, =__bss_end
    sub x1, x1, x0        // Calculate size
    mov x2, #0
clear_bss:
    cbz x1, bss_cleared
    str x2, [x0], #8
    sub x1, x1, #8
    b clear_bss

bss_cleared:
    // Jump to kernel main
    bl kernel_main
    
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
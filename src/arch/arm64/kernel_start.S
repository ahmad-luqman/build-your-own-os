/*
 * MiniOS ARM64 Kernel Entry Point
 * Direct kernel boot entry for QEMU
 */

.section ".text.start"
.global _start

_start:
    // Disable interrupts
    msr daifset, #0xf

    // Enable FP/SIMD access at EL1
    mrs x1, cpacr_el1
    orr x1, x1, #(3 << 20)
    msr cpacr_el1, x1
    isb

    // Set up initial stack pointer at fixed location
    ldr x0, =0x40200000      // Fixed stack location (well above kernel)
    mov sp, x0

    // Set up stack canary at bottom of stack guard area
    // Canary pattern: 0xDEADC0DEDEADC0DE
    ldr x1, =0x401F0000      // Bottom of stack guard
    ldr x2, =0xDEADC0DE
    ldr x3, =0xDEADC0DE
    stp x2, x3, [x1]
    ldr x2, =0xDEADC0DE
    ldr x3, =0xDEADC0DE
    stp x2, x3, [x1, #16]
    ldr x2, =0xDEADC0DE
    ldr x3, =0xDEADC0DE
    stp x2, x3, [x1, #32]
    ldr x2, =0xDEADC0DE
    ldr x3, =0xDEADC0DE
    stp x2, x3, [x1, #48]
    
    // Clear BSS section
    ldr x0, =__bss_start
    ldr x1, =__bss_end
    sub x1, x1, x0        // Calculate BSS size
    mov x2, #0
clear_bss_loop:
    cbz x1, bss_done
    str x2, [x0], #8
    sub x1, x1, #8
    b clear_bss_loop

bss_done:
    // Simple early debug output
    mov x0, #0x09000000      // UART base address for QEMU virt
    mov x1, #0x48            // 'H' character
    str w1, [x0]             // Write to UART
    
    mov x1, #0x69            // 'i' character  
    str w1, [x0]             // Write to UART
    
    mov x1, #0x0A            // '\n' character
    str w1, [x0]             // Write to UART
    
    // Set up boot_info structure
    ldr x0, =static_boot_info
    
    // Initialize boot_info structure with correct offsets
    // magic = BOOT_PROTOCOL_MAGIC (0x4d696e694f53ULL) 
    movz x1, #0x4f53         // Lower 16 bits (0x4f53)
    movk x1, #0x6e69, lsl #16   // Next 16 bits (0x6e69)
    movk x1, #0x4d69, lsl #32   // Next 16 bits (0x4d69)
    movk x1, #0x0000, lsl #48   // Upper 16 bits (0x0000)
    str x1, [x0, #0]         // magic at offset 0
    
    // arch = BOOT_ARCH_ARM64 (0x1)
    mov w1, #1
    str w1, [x0, #8]         // arch at offset 8 (32-bit)
    
    // version = BOOT_PROTOCOL_VERSION (1)  
    mov w1, #1
    str w1, [x0, #12]        // version at offset 12 (32-bit)
    
    // Set up memory map
    ldr x2, =static_boot_info
    add x2, x2, #384         // Place memory map after boot_info
    str x2, [x0, #16]        // memory_map pointer at offset 16
    
    mov w1, #1
    str w1, [x0, #24]        // memory_map_entries = 1
    
    mov w1, #24
    str w1, [x0, #28]        // memory_map_size = 24
    
    // Fill in the memory map entry 
    mov x1, #0x40000000      // base address
    str x1, [x2, #0]         // base
    mov x1, #0x8000000       // 128MB length  
    str x1, [x2, #8]         // length
    mov w1, #1               // MEMORY_TYPE_AVAILABLE
    str w1, [x2, #16]        // type
    str wzr, [x2, #20]       // attributes = 0
    
    // Call kernel_main with boot_info
    bl kernel_main
    
    // Should never return, but halt if it does
halt:
    wfi
    b halt

// Stack is now at fixed location (0x40140000) - no need to allocate in BSS

// Static boot_info structure
.align 16
static_boot_info:
    .skip 512           // Allocate 512 bytes for boot_info + memory map

// BSS symbols (will be defined by linker)
.global __bss_start
.global __bss_end

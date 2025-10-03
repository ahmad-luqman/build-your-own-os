/*
 * ARM64 Exception Vector Table
 * Phase 3: Memory Management & Kernel Loading
 */

.section ".text.vectors", "ax"
.align 11
.global exception_vectors

/*
 * Exception vector table
 * Each entry is 128 bytes (0x80)
 * Total table size is 2KB (0x800)
 */
exception_vectors:
    /* Current EL with SP0 */
    .align 7
    b sync_exception_sp0_handler
    .align 7  
    b irq_exception_sp0_handler
    .align 7
    b fiq_exception_sp0_handler
    .align 7
    b serror_exception_sp0_handler

    /* Current EL with SPx */
    .align 7
    b sync_exception_spx_handler
    .align 7
    b irq_exception_spx_handler
    .align 7
    b fiq_exception_spx_handler
    .align 7
    b serror_exception_spx_handler

    /* Lower EL using AArch64 */
    .align 7
    b sync_exception_aarch64_handler
    .align 7
    b irq_exception_aarch64_handler
    .align 7
    b fiq_exception_aarch64_handler
    .align 7
    b serror_exception_aarch64_handler

    /* Lower EL using AArch32 */
    .align 7
    b sync_exception_aarch32_handler
    .align 7
    b irq_exception_aarch32_handler
    .align 7
    b fiq_exception_aarch32_handler
    .align 7
    b serror_exception_aarch32_handler

/*
 * Exception handler stubs - save context and call C handlers
 */

// Exception stack base address (dedicated stack for exceptions)
.set EXCEPTION_STACK_BASE, 0x40300000

.macro save_context
    // Switch to exception stack FIRST
    mov x5, sp                // Save current SP
    ldr x6, =EXCEPTION_STACK_BASE
    mov sp, x6                // Use dedicated exception stack

    // Make room for context FIRST
    sub sp, sp, #(35 * 8)     // Make room for context + original SP

    // Save original SP at the end
    str x5, [sp, #(34 * 8)]   // Save original SP

    // Save general purpose registers on exception stack
    stp x0, x1, [sp, #(0 * 8)]
    stp x2, x3, [sp, #(2 * 8)]
    stp x4, x5, [sp, #(4 * 8)]  // x5 contains original SP (already saved above)
    stp x6, x7, [sp, #(6 * 8)]
    stp x8, x9, [sp, #(8 * 8)]
    stp x10, x11, [sp, #(10 * 8)]
    stp x12, x13, [sp, #(12 * 8)]
    stp x14, x15, [sp, #(14 * 8)]
    stp x16, x17, [sp, #(16 * 8)]
    stp x18, x19, [sp, #(18 * 8)]
    stp x20, x21, [sp, #(20 * 8)]
    stp x22, x23, [sp, #(22 * 8)]
    stp x24, x25, [sp, #(24 * 8)]
    stp x26, x27, [sp, #(26 * 8)]
    stp x28, x29, [sp, #(28 * 8)]
    str x30, [sp, #(30 * 8)]

    // Save system registers
    mrs x0, elr_el1
    mrs x1, spsr_el1
    mrs x2, esr_el1
    mrs x3, far_el1
    stp x0, x1, [sp, #(31 * 8)]
    stp x2, x3, [sp, #(33 * 8)]    // esr_el1, far_el1
.endm

.macro restore_context
    // Save original SP before restoring other registers
    ldr x5, [sp, #(34 * 8)]   // Load original SP

    // Restore system registers
    ldp x0, x1, [sp, #(31 * 8)]
    msr elr_el1, x0
    msr spsr_el1, x1

    // Restore general purpose registers
    ldp x0, x1, [sp, #(0 * 8)]
    ldp x2, x3, [sp, #(2 * 8)]
    ldp x4, x6, [sp, #(4 * 8)]  // Use x6, x5 holds original SP
    ldp x6, x7, [sp, #(6 * 8)]
    ldp x8, x9, [sp, #(8 * 8)]
    ldp x10, x11, [sp, #(10 * 8)]
    ldp x12, x13, [sp, #(12 * 8)]
    ldp x14, x15, [sp, #(14 * 8)]
    ldp x16, x17, [sp, #(16 * 8)]
    ldp x18, x19, [sp, #(18 * 8)]
    ldp x20, x21, [sp, #(20 * 8)]
    ldp x22, x23, [sp, #(22 * 8)]
    ldp x24, x25, [sp, #(24 * 8)]
    ldp x26, x27, [sp, #(26 * 8)]
    ldp x28, x29, [sp, #(28 * 8)]
    ldr x30, [sp, #(30 * 8)]

    // Restore original SP and clean up
    mov sp, x5                // Restore original SP
.endm

// Current EL with SP0 handlers
sync_exception_sp0_handler:
    save_context
    mov x0, #0x00           // Exception type: sync_sp0
    mov x1, sp              // Context pointer
    bl arm64_exception_handler
    restore_context
    eret

irq_exception_sp0_handler:
    save_context
    mov x0, #0x01           // Exception type: irq_sp0
    mov x1, sp              // Context pointer
    bl arm64_exception_handler
    restore_context
    eret

fiq_exception_sp0_handler:
    save_context
    mov x0, #0x02           // Exception type: fiq_sp0
    mov x1, sp              // Context pointer
    bl arm64_exception_handler
    restore_context
    eret

serror_exception_sp0_handler:
    save_context
    mov x0, #0x03           // Exception type: serror_sp0
    mov x1, sp              // Context pointer
    bl arm64_exception_handler
    restore_context
    eret

// Current EL with SPx handlers
sync_exception_spx_handler:
    save_context
    mov x0, #0x04           // Exception type: sync_spx
    mov x1, sp              // Context pointer
    bl arm64_exception_handler
    restore_context
    eret

irq_exception_spx_handler:
    save_context
    mov x0, #0x05           // Exception type: irq_spx
    mov x1, sp              // Context pointer
    bl arm64_exception_handler
    restore_context
    eret

fiq_exception_spx_handler:
    save_context
    mov x0, #0x06           // Exception type: fiq_spx
    mov x1, sp              // Context pointer
    bl arm64_exception_handler
    restore_context
    eret

serror_exception_spx_handler:
    save_context
    mov x0, #0x07           // Exception type: serror_spx
    mov x1, sp              // Context pointer
    bl arm64_exception_handler
    restore_context
    eret

// Lower EL using AArch64 handlers
sync_exception_aarch64_handler:
    save_context
    mov x0, #0x08           // Exception type: sync_aarch64
    mov x1, sp              // Context pointer
    bl arm64_exception_handler
    restore_context
    eret

irq_exception_aarch64_handler:
    save_context
    mov x0, #0x09           // Exception type: irq_aarch64
    mov x1, sp              // Context pointer
    bl arm64_exception_handler
    restore_context
    eret

fiq_exception_aarch64_handler:
    save_context
    mov x0, #0x0A           // Exception type: fiq_aarch64
    mov x1, sp              // Context pointer
    bl arm64_exception_handler
    restore_context
    eret

serror_exception_aarch64_handler:
    save_context
    mov x0, #0x0B           // Exception type: serror_aarch64
    mov x1, sp              // Context pointer
    bl arm64_exception_handler
    restore_context
    eret

// Lower EL using AArch32 handlers
sync_exception_aarch32_handler:
    save_context
    mov x0, #0x0C           // Exception type: sync_aarch32
    mov x1, sp              // Context pointer
    bl arm64_exception_handler
    restore_context
    eret

irq_exception_aarch32_handler:
    save_context
    mov x0, #0x0D           // Exception type: irq_aarch32
    mov x1, sp              // Context pointer
    bl arm64_exception_handler
    restore_context
    eret

fiq_exception_aarch32_handler:
    save_context
    mov x0, #0x0E           // Exception type: fiq_aarch32
    mov x1, sp              // Context pointer
    bl arm64_exception_handler
    restore_context
    eret

serror_exception_aarch32_handler:
    save_context
    mov x0, #0x0F           // Exception type: serror_aarch32
    mov x1, sp              // Context pointer
    bl arm64_exception_handler
    restore_context
    eret

/*
 * Setup exception vector table
 */
.global setup_exception_vectors
setup_exception_vectors:
    adrp x0, exception_vectors
    add x0, x0, :lo12:exception_vectors
    msr vbar_el1, x0
    isb
    ret

/*
 * Enable/disable interrupts
 */
.global arch_interrupts_enable_asm
arch_interrupts_enable_asm:
    cbnz x0, 1f
    msr daifset, #2         // Disable IRQ
    ret
1:  msr daifclr, #2         // Enable IRQ
    ret

.global arch_interrupts_enabled_asm
arch_interrupts_enabled_asm:
    mrs x0, daif
    and x0, x0, #2          // Check IRQ mask bit
    eor x0, x0, #2          // Invert (0 = enabled, 2 = disabled)
    lsr x0, x0, #1          // Return 1 if enabled, 0 if disabled
    ret
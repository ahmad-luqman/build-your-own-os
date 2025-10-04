/*
 * ARM64 Exception Vector Table
 * Phase 3: Memory Management & Kernel Loading
 */

// Exception stack definitions
.set EXCEPTION_STACK_START, 0x40300000
.set EXCEPTION_STACK_END, EXCEPTION_STACK_START + 0x10000
.set EXCEPTION_CONTEXT_SIZE, (36 * 8)

/*
 * Exception handler stubs - save context and call C handlers
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
// Using EXCEPTION_STACK_START from #define above

.macro save_context
    // Reserve space on the current stack for the exception context structure
    sub sp, sp, #EXCEPTION_CONTEXT_SIZE

    // Save general-purpose registers (x0-x30)
    str x0,  [sp, #(0  * 8)]
    str x1,  [sp, #(1  * 8)]
    str x2,  [sp, #(2  * 8)]
    str x3,  [sp, #(3  * 8)]
    str x4,  [sp, #(4  * 8)]
    str x5,  [sp, #(5  * 8)]
    str x6,  [sp, #(6  * 8)]
    str x7,  [sp, #(7  * 8)]
    str x8,  [sp, #(8  * 8)]
    str x9,  [sp, #(9  * 8)]
    str x10, [sp, #(10 * 8)]
    str x11, [sp, #(11 * 8)]
    str x12, [sp, #(12 * 8)]
    str x13, [sp, #(13 * 8)]
    str x14, [sp, #(14 * 8)]
    str x15, [sp, #(15 * 8)]
    str x16, [sp, #(16 * 8)]
    str x17, [sp, #(17 * 8)]
    str x18, [sp, #(18 * 8)]
    str x19, [sp, #(19 * 8)]
    str x20, [sp, #(20 * 8)]
    str x21, [sp, #(21 * 8)]
    str x22, [sp, #(22 * 8)]
    str x23, [sp, #(23 * 8)]
    str x24, [sp, #(24 * 8)]
    str x25, [sp, #(25 * 8)]
    str x26, [sp, #(26 * 8)]
    str x27, [sp, #(27 * 8)]
    str x28, [sp, #(28 * 8)]
    str x29, [sp, #(29 * 8)]
    str x30, [sp, #(30 * 8)]

    // Save original stack pointer into the context (sp field)
    add x0, sp, #EXCEPTION_CONTEXT_SIZE
    str x0, [sp, #(31 * 8)]

    // Save system registers (for diagnostics)
    mrs x1, elr_el1
    str x1, [sp, #(32 * 8)]
    mrs x1, spsr_el1
    str x1, [sp, #(33 * 8)]
    mrs x1, esr_el1
    str x1, [sp, #(34 * 8)]
    mrs x1, far_el1
    str x1, [sp, #(35 * 8)]

    // Move to the dedicated exception stack and remember context + original SP
    mov x1, sp                           // context pointer
    add x2, sp, #EXCEPTION_CONTEXT_SIZE  // original SP for restore
    ldr x3, =EXCEPTION_STACK_START
    mov sp, x3
    sub sp, sp, #32
    str x1, [sp]
    str x2, [sp, #16]
.endm

.macro restore_context
    // Retrieve context pointer and original SP from exception stack
    ldr x9, [sp]
    ldr x10, [sp, #16]
    add sp, sp, #32

    // Restore ELR/SPSR
    ldr x11, [x9, #(32 * 8)]
    msr elr_el1, x11
    ldr x11, [x9, #(33 * 8)]
    msr spsr_el1, x11

    // Restore general-purpose registers (except x9 for now)
    ldr x0,  [x9, #(0  * 8)]
    ldr x1,  [x9, #(1  * 8)]
    ldr x2,  [x9, #(2  * 8)]
    ldr x3,  [x9, #(3  * 8)]
    ldr x4,  [x9, #(4  * 8)]
    ldr x5,  [x9, #(5  * 8)]
    ldr x6,  [x9, #(6  * 8)]
    ldr x7,  [x9, #(7  * 8)]
    ldr x8,  [x9, #(8  * 8)]

    mov sp, x10                        // restore original SP before reloading x10

    ldr x10, [x9, #(10 * 8)]
    ldr x11, [x9, #(11 * 8)]
    ldr x12, [x9, #(12 * 8)]
    ldr x13, [x9, #(13 * 8)]
    ldr x14, [x9, #(14 * 8)]
    ldr x15, [x9, #(15 * 8)]
    ldr x16, [x9, #(16 * 8)]
    ldr x17, [x9, #(17 * 8)]
    ldr x18, [x9, #(18 * 8)]
    ldr x19, [x9, #(19 * 8)]
    ldr x20, [x9, #(20 * 8)]
    ldr x21, [x9, #(21 * 8)]
    ldr x22, [x9, #(22 * 8)]
    ldr x23, [x9, #(23 * 8)]
    ldr x24, [x9, #(24 * 8)]
    ldr x25, [x9, #(25 * 8)]
    ldr x26, [x9, #(26 * 8)]
    ldr x27, [x9, #(27 * 8)]
    ldr x28, [x9, #(28 * 8)]
    ldr x29, [x9, #(29 * 8)]
    ldr x30, [x9, #(30 * 8)]

    // Finally restore x9 (after the pointer is no longer required)
    ldr x9, [x9, #(9 * 8)]
.endm

// Current EL with SP0 handlers
sync_exception_sp0_handler:
    save_context
    mov x0, #0x00           // Exception type: sync_sp0
    ldr x1, [sp]            // Context pointer
    bl arm64_exception_handler
    restore_context
    eret

irq_exception_sp0_handler:
    save_context
    mov x0, #0x01           // Exception type: irq_sp0
    ldr x1, [sp]            // Context pointer
    bl arm64_exception_handler
    restore_context
    eret

fiq_exception_sp0_handler:
    save_context
    mov x0, #0x02           // Exception type: fiq_sp0
    ldr x1, [sp]            // Context pointer
    bl arm64_exception_handler
    restore_context
    eret

serror_exception_sp0_handler:
    save_context
    mov x0, #0x03           // Exception type: serror_sp0
    ldr x1, [sp]            // Context pointer
    bl arm64_exception_handler
    restore_context
    eret

// Current EL with SPx handlers
sync_exception_spx_handler:
    save_context
    mov x0, #0x04           // Exception type: sync_spx
    ldr x1, [sp]            // Context pointer
    bl arm64_exception_handler
    restore_context
    eret

irq_exception_spx_handler:
    save_context
    mov x0, #0x05           // Exception type: irq_spx
    ldr x1, [sp]            // Context pointer
    bl arm64_exception_handler
    restore_context
    eret

fiq_exception_spx_handler:
    save_context
    mov x0, #0x06           // Exception type: fiq_spx
    ldr x1, [sp]            // Context pointer
    bl arm64_exception_handler
    restore_context
    eret

serror_exception_spx_handler:
    save_context
    mov x0, #0x07           // Exception type: serror_spx
    ldr x1, [sp]            // Context pointer
    bl arm64_exception_handler
    restore_context
    eret

// Lower EL using AArch64 handlers
sync_exception_aarch64_handler:
    save_context
    mov x0, #0x08           // Exception type: sync_aarch64
    ldr x1, [sp]            // Context pointer
    bl arm64_exception_handler
    restore_context
    eret

irq_exception_aarch64_handler:
    save_context
    mov x0, #0x09           // Exception type: irq_aarch64
    ldr x1, [sp]            // Context pointer
    bl arm64_exception_handler
    restore_context
    eret

fiq_exception_aarch64_handler:
    save_context
    mov x0, #0x0A           // Exception type: fiq_aarch64
    ldr x1, [sp]            // Context pointer
    bl arm64_exception_handler
    restore_context
    eret

serror_exception_aarch64_handler:
    save_context
    mov x0, #0x0B           // Exception type: serror_aarch64
    ldr x1, [sp]            // Context pointer
    bl arm64_exception_handler
    restore_context
    eret

// Lower EL using AArch32 handlers
sync_exception_aarch32_handler:
    save_context
    mov x0, #0x0C           // Exception type: sync_aarch32
    ldr x1, [sp]            // Context pointer
    bl arm64_exception_handler
    restore_context
    eret

irq_exception_aarch32_handler:
    save_context
    mov x0, #0x0D           // Exception type: irq_aarch32
    ldr x1, [sp]            // Context pointer
    bl arm64_exception_handler
    restore_context
    eret

fiq_exception_aarch32_handler:
    save_context
    mov x0, #0x0E           // Exception type: fiq_aarch32
    ldr x1, [sp]            // Context pointer
    bl arm64_exception_handler
    restore_context
    eret

serror_exception_aarch32_handler:
    save_context
    mov x0, #0x0F           // Exception type: serror_aarch32
    ldr x1, [sp]            // Context pointer
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

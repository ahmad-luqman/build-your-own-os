/**
 * ARM64 Context Switching Implementation
 * 
 * Low-level context switching for ARM64 architecture
 */

.section .text

// Context switch between two tasks
// void context_switch(struct cpu_context *old_ctx, struct cpu_context *new_ctx)
.global context_switch
.type context_switch, @function
context_switch:
    // x0 = old context pointer
    // x1 = new context pointer
    
    // If old_ctx is NULL, just restore new context
    cbz x0, restore_context
    
    // Save current context to old_ctx
    stp x0, x1, [x0, #0]      // x0, x1
    stp x2, x3, [x0, #16]     // x2, x3
    stp x4, x5, [x0, #32]     // x4, x5
    stp x6, x7, [x0, #48]     // x6, x7
    stp x8, x9, [x0, #64]     // x8, x9
    stp x10, x11, [x0, #80]   // x10, x11
    stp x12, x13, [x0, #96]   // x12, x13
    stp x14, x15, [x0, #112]  // x14, x15
    stp x16, x17, [x0, #128]  // x16, x17
    stp x18, x19, [x0, #144]  // x18, x19
    stp x20, x21, [x0, #160]  // x20, x21
    stp x22, x23, [x0, #176]  // x22, x23
    stp x24, x25, [x0, #192]  // x24, x25
    stp x26, x27, [x0, #208]  // x26, x27
    stp x28, x29, [x0, #224]  // x28, x29
    str x30, [x0, #240]       // x30 (LR)
    
    // Save stack pointers
    mov x2, sp
    str x2, [x0, #248]        // sp_el1 (current stack)
    mrs x2, sp_el0
    str x2, [x0, #256]        // sp_el0 (user stack)
    
    // Save exception return state
    mrs x2, elr_el1
    str x2, [x0, #264]        // elr_el1
    mrs x2, spsr_el1
    str x2, [x0, #272]        // spsr_el1
    
    // Save MMU state
    mrs x2, ttbr0_el1
    str x2, [x0, #280]        // ttbr0_el1
    mrs x2, ttbr1_el1
    str x2, [x0, #288]        // ttbr1_el1

restore_context:
    // Restore new context from new_ctx (x1)
    // Restore MMU state first
    ldr x2, [x1, #280]        // ttbr0_el1
    msr ttbr0_el1, x2
    ldr x2, [x1, #288]        // ttbr1_el1
    msr ttbr1_el1, x2
    
    // Invalidate TLB
    tlbi vmalle1is
    dsb sy
    isb
    
    // Restore exception return state
    ldr x2, [x1, #264]        // elr_el1
    msr elr_el1, x2
    ldr x2, [x1, #272]        // spsr_el1
    msr spsr_el1, x2
    
    // Restore stack pointers
    ldr x2, [x1, #256]        // sp_el0
    msr sp_el0, x2
    ldr x2, [x1, #248]        // sp_el1
    mov sp, x2
    
    // Restore general purpose registers
    ldp x0, x2, [x1, #0]      // x0, x1 (x1 will be restored later)
    ldp x3, x4, [x1, #24]     // x2, x3 (x2 already in x3, will fix)
    ldp x4, x5, [x1, #32]     // x4, x5
    ldp x6, x7, [x1, #48]     // x6, x7
    ldp x8, x9, [x1, #64]     // x8, x9
    ldp x10, x11, [x1, #80]   // x10, x11
    ldp x12, x13, [x1, #96]   // x12, x13
    ldp x14, x15, [x1, #112]  // x14, x15
    ldp x16, x17, [x1, #128]  // x16, x17
    ldp x18, x19, [x1, #144]  // x18, x19
    ldp x20, x21, [x1, #160]  // x20, x21
    ldp x22, x23, [x1, #176]  // x22, x23
    ldp x24, x25, [x1, #192]  // x24, x25
    ldp x26, x27, [x1, #208]  // x26, x27
    ldp x28, x29, [x1, #224]  // x28, x29
    ldr x30, [x1, #240]       // x30 (LR)
    
    // Fix up x1 and x2
    mov x3, x2                // Save x0 value
    ldr x2, [x1, #16]         // Load correct x2 value
    ldr x1, [x1, #8]          // Load correct x1 value
    mov x0, x3                // Restore correct x0 value
    
    ret

// Setup initial context for a new task
// void arch_setup_task_context(struct cpu_context *ctx, task_entry_t entry, void *arg, void *stack_top)
.global arch_setup_task_context
.type arch_setup_task_context, @function
arch_setup_task_context:
    // x0 = context pointer
    // x1 = task entry point
    // x2 = task argument
    // x3 = stack top
    
    // Clear the context
    mov x4, #0
    mov x5, #296              // Size of cpu_context struct
clear_loop:
    str x4, [x0], #8
    subs x5, x5, #8
    b.gt clear_loop
    
    // Reset context pointer
    sub x0, x0, #296
    
    // Set up initial register state
    str x2, [x0, #0]          // x0 = task argument
    str x1, [x0, #240]        // x30 (LR) = task entry point
    
    // Set up stack pointer
    str x3, [x0, #248]        // sp_el1 = stack_top
    
    // Set up exception return state for EL1
    mov x4, #0x00000005       // EL1h with interrupts enabled
    str x4, [x0, #272]        // spsr_el1
    str x1, [x0, #264]        // elr_el1 = task entry point
    
    // Set up MMU state (inherit current state)
    mrs x4, ttbr0_el1
    str x4, [x0, #280]        // ttbr0_el1
    mrs x4, ttbr1_el1
    str x4, [x0, #288]        // ttbr1_el1
    
    ret

// System call entry point (SVC handler)
.global syscall_entry_svc
.type syscall_entry_svc, @function
syscall_entry_svc:
    // Save caller's context
    sub sp, sp, #256          // Space for saved registers
    stp x0, x1, [sp, #0]
    stp x2, x3, [sp, #16]
    stp x4, x5, [sp, #32]
    stp x6, x7, [sp, #48]
    stp x8, x9, [sp, #64]
    stp x10, x11, [sp, #80]
    stp x12, x13, [sp, #96]
    stp x14, x15, [sp, #112]
    stp x16, x17, [sp, #128]
    stp x18, x19, [sp, #144]
    stp x20, x21, [sp, #160]
    stp x22, x23, [sp, #176]
    stp x24, x25, [sp, #192]
    stp x26, x27, [sp, #208]
    stp x28, x29, [sp, #224]
    str x30, [sp, #240]
    
    // System call number in x8, arguments in x0-x5
    mov x6, x5                // Move x5 to x6 for 6th argument
    mov x5, x4                // Move x4 to x5 for 5th argument
    mov x4, x3                // Move x3 to x4 for 4th argument
    mov x3, x2                // Move x2 to x3 for 3rd argument
    mov x2, x1                // Move x1 to x2 for 2nd argument
    mov x1, x0                // Move x0 to x1 for 1st argument
    mov x0, x8                // System call number in x0
    
    // Call system call dispatcher
    bl syscall_dispatch
    
    // Result is in x0, store it for return
    mov x8, x0
    
    // Restore caller's context
    ldp x0, x1, [sp, #0]
    ldp x2, x3, [sp, #16]
    ldp x4, x5, [sp, #32]
    ldp x6, x7, [sp, #48]
    // Skip x8, x9 - x8 contains result
    ldp x10, x11, [sp, #80]
    ldp x12, x13, [sp, #96]
    ldp x14, x15, [sp, #112]
    ldp x16, x17, [sp, #128]
    ldp x18, x19, [sp, #144]
    ldp x20, x21, [sp, #160]
    ldp x22, x23, [sp, #176]
    ldp x24, x25, [sp, #192]
    ldp x26, x27, [sp, #208]
    ldp x28, x29, [sp, #224]
    ldr x30, [sp, #240]
    add sp, sp, #256
    
    // Move result to x0
    mov x0, x8
    
    // Return to caller
    ret

.size context_switch, . - context_switch
.size arch_setup_task_context, . - arch_setup_task_context  
.size syscall_entry_svc, . - syscall_entry_svc
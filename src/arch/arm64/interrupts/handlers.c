/*
 * ARM64 Exception Handlers (Architecture-specific functions only)
 * Phase 3: Memory Management & Kernel Loading
 */

#include <stdint.h>
#include "exceptions.h"
#include "kernel.h"

// ARM64 Exception Syndrome Register (ESR_EL1) exception classes
#define ESR_EC_UNKNOWN          0x00
#define ESR_EC_WFI_WFE          0x01
#define ESR_EC_CP15_32          0x03
#define ESR_EC_CP15_64          0x04
#define ESR_EC_CP14_MR          0x05
#define ESR_EC_CP14_LS          0x06
#define ESR_EC_FP_ASIMD         0x07
#define ESR_EC_CP10_ID          0x08
#define ESR_EC_PAC              0x09
#define ESR_EC_CP14_64          0x0C
#define ESR_EC_BTI              0x0D
#define ESR_EC_ILL_STATE        0x0E
#define ESR_EC_SVC32            0x11
#define ESR_EC_HVC32            0x12
#define ESR_EC_SMC32            0x13
#define ESR_EC_SVC64            0x15
#define ESR_EC_HVC64            0x16
#define ESR_EC_SMC64            0x17
#define ESR_EC_SYS64            0x18
#define ESR_EC_SVE              0x19
#define ESR_EC_ERET             0x1A
#define ESR_EC_IMP_DEF          0x1F
#define ESR_EC_IABT_LOW         0x20
#define ESR_EC_IABT_CUR         0x21
#define ESR_EC_PC_ALIGN         0x22
#define ESR_EC_DABT_LOW         0x24
#define ESR_EC_DABT_CUR         0x25
#define ESR_EC_SP_ALIGN         0x26
#define ESR_EC_FP_EXC32         0x28
#define ESR_EC_FP_EXC64         0x2C
#define ESR_EC_SERROR           0x2F
#define ESR_EC_BREAKPT_LOW      0x30
#define ESR_EC_BREAKPT_CUR      0x31
#define ESR_EC_SOFTSTP_LOW      0x32
#define ESR_EC_SOFTSTP_CUR      0x33
#define ESR_EC_WATCHPT_LOW      0x34
#define ESR_EC_WATCHPT_CUR      0x35
#define ESR_EC_BKPT32           0x38
#define ESR_EC_VECTOR32         0x3A
#define ESR_EC_BRK64            0x3C

// Exception handler table
static exception_handler_t exception_handlers[16] = {0};

// Forward declarations

// External assembly functions
extern void setup_exception_vectors(void);
extern void arch_interrupts_enable_asm(int enable);
extern int arch_interrupts_enabled_asm(void);

/**
 * Initialize ARM64 exception handling
 */
int arch_exception_init(void)
{
    // Install exception vector table
    setup_exception_vectors();
    
    // Initialize all handlers to default
    for (int i = 0; i < 16; i++) {
        exception_handlers[i] = exception_default_handler;
    }
    
    return 0;
}

/**
 * ARM64 exception handler (called from assembly)
 */
void arm64_exception_handler(uint32_t exc_type, struct exception_context *ctx)
{
    // Extract ESR information for sync exceptions
    if (exc_type == 0x00 || exc_type == 0x04 || exc_type == 0x08 || exc_type == 0x0C) {
        // Synchronous exceptions - check ESR
        uint32_t ec = (ctx->esr >> 26) & 0x3F;  // Exception class
        
        // Convert to cross-platform exception number
        uint32_t exception_num = EXCEPTION_SYNC;
        
        switch (ec) {
            case ESR_EC_IABT_LOW:
            case ESR_EC_IABT_CUR:
            case ESR_EC_DABT_LOW: 
            case ESR_EC_DABT_CUR:
                exception_num = EXCEPTION_PAGE_FAULT;
                break;
                
            case ESR_EC_PC_ALIGN:
            case ESR_EC_SP_ALIGN:
                exception_num = EXCEPTION_ALIGNMENT_CHECK;
                break;
                
            case ESR_EC_ILL_STATE:
                exception_num = EXCEPTION_INVALID_OPCODE;
                break;
                
            case ESR_EC_BRK64:
                exception_num = EXCEPTION_BREAKPOINT;
                break;
                
            default:
                exception_num = EXCEPTION_SYNC;
                break;
        }
        
        // Call registered handler
        if (exception_handlers[exception_num & 0xF]) {
            exception_handlers[exception_num & 0xF](exception_num, ctx);
        }
    } else {
        // IRQ, FIQ, or SError
        uint32_t exception_num;
        
        switch (exc_type & 0x3) {
            case 0x1: exception_num = EXCEPTION_IRQ; break;
            case 0x2: exception_num = EXCEPTION_FIQ; break;
            case 0x3: exception_num = EXCEPTION_SERROR; break;
            default: exception_num = EXCEPTION_SYNC; break;
        }
        
        if (exception_handlers[exception_num & 0xF]) {
            exception_handlers[exception_num & 0xF](exception_num, ctx);
        }
    }
}

/**
 * Register exception handler
 */
int exception_register_handler(uint32_t exception_num, exception_handler_t handler)
{
    if ((exception_num & 0xF) >= 16) {
        return -1;
    }
    
    exception_handlers[exception_num & 0xF] = handler ? handler : exception_default_handler;
    return 0;
}

/**
 * Enable/disable interrupts
 */
void arch_interrupts_enable(int enable)
{
    arch_interrupts_enable_asm(enable);
}

/**
 * Check if interrupts are enabled
 */
int arch_interrupts_enabled(void)
{
    return arch_interrupts_enabled_asm();
}
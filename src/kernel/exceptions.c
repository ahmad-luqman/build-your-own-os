/*
 * MiniOS Cross-Platform Exception Handling Implementation
 * Phase 3: Memory Management & Kernel Loading
 */

#include "exceptions.h"
#include "kernel.h"

// Exception subsystem initialization state
static int exception_initialized = 0;

// Helper function to print single hex digit
static void print_hex_digit(int digit)
{
    if (digit < 10) {
        char ch[2] = {'0' + digit, 0};
        early_print(ch);
    } else {
        char ch[2] = {'A' + digit - 10, 0};
        early_print(ch);
    }
}

/**
 * Initialize exception handling system
 */
int exception_init(void)
{
    if (exception_initialized) {
        return 0;  // Already initialized
    }
    
    early_print("Initializing exception handling...\n");
    
    // Initialize architecture-specific exception handling
    if (arch_exception_init() < 0) {
        early_print("Exception init: Architecture init failed\n");
        return -1;
    }
    
    // Register default page fault handler
    if (exception_register_handler(EXCEPTION_PAGE_FAULT, 
                                   exception_page_fault_handler) < 0) {
        early_print("Exception init: Failed to register page fault handler\n");
        return -1;
    }
    
    exception_initialized = 1;
    early_print("Exception handling initialized\n");
    
    return 0;
}

/**
 * Default exception handler
 */
void exception_default_handler(uint32_t exception_num, 
                               struct exception_context *ctx)
{
    early_print("\n*** UNHANDLED EXCEPTION ***\n");
    exception_print_info(exception_num, ctx);
    early_print("\nSystem will halt.\n");
    
    // Disable interrupts and halt system
    arch_interrupts_enable(0);
    arch_halt();
}

/**
 * Page fault handler
 */
void exception_page_fault_handler(uint32_t exception_num,
                                  struct exception_context *ctx)
{
    early_print("\n*** PAGE FAULT ***\n");
    
    // Print fault information
#ifdef ARCH_ARM64
    early_print("Fault address: 0x");
    uint64_t fault_addr = ctx->far;
    for (int i = 15; i >= 0; i--) {
        int digit = (fault_addr >> (i * 4)) & 0xF;
        print_hex_digit(digit);
    }
    early_print("\n");
    
    early_print("Exception syndrome: 0x");
    uint32_t esr = ctx->esr;
    for (int i = 7; i >= 0; i--) {
        int digit = (esr >> (i * 4)) & 0xF;
        print_hex_digit(digit);
    }
    early_print("\n");
    
#elif defined(ARCH_X86_64)
    early_print("Fault address: 0x");
    uint64_t fault_addr = ctx->cr2;
    for (int i = 15; i >= 0; i--) {
        int digit = (fault_addr >> (i * 4)) & 0xF;
        print_hex_digit(digit);
    }
    early_print("\n");
    
    early_print("Error code: 0x");
    uint64_t error_code = ctx->error_code;
    for (int i = 7; i >= 0; i--) {
        int digit = (error_code >> (i * 4)) & 0xF;
        print_hex_digit(digit);
    }
    early_print("\n");
    
    // Decode error code
    if (error_code & 1) {
        early_print("  Protection violation ");
    } else {
        early_print("  Page not present ");
    }
    
    if (error_code & 2) {
        early_print("(write access)\n");
    } else {
        early_print("(read access)\n");
    }
    
    if (error_code & 4) {
        early_print("  User mode access\n");
    } else {
        early_print("  Supervisor mode access\n");
    }
#else
    early_print("Page fault details not available for this architecture\n");
#endif
    
    exception_print_info(exception_num, ctx);
    early_print("\nSystem will halt.\n");
    
    arch_interrupts_enable(0);
    arch_halt();
}

/**
 * Print exception information
 */
void exception_print_info(uint32_t exception_num, struct exception_context *ctx)
{
    if (!ctx) {
        early_print("No exception context available\n");
        return;
    }
    
    // Print exception type
    early_print("Exception type: ");
    switch (exception_num) {
        case EXCEPTION_DIVIDE_BY_ZERO:
            early_print("Divide by Zero");
            break;
        case EXCEPTION_DEBUG:
            early_print("Debug");
            break;
        case EXCEPTION_BREAKPOINT:
            early_print("Breakpoint");
            break;
        case EXCEPTION_INVALID_OPCODE:
            early_print("Invalid Opcode");
            break;
        case EXCEPTION_GENERAL_PROTECTION:
            early_print("General Protection Fault");
            break;
        case EXCEPTION_PAGE_FAULT:
            early_print("Page Fault");
            break;
        case EXCEPTION_ALIGNMENT_CHECK:
            early_print("Alignment Check");
            break;
#ifdef ARCH_ARM64
        case EXCEPTION_SYNC:
            early_print("Synchronous Exception");
            break;
        case EXCEPTION_IRQ:
            early_print("IRQ");
            break;
        case EXCEPTION_FIQ:
            early_print("FIQ");
            break;
        case EXCEPTION_SERROR:
            early_print("SError");
            break;
#endif
        default:
            early_print("Unknown (");
            char num_str[16];
            uint32_t num = exception_num;
            int pos = 0;
            if (num == 0) {
                num_str[pos++] = '0';
            } else {
                char temp[16];
                int temp_pos = 0;
                while (num > 0) {
                    temp[temp_pos++] = '0' + (num % 10);
                    num /= 10;
                }
                while (temp_pos > 0) {
                    num_str[pos++] = temp[--temp_pos];
                }
            }
            num_str[pos] = 0;
            early_print(num_str);
            early_print(")");
            break;
    }
    early_print("\n");
    
    // Print register state
#ifdef ARCH_ARM64
    early_print("PC (ELR_EL1): 0x");
    uint64_t pc = ctx->elr;
    for (int i = 15; i >= 0; i--) {
        int digit = (pc >> (i * 4)) & 0xF;
        print_hex_digit(digit);
    }
    early_print("\n");
    
    early_print("SP: 0x");
    uint64_t sp = ctx->sp;
    for (int i = 15; i >= 0; i--) {
        int digit = (sp >> (i * 4)) & 0xF;
        print_hex_digit(digit);
    }
    early_print("\n");
    
#elif defined(ARCH_X86_64)
    early_print("RIP: 0x");
    uint64_t rip = ctx->rip;
    for (int i = 15; i >= 0; i--) {
        int digit = (rip >> (i * 4)) & 0xF;
        print_hex_digit(digit);
    }
    early_print("\n");
    
    early_print("RSP: 0x");
    uint64_t rsp = ctx->rsp;
    for (int i = 15; i >= 0; i--) {
        int digit = (rsp >> (i * 4)) & 0xF;
        print_hex_digit(digit);
    }
    early_print("\n");
    
    early_print("RFLAGS: 0x");
    uint64_t rflags = ctx->rflags;
    for (int i = 7; i >= 0; i--) {
        int digit = (rflags >> (i * 4)) & 0xF;
        print_hex_digit(digit);
    }
    early_print("\n");
#endif
}

/**
 * Test memory allocation (for Phase 3 validation)
 */
void test_memory_allocation(void)
{
    early_print("\n=== Memory Allocation Test ===\n");
    
    // Test page allocation
    early_print("Testing page allocation...\n");
    void *page1 = memory_alloc_pages(1);
    void *page2 = memory_alloc_pages(2);
    void *page3 = memory_alloc_pages(4);
    
    if (page1) {
        early_print("Single page allocated: 0x");
        uint64_t addr = (uint64_t)page1;
        for (int i = 7; i >= 0; i--) {
            int digit = (addr >> (i * 4)) & 0xF;
            print_hex_digit(digit);
        }
        early_print("\n");
    } else {
        early_print("Single page allocation failed\n");
    }
    
    if (page2) {
        early_print("Two pages allocated: 0x");
        uint64_t addr = (uint64_t)page2;
        for (int i = 7; i >= 0; i--) {
            int digit = (addr >> (i * 4)) & 0xF;
            print_hex_digit(digit);
        }
        early_print("\n");
    } else {
        early_print("Two page allocation failed\n");
    }
    
    if (page3) {
        early_print("Four pages allocated: 0x");
        uint64_t addr = (uint64_t)page3;
        for (int i = 7; i >= 0; i--) {
            int digit = (addr >> (i * 4)) & 0xF;
            print_hex_digit(digit);
        }
        early_print("\n");
    } else {
        early_print("Four page allocation failed\n");
    }
    
    // Test memory writing (verify allocation works)
    if (page1) {
        early_print("Testing memory write...\n");
        *(uint64_t *)page1 = 0xDEADBEEFCAFEBABEULL;
        uint64_t read_val = *(uint64_t *)page1;
        if (read_val == 0xDEADBEEFCAFEBABEULL) {
            early_print("Memory write/read test: PASS\n");
        } else {
            early_print("Memory write/read test: FAIL\n");
        }
    }
    
    // Free allocations
    if (page1) memory_free_pages(page1, 1);
    if (page2) memory_free_pages(page2, 2);
    if (page3) memory_free_pages(page3, 4);
    
    early_print("Memory test complete\n");
    early_print("=============================\n\n");
}
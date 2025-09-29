/*
 * ARM64 Architecture Initialization
 */

#include "kernel.h"

// PL011 UART base address (QEMU virt machine)
#define UART_BASE 0x09000000
#define UART_DR   (UART_BASE + 0x00)  // Data register
#define UART_FR   (UART_BASE + 0x18)  // Flag register
#define UART_FR_TXFF (1 << 5)         // Transmit FIFO full

// Memory-mapped I/O functions
static inline void mmio_write(uint64_t addr, uint32_t value)
{
    *(volatile uint32_t*)addr = value;
}

static inline uint32_t mmio_read(uint64_t addr)
{
    return *(volatile uint32_t*)addr;
}

void arch_init(void)
{
    // Basic ARM64 initialization
    // UART should already be initialized by firmware/bootloader
    
    // For now, just print that we're in ARM64 init
    arch_early_print("ARM64 architecture initialization\n");
}

void arch_early_print(const char *str)
{
    // Simple UART output for early boot debugging
    while (*str) {
        // Wait for UART to be ready (TX FIFO not full)
        while (mmio_read(UART_FR) & UART_FR_TXFF)
            ;
        
        // Write character to UART
        mmio_write(UART_DR, *str);
        str++;
    }
}

void arch_halt(void)
{
    arch_early_print("ARM64: Halting processor\n");
    
    // Disable interrupts and halt
    __asm__ volatile (
        "msr daifset, #0xf\n"  // Disable all interrupts
        "1: wfi\n"             // Wait for interrupt (low power)
        "b 1b\n"               // Loop forever
        ::: "memory"
    );
    
    // Should never reach here
    __builtin_unreachable();
}
/*
 * MiniOS Kernel Main
 * Cross-platform kernel entry point
 */

#include "kernel.h"

void kernel_main(void)
{
    // Architecture-specific initialization
    arch_init();
    
    // Print welcome message
    early_print("MiniOS " KERNEL_VERSION " Starting...\n");
    
#ifdef ARCH_ARM64
    early_print("Architecture: ARM64 (AArch64)\n");
#elif defined(ARCH_X86_64)
    early_print("Architecture: x86-64 (AMD64)\n");
#endif
    
    early_print("Kernel loaded successfully!\n");
    
#ifdef DEBUG
    early_print("Debug build - verbose logging enabled\n");
#endif
    
    // TODO: Initialize memory management
    early_print("TODO: Memory management initialization\n");
    
    // TODO: Initialize process management  
    early_print("TODO: Process management initialization\n");
    
    // TODO: Initialize device drivers
    early_print("TODO: Device driver initialization\n");
    
    // TODO: Start shell
    early_print("TODO: Shell initialization\n");
    
    early_print("Kernel initialization complete!\n");
    early_print("MiniOS is ready (basic version)\n");
    
    // For now, just halt - later we'll start a shell
    early_print("Halting system (no shell yet)...\n");
    arch_halt();
}

void kernel_panic(const char *message)
{
    early_print("\n*** KERNEL PANIC ***\n");
    early_print("Panic: ");
    early_print(message);
    early_print("\nSystem halted.\n");
    
    arch_halt();
}

void early_print(const char *str)
{
    // Delegate to architecture-specific implementation
    arch_early_print(str);
}
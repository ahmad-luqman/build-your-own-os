/*
 * MiniOS Kernel Main
 * Cross-platform kernel entry point
 */

#include "kernel.h"

void kernel_main(struct boot_info *boot_info)
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
    
    // Validate boot information
    if (boot_info && boot_info_valid(boot_info)) {
        early_print("Boot info: Valid\n");
        
        // Display architecture
        if (boot_info->arch == BOOT_ARCH_ARM64) {
            early_print("Boot arch: ARM64\n");
        } else if (boot_info->arch == BOOT_ARCH_X86_64) {
            early_print("Boot arch: x86-64\n");
        }
        
        // Display memory information
        if (boot_info->memory_map_entries > 0) {
            early_print("Memory regions detected: ");
            // Simple number to string conversion
            char num_str[16];
            int entries = boot_info->memory_map_entries;
            int pos = 0;
            if (entries == 0) {
                num_str[pos++] = '0';
            } else {
                char temp[16];
                int temp_pos = 0;
                while (entries > 0) {
                    temp[temp_pos++] = '0' + (entries % 10);
                    entries /= 10;
                }
                while (temp_pos > 0) {
                    num_str[pos++] = temp[--temp_pos];
                }
            }
            num_str[pos] = 0;
            early_print(num_str);
            early_print("\n");
        } else {
            early_print("No memory map available\n");
        }
        
        // Display graphics information
        if (boot_info->framebuffer.framebuffer != 0) {
            early_print("Framebuffer available\n");
        } else {
            early_print("No framebuffer available\n");
        }
        
        // Display command line
        if (boot_info->cmdline[0] != 0) {
            early_print("Command line: ");
            early_print(boot_info->cmdline);
            early_print("\n");
        }
        
    } else {
        early_print("Boot info: Invalid or missing\n");
    }
    
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
    early_print("MiniOS is ready (Phase 2 - Enhanced bootloader)\n");
    
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
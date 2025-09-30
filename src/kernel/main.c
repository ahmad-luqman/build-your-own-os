/*
 * MiniOS Kernel Main
 * Cross-platform kernel entry point
 */

#include "kernel.h"
#include "process.h"
#include "syscall.h"

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
    
    // Phase 3: Initialize memory management
    if (memory_init(boot_info) < 0) {
        kernel_panic("Memory management initialization failed");
    }
    
    // Phase 3: Initialize exception handling
    if (exception_init() < 0) {
        kernel_panic("Exception handling initialization failed");
    }
    
    // Phase 3: Test memory allocation
    test_memory_allocation();
    
    // Phase 4: Initialize device drivers and system services
    early_print("Phase 4: Initializing device drivers and system services...\n");
    
    // Initialize device subsystem
    if (device_init(boot_info) < 0) {
        kernel_panic("Device subsystem initialization failed");
    }
    
    // Initialize timer services
    if (timer_init() < 0) {
        kernel_panic("Timer subsystem initialization failed");
    }
    
    // Enable scheduler timer (basic scheduling support)
    if (timer_enable_scheduler() < 0) {
        early_print("Warning: Failed to enable timer scheduler\n");
    }
    
    // Show device information
    device_list_all();
    
    // Show timer information
    struct timer_info timer_info;
    timer_get_info(&timer_info);
    early_print("Timer frequency: ");
    
    // Convert frequency to string for display
    char freq_str[32];
    int pos = 0;
    uint64_t freq = timer_info.frequency;
    if (freq == 0) {
        freq_str[pos++] = '0';
    } else {
        char temp[32];
        int temp_pos = 0;
        while (freq > 0) {
            temp[temp_pos++] = '0' + (freq % 10);
            freq /= 10;
        }
        while (temp_pos > 0) {
            freq_str[pos++] = temp[--temp_pos];
        }
    }
    freq_str[pos] = 0;
    early_print(freq_str);
    early_print(" Hz\n");
    
    // Initialize UART services
    if (uart_init() < 0) {
        kernel_panic("UART subsystem initialization failed");
    }
    
    // Test UART functionality
    uart_test_output();
    
    // Initialize interrupt management
    if (interrupt_init() < 0) {
        kernel_panic("Interrupt subsystem initialization failed");
    }
    
    // Test interrupt functionality
    show_interrupt_controllers();
    
    // Initialize process management
    if (process_init() < 0) {
        kernel_panic("Process management initialization failed");
    }
    
    // Initialize scheduler
    scheduler_init();
    
    // Initialize system call interface
    if (syscall_init() < 0) {
        kernel_panic("System call interface initialization failed");
    }
    
    early_print("Kernel initialization complete!\n");
    early_print("MiniOS is ready (Phase 4 - Device Drivers & System Services)\n");
    
    // Create initial system tasks
    early_print("Creating initial system tasks...\n");
    
    // Create init task (higher priority)
    int init_pid = process_create(init_task, NULL, "init", PRIORITY_NORMAL);
    if (init_pid < 0) {
        kernel_panic("Failed to create init task");
    }
    
    early_print("Starting scheduler with system services...\n");
    
    // Enable system call tracing for debugging
    syscall_enable_tracing(1);
    
    // Start the scheduler - this should not return
    scheduler_start();
    
    // Should never reach here
    early_print("ERROR: Scheduler returned unexpectedly\n");
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
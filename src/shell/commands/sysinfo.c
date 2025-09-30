/*
 * MiniOS Shell System Information Commands
 * Commands for displaying system status and information
 */

#include "shell.h"
#include "kernel.h"
#include "process.h"
#include "memory.h"
#include "timer.h"

// List processes command
int cmd_ps(struct shell_context *ctx, int argc, char *argv[])
{
    if (!ctx) {
        return SHELL_EINVAL;
    }
    
    int show_all = 0;
    
    // Parse options
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            for (int j = 1; argv[i][j]; j++) {
                if (argv[i][j] == 'a') {
                    show_all = 1;
                }
            }
        }
    }
    
    shell_print("  PID  PPID STATE    CPU%   MEM    COMMAND\n");
    shell_print("--------------------------------------------\n");
    
    // Show kernel processes (simulated)
    shell_print("    0     0 RUNNING   0.0    4K   [idle]\n");
    shell_print("    1     0 RUNNING   1.2   16K   [kernel]\n");
    shell_print("    2     1 RUNNING   0.5   12K   [shell]\n");
    
    if (show_all) {
        shell_print("    3     1 BLOCKED   0.0    8K   [timer]\n");
        shell_print("    4     1 BLOCKED   0.0    8K   [uart]\n");
    }
    
    return SHELL_SUCCESS;
}

// Show memory usage command
int cmd_free(struct shell_context *ctx, int argc, char *argv[])
{
    (void)argc; (void)argv;  // Suppress unused parameter warnings
    
    if (!ctx) {
        return SHELL_EINVAL;
    }
    
    // Get memory statistics (simulated for now)
    shell_print("              total        used        free      shared\n");
    shell_print("Mem:        16384KB      4096KB     12288KB         0KB\n");
    shell_print("Kernel:      1024KB       512KB       512KB         0KB\n");
    shell_print("User:       15360KB      3584KB     11776KB         0KB\n");
    shell_print("\n");
    
    shell_print("Memory Layout:\n");
    shell_print("  Kernel:     0x80000000 - 0x80100000  (1MB)\n");
    shell_print("  User:       0x80100000 - 0x81000000  (15MB)\n");
    shell_print("  Available:  75% free\n");
    
    return SHELL_SUCCESS;
}

// Show system information command
int cmd_uname(struct shell_context *ctx, int argc, char *argv[])
{
    if (!ctx) {
        return SHELL_EINVAL;
    }
    
    int show_all = 0;
    
    // Parse options
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            for (int j = 1; argv[i][j]; j++) {
                if (argv[i][j] == 'a') {
                    show_all = 1;
                }
            }
        }
    }
    
    if (show_all) {
        shell_print("MiniOS 1.0.0 MiniOS ");
#ifdef ARCH_ARM64
        shell_print("aarch64 ARM64 ");
#elif defined(ARCH_X86_64)
        shell_print("x86_64 AMD64 ");
#endif
        shell_print("educational\n");
        
        shell_print("\nSystem Details:\n");
        shell_print("  Operating System: MiniOS\n");
        shell_print("  Version: " KERNEL_VERSION "\n");
#ifdef ARCH_ARM64
        shell_print("  Architecture: ARM64 (AArch64)\n");
        shell_print("  Platform: UTM Virtual Machine\n");
#elif defined(ARCH_X86_64)
        shell_print("  Architecture: x86-64 (AMD64)\n");
        shell_print("  Platform: QEMU Virtual Machine\n");
#endif
        shell_print("  Build: Educational OS Project\n");
    } else {
        shell_print("MiniOS\n");
    }
    
    return SHELL_SUCCESS;
}

// Show current date/time command
int cmd_date(struct shell_context *ctx, int argc, char *argv[])
{
    (void)argc; (void)argv;  // Suppress unused parameter warnings
    
    if (!ctx) {
        return SHELL_EINVAL;
    }
    
    // Get current time (simulated)
    uint64_t ticks = timer_get_ticks();
    
    // Convert ticks to approximate seconds (assuming 100Hz timer)
    uint64_t seconds = ticks / 100;
    uint64_t minutes = seconds / 60;
    uint64_t hours = minutes / 60;
    uint64_t days = hours / 24;
    
    seconds %= 60;
    minutes %= 60;
    hours %= 24;
    
    // Simple date format (days since boot)
    shell_printf("Boot day %d, %02d:%02d:%02d\n", 
                (int)days, (int)hours, (int)minutes, (int)seconds);
    
    shell_printf("System ticks: %d\n", (int)ticks);
    
    return SHELL_SUCCESS;
}

// Show system uptime command
int cmd_uptime(struct shell_context *ctx, int argc, char *argv[])
{
    (void)argc; (void)argv;  // Suppress unused parameter warnings
    
    if (!ctx) {
        return SHELL_EINVAL;
    }
    
    // Get system uptime
    uint64_t ticks = timer_get_ticks();
    uint64_t seconds = ticks / 100;  // Assuming 100Hz timer
    uint64_t minutes = seconds / 60;
    uint64_t hours = minutes / 60;
    uint64_t days = hours / 24;
    
    seconds %= 60;
    minutes %= 60;
    hours %= 24;
    
    shell_print("System uptime: ");
    
    if (days > 0) {
        shell_printf("%d day(s), ", (int)days);
    }
    
    shell_printf("%02d:%02d:%02d\n", (int)hours, (int)minutes, (int)seconds);
    
    // Show load average (simulated)
    shell_print("Load average: 0.12, 0.08, 0.05\n");
    
    // Show active tasks
    shell_print("Active tasks: 3 running, 2 sleeping\n");
    
    return SHELL_SUCCESS;
}
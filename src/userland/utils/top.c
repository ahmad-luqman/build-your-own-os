/*
 * Top - Process monitor for MiniOS
 * Displays running processes and system information
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// User I/O and system call functions
int user_printf(const char *format, ...);
int user_puts(const char *str);
int user_getchar(void);
int user_syscall_ps(void *buffer, size_t size);
int user_syscall_meminfo(void *buffer, size_t size);
int user_syscall_sysinfo(void *buffer, size_t size);

// Process information structure (matching kernel's)
struct process_info {
    int pid;
    char name[32];
    int state;
    int priority;
    int cpu_time;
    int memory_usage;
};

// Memory information structure
struct memory_info {
    int total_memory;
    int used_memory;
    int free_memory;
    int kernel_memory;
    int user_memory;
};

// System information structure
struct system_info {
    int uptime;
    int total_processes;
    int running_processes;
    int context_switches;
    int interrupts;
};

// Display header
void display_header(void) {
    user_puts("\033[2J\033[H");  // Clear screen and move cursor to top
    user_puts("MiniOS Top - Process Monitor");
    user_puts("============================");
    user_puts("");
}

// Display system statistics
void display_system_stats(struct system_info *sysinfo, struct memory_info *meminfo) {
    user_printf("System uptime: %d seconds\n", sysinfo->uptime);
    user_printf("Processes: %d total, %d running\n", 
                sysinfo->total_processes, sysinfo->running_processes);
    user_printf("Context switches: %d\n", sysinfo->context_switches);
    user_puts("");
    
    user_printf("Memory: %dK total, %dK used, %dK free\n",
                meminfo->total_memory / 1024,
                meminfo->used_memory / 1024,
                meminfo->free_memory / 1024);
    user_printf("Kernel: %dK, User: %dK\n",
                meminfo->kernel_memory / 1024,
                meminfo->user_memory / 1024);
    user_puts("");
}

// Display process list header
void display_process_header(void) {
    user_puts("  PID  Name                 State    Pri   CPU   Mem");
    user_puts("====================================================");
}

// Get state name
const char *get_state_name(int state) {
    switch (state) {
        case 0: return "READY";
        case 1: return "RUN  ";
        case 2: return "BLOCK";
        case 3: return "TERM ";
        default: return "UNK  ";
    }
}

// Display process information
void display_process(struct process_info *proc) {
    user_printf("%5d  %-18s  %-5s  %3d  %5d  %4dK\n",
                proc->pid,
                proc->name,
                get_state_name(proc->state),
                proc->priority,
                proc->cpu_time,
                proc->memory_usage / 1024);
}

// Main top loop
void run_top(void) {
    struct system_info sysinfo;
    struct memory_info meminfo;
    struct process_info processes[32];
    int process_count;
    
    while (1) {
        // Get system information
        user_syscall_sysinfo(&sysinfo, sizeof(sysinfo));
        user_syscall_meminfo(&meminfo, sizeof(meminfo));
        
        // Get process list
        process_count = user_syscall_ps(processes, sizeof(processes));
        if (process_count > 32) process_count = 32;
        
        // Display information
        display_header();
        display_system_stats(&sysinfo, &meminfo);
        display_process_header();
        
        for (int i = 0; i < process_count; i++) {
            display_process(&processes[i]);
        }
        
        user_puts("");
        user_puts("Press 'q' to quit, any other key to refresh");
        
        // Check for input (non-blocking would be better)
        int ch = user_getchar();
        if (ch == 'q' || ch == 'Q') {
            break;
        }
        
        // In a real implementation, we'd sleep for a bit
        // For now, just continue to next iteration
    }
}

int main(int argc, char *argv[]) {
    user_puts("MiniOS Top - Process Monitor");
    user_puts("Starting process monitor...");
    user_puts("Press 'q' to quit at any time");
    user_puts("");
    
    run_top();
    
    user_puts("\nTop exited.");
    return 0;
}

// Stub implementations
int user_printf(const char *format, ...) { return 0; }
int user_puts(const char *str) { return 0; }
int user_getchar(void) { return 'q'; }  // Auto-quit for demo

int user_syscall_ps(void *buffer, size_t size) {
    // Mock process data
    struct process_info *procs = (struct process_info *)buffer;
    
    // Mock process 1: init
    procs[0].pid = 1;
    strcpy(procs[0].name, "init");
    procs[0].state = 1;  // Running
    procs[0].priority = 0;
    procs[0].cpu_time = 1000;
    procs[0].memory_usage = 4096;
    
    // Mock process 2: shell
    procs[1].pid = 2;
    strcpy(procs[1].name, "shell");
    procs[1].state = 0;  // Ready
    procs[1].priority = 1;
    procs[1].cpu_time = 500;
    procs[1].memory_usage = 8192;
    
    return 2;  // Return number of processes
}

int user_syscall_meminfo(void *buffer, size_t size) {
    struct memory_info *mem = (struct memory_info *)buffer;
    mem->total_memory = 16 * 1024 * 1024;  // 16MB
    mem->used_memory = 8 * 1024 * 1024;    // 8MB
    mem->free_memory = 8 * 1024 * 1024;    // 8MB
    mem->kernel_memory = 4 * 1024 * 1024;  // 4MB
    mem->user_memory = 4 * 1024 * 1024;    // 4MB
    return 0;
}

int user_syscall_sysinfo(void *buffer, size_t size) {
    struct system_info *sys = (struct system_info *)buffer;
    sys->uptime = 3600;  // 1 hour
    sys->total_processes = 2;
    sys->running_processes = 1;
    sys->context_switches = 1000;
    sys->interrupts = 5000;
    return 0;
}
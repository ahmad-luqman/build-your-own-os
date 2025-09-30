/*
 * Kill - Process termination utility
 * Sends signals to processes (simplified for MiniOS)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// User system call functions
int user_printf(const char *format, ...);
int user_puts(const char *str);
int user_syscall_kill(int pid, int signal);
int user_syscall_ps(void *buffer, size_t size);

// Signal definitions (simplified)
#define SIGTERM     15  // Terminate
#define SIGKILL     9   // Kill
#define SIGSTOP     19  // Stop
#define SIGCONT     18  // Continue

// Process info for validation
struct process_info {
    int pid;
    char name[32];
    int state;
};

// String to integer conversion
int str_to_int(const char *str) {
    int result = 0;
    int i = 0;
    int negative = 0;
    
    if (str[0] == '-') {
        negative = 1;
        i = 1;
    }
    
    while (str[i] >= '0' && str[i] <= '9') {
        result = result * 10 + (str[i] - '0');
        i++;
    }
    
    return negative ? -result : result;
}

// Get signal number from name
int get_signal_number(const char *signal_name) {
    if (strcmp(signal_name, "TERM") == 0 || strcmp(signal_name, "15") == 0) {
        return SIGTERM;
    } else if (strcmp(signal_name, "KILL") == 0 || strcmp(signal_name, "9") == 0) {
        return SIGKILL;
    } else if (strcmp(signal_name, "STOP") == 0 || strcmp(signal_name, "19") == 0) {
        return SIGSTOP;
    } else if (strcmp(signal_name, "CONT") == 0 || strcmp(signal_name, "18") == 0) {
        return SIGCONT;
    } else {
        return str_to_int(signal_name);
    }
}

// Get signal name
const char *get_signal_name(int signal) {
    switch (signal) {
        case SIGTERM: return "TERM";
        case SIGKILL: return "KILL";
        case SIGSTOP: return "STOP";
        case SIGCONT: return "CONT";
        default: return "UNKNOWN";
    }
}

// Check if process exists
int process_exists(int pid) {
    struct process_info processes[32];
    int count = user_syscall_ps(processes, sizeof(processes));
    
    for (int i = 0; i < count && i < 32; i++) {
        if (processes[i].pid == pid) {
            return 1;
        }
    }
    return 0;
}

// Display usage information
void display_usage(void) {
    user_puts("Usage: kill [-signal] <pid> [pid2] ...");
    user_puts("       kill -l");
    user_puts("");
    user_puts("Send a signal to one or more processes.");
    user_puts("");
    user_puts("Options:");
    user_puts("  -l            List available signals");
    user_puts("  -signal       Signal to send (default: TERM)");
    user_puts("                Can be number (9, 15) or name (KILL, TERM)");
    user_puts("");
    user_puts("Examples:");
    user_puts("  kill 1234     Send TERM signal to process 1234");
    user_puts("  kill -9 1234  Send KILL signal to process 1234");
    user_puts("  kill -KILL 1234  Same as above");
}

// List available signals
void list_signals(void) {
    user_puts("Available signals:");
    user_puts("  9  KILL  - Terminate immediately (cannot be caught)");
    user_puts(" 15  TERM  - Terminate gracefully (default)");
    user_puts(" 18  CONT  - Continue stopped process");
    user_puts(" 19  STOP  - Stop process");
}

int main(int argc, char *argv[]) {
    int signal = SIGTERM;  // Default signal
    int pid_start = 1;     // Index where PIDs start in argv
    
    if (argc < 2) {
        display_usage();
        return 1;
    }
    
    // Check for options
    if (strcmp(argv[1], "-l") == 0) {
        list_signals();
        return 0;
    }
    
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        display_usage();
        return 0;
    }
    
    // Check for signal specification
    if (argv[1][0] == '-' && strlen(argv[1]) > 1) {
        signal = get_signal_number(&argv[1][1]);
        if (signal <= 0) {
            user_printf("kill: invalid signal '%s'\n", &argv[1][1]);
            user_puts("Use 'kill -l' to list available signals");
            return 1;
        }
        pid_start = 2;
    }
    
    if (pid_start >= argc) {
        user_puts("kill: no process ID specified");
        display_usage();
        return 1;
    }
    
    int errors = 0;
    
    // Process each PID
    for (int i = pid_start; i < argc; i++) {
        int pid = str_to_int(argv[i]);
        
        if (pid <= 0) {
            user_printf("kill: invalid process ID '%s'\n", argv[i]);
            errors++;
            continue;
        }
        
        // Check if process exists
        if (!process_exists(pid)) {
            user_printf("kill: process %d not found\n", pid);
            errors++;
            continue;
        }
        
        // Send signal
        int result = user_syscall_kill(pid, signal);
        if (result == 0) {
            user_printf("Sent %s signal to process %d\n", get_signal_name(signal), pid);
        } else {
            user_printf("kill: failed to send signal to process %d\n", pid);
            errors++;
        }
    }
    
    return errors > 0 ? 1 : 0;
}

// Stub implementations
int user_printf(const char *format, ...) { return 0; }
int user_puts(const char *str) { return 0; }

int user_syscall_kill(int pid, int signal) {
    // Mock implementation - always succeed for demo
    return 0;
}

int user_syscall_ps(void *buffer, size_t size) {
    // Mock process data
    struct process_info *procs = (struct process_info *)buffer;
    
    procs[0].pid = 1;
    strcpy(procs[0].name, "init");
    procs[0].state = 1;
    
    procs[1].pid = 2;
    strcpy(procs[1].name, "shell");
    procs[1].state = 0;
    
    return 2;
}
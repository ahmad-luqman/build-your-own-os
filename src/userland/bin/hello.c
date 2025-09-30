/*
 * Hello World - Simple user program for MiniOS
 * Demonstrates basic user program execution and argument handling
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simple printf implementation for user programs
int user_printf(const char *format, ...) {
    // For now, we'll use a simple syscall-based output
    // This would be implemented via system calls in a full version
    return 0;
}

int main(int argc, char *argv[]) {
    user_printf("Hello from MiniOS user program!\n");
    user_printf("Program: %s\n", argv[0]);
    
    if (argc > 1) {
        user_printf("Arguments received:\n");
        for (int i = 1; i < argc; i++) {
            user_printf("  argv[%d]: %s\n", i, argv[i]);
        }
    } else {
        user_printf("No arguments provided.\n");
    }
    
    user_printf("User program demonstration complete.\n");
    
    return 0;
}
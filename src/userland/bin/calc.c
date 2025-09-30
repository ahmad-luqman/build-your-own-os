/*
 * Simple Calculator - Interactive calculator user program
 * Demonstrates user input, computation, and interactive programs
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simple I/O functions for user programs
int user_printf(const char *format, ...);
int user_getchar(void);
int user_puts(const char *str);

// Simple string to integer conversion
int str_to_int(const char *str) {
    int result = 0;
    int sign = 1;
    int i = 0;
    
    if (str[0] == '-') {
        sign = -1;
        i = 1;
    }
    
    while (str[i] >= '0' && str[i] <= '9') {
        result = result * 10 + (str[i] - '0');
        i++;
    }
    
    return result * sign;
}

// Simple tokenizer for calculator input
void tokenize_input(char *input, char *tokens[], int *count) {
    *count = 0;
    char *token = input;
    
    while (*token && *count < 10) {
        // Skip whitespace
        while (*token == ' ' || *token == '\t') {
            token++;
        }
        
        if (*token == '\0') {
            break;
        }
        
        tokens[*count] = token;
        (*count)++;
        
        // Find end of token
        while (*token && *token != ' ' && *token != '\t') {
            token++;
        }
        
        if (*token) {
            *token = '\0';
            token++;
        }
    }
}

// Calculator operations
int calculate(int a, char op, int b) {
    switch (op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': return (b != 0) ? a / b : 0;
        case '%': return (b != 0) ? a % b : 0;
        default: return 0;
    }
}

int main(int argc, char *argv[]) {
    user_puts("MiniOS Calculator");
    user_puts("================");
    user_puts("Enter expressions like: 10 + 5");
    user_puts("Supported operations: + - * / %");
    user_puts("Type 'quit' to exit");
    user_puts("");
    
    char input[128];
    char *tokens[10];
    int token_count;
    
    while (1) {
        user_printf("calc> ");
        
        // Read input (simplified for demo)
        int i = 0;
        int ch;
        while (i < 127 && (ch = user_getchar()) != '\n' && ch != EOF) {
            input[i++] = ch;
        }
        input[i] = '\0';
        
        // Check for quit
        if (strcmp(input, "quit") == 0) {
            user_puts("Goodbye!");
            break;
        }
        
        // Tokenize input
        tokenize_input(input, tokens, &token_count);
        
        if (token_count == 3) {
            int a = str_to_int(tokens[0]);
            char op = tokens[1][0];
            int b = str_to_int(tokens[2]);
            
            int result = calculate(a, op, b);
            user_printf("Result: %d\n", result);
        } else if (token_count == 1) {
            // Single number
            int num = str_to_int(tokens[0]);
            user_printf("Value: %d\n", num);
        } else {
            user_puts("Invalid expression. Use format: number operator number");
        }
        
        user_puts("");
    }
    
    return 0;
}

// Stub implementations for user I/O
int user_printf(const char *format, ...) {
    // This would use system calls to output text
    return 0;
}

int user_getchar(void) {
    // This would use system calls to read input
    return 0;
}

int user_puts(const char *str) {
    // This would use system calls to output a string
    return 0;
}
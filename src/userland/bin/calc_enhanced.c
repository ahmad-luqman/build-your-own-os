/*
 * Enhanced Calculator - Advanced calculator with scientific functions
 * Demonstrates Phase 8.1 Enhanced Applications with MiniOS C library
 * Uses proper stdio, stdlib, string, and math libraries
 */

#include "../lib/minios_libc/stdio.h"
#include "../lib/minios_libc/stdlib.h"
#include "../lib/minios_libc/string.h"
#include "../lib/minios_libc/math.h"

// Calculator history
#define MAX_HISTORY 10
static double history[MAX_HISTORY];
static int history_count = 0;

// Variables storage
#define MAX_VARIABLES 26  // a-z
static double variables[MAX_VARIABLES];
static int var_set[MAX_VARIABLES];

// Function prototypes
void show_help(void);
void show_history(void);
void show_variables(void);
double evaluate_expression(const char *expr);
double parse_number_or_variable(const char *token);
void set_variable(char var, double value);
double get_variable(char var);
int is_operator(char c);
double apply_function(const char *func, double arg);

int main(int argc, char *argv[]) {
    printf("MiniOS Enhanced Calculator v2.0\n");
    printf("===============================\n");
    printf("Scientific calculator with variables and functions\n");
    printf("Type 'help' for commands, 'quit' to exit\n\n");
    
    // Initialize variables
    memset(variables, 0, sizeof(variables));
    memset(var_set, 0, sizeof(var_set));
    
    char input[256];
    
    while (1) {
        printf("calc> ");
        fflush(stdout);
        
        // Read input line
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        
        // Remove newline
        char *newline = strchr(input, '\n');
        if (newline) *newline = '\0';
        
        // Skip empty lines
        if (strlen(input) == 0) {
            continue;
        }
        
        // Handle commands
        if (strcmp(input, "quit") == 0 || strcmp(input, "exit") == 0) {
            printf("Goodbye!\n");
            break;
        } else if (strcmp(input, "help") == 0) {
            show_help();
            continue;
        } else if (strcmp(input, "history") == 0) {
            show_history();
            continue;
        } else if (strcmp(input, "vars") == 0 || strcmp(input, "variables") == 0) {
            show_variables();
            continue;
        } else if (strcmp(input, "clear") == 0) {
            history_count = 0;
            memset(var_set, 0, sizeof(var_set));
            printf("History and variables cleared.\n");
            continue;
        }
        
        // Check for variable assignment (e.g., "x = 5")
        char *equals = strchr(input, '=');
        if (equals && equals > input && equals[-1] >= 'a' && equals[-1] <= 'z') {
            char var = equals[-1];
            double value = evaluate_expression(equals + 1);
            set_variable(var, value);
            printf("%c = %.6g\n", var, value);
            continue;
        }
        
        // Evaluate expression
        double result = evaluate_expression(input);
        
        // Store in history
        if (history_count < MAX_HISTORY) {
            history[history_count++] = result;
        } else {
            // Shift history
            for (int i = 0; i < MAX_HISTORY - 1; i++) {
                history[i] = history[i + 1];
            }
            history[MAX_HISTORY - 1] = result;
        }
        
        printf("= %.6g\n", result);
    }
    
    return 0;
}

void show_help(void) {
    printf("\nMiniOS Enhanced Calculator Help\n");
    printf("===============================\n");
    printf("Basic Operations:\n");
    printf("  +, -, *, /     - Basic arithmetic\n");
    printf("  ^              - Power (e.g., 2^3 = 8)\n");
    printf("  %              - Modulo\n");
    printf("\nScientific Functions:\n");
    printf("  sin(x), cos(x), tan(x)    - Trigonometric functions\n");
    printf("  asin(x), acos(x), atan(x) - Inverse trig functions\n");
    printf("  exp(x), log(x), log10(x)  - Exponential and logarithm\n");
    printf("  sqrt(x), abs(x)           - Square root and absolute value\n");
    printf("  floor(x), ceil(x)         - Rounding functions\n");
    printf("\nConstants:\n");
    printf("  pi             - 3.14159...\n");
    printf("  e              - 2.71828...\n");
    printf("\nVariables:\n");
    printf("  x = 5          - Set variable x to 5\n");
    printf("  x + 2          - Use variable x in expression\n");
    printf("\nCommands:\n");
    printf("  history        - Show calculation history\n");
    printf("  vars           - Show all variables\n");
    printf("  clear          - Clear history and variables\n");
    printf("  help           - Show this help\n");
    printf("  quit           - Exit calculator\n\n");
}

void show_history(void) {
    if (history_count == 0) {
        printf("No calculation history.\n");
        return;
    }
    
    printf("\nCalculation History:\n");
    printf("===================\n");
    for (int i = 0; i < history_count; i++) {
        printf("%2d: %.6g\n", i + 1, history[i]);
    }
    printf("\n");
}

void show_variables(void) {
    int any_set = 0;
    
    printf("\nVariables:\n");
    printf("==========\n");
    for (int i = 0; i < MAX_VARIABLES; i++) {
        if (var_set[i]) {
            printf("%c = %.6g\n", 'a' + i, variables[i]);
            any_set = 1;
        }
    }
    
    if (!any_set) {
        printf("No variables set.\n");
    }
    printf("\n");
}

// Simple expression evaluator (handles basic operations and functions)
double evaluate_expression(const char *expr) {
    if (!expr) return 0.0;
    
    // Skip whitespace
    while (*expr == ' ' || *expr == '\t') expr++;
    
    // Handle constants
    if (strncmp(expr, "pi", 2) == 0) {
        return M_PI;
    } else if (strncmp(expr, "e", 1) == 0 && (expr[1] == '\0' || expr[1] == ' ')) {
        return M_E;
    }
    
    // Handle functions
    if (strncmp(expr, "sin(", 4) == 0) {
        double arg = evaluate_expression(expr + 4);  // Simplified
        return sin(arg);
    } else if (strncmp(expr, "cos(", 4) == 0) {
        double arg = evaluate_expression(expr + 4);
        return cos(arg);
    } else if (strncmp(expr, "tan(", 4) == 0) {
        double arg = evaluate_expression(expr + 4);
        return tan(arg);
    } else if (strncmp(expr, "sqrt(", 5) == 0) {
        double arg = evaluate_expression(expr + 5);
        return sqrt(arg);
    } else if (strncmp(expr, "exp(", 4) == 0) {
        double arg = evaluate_expression(expr + 4);
        return exp(arg);
    } else if (strncmp(expr, "log(", 4) == 0) {
        double arg = evaluate_expression(expr + 4);
        return log(arg);
    } else if (strncmp(expr, "log10(", 6) == 0) {
        double arg = evaluate_expression(expr + 6);
        return log10(arg);
    } else if (strncmp(expr, "abs(", 4) == 0) {
        double arg = evaluate_expression(expr + 4);
        return fabs(arg);
    } else if (strncmp(expr, "floor(", 6) == 0) {
        double arg = evaluate_expression(expr + 6);
        return floor(arg);
    } else if (strncmp(expr, "ceil(", 5) == 0) {
        double arg = evaluate_expression(expr + 5);
        return ceil(arg);
    }
    
    // Simple binary operation parser (left to right evaluation)
    char expr_copy[256];
    strncpy(expr_copy, expr, sizeof(expr_copy) - 1);
    expr_copy[sizeof(expr_copy) - 1] = '\0';
    
    // Tokenize simple expressions like "5 + 3" or "x * 2"
    char *tokens[10];
    int token_count = 0;
    char *token = strtok(expr_copy, " \t");
    
    while (token && token_count < 10) {
        tokens[token_count++] = token;
        token = strtok(NULL, " \t");
    }
    
    if (token_count == 1) {
        // Single token - number or variable
        return parse_number_or_variable(tokens[0]);
    } else if (token_count == 3) {
        // Binary operation
        double a = parse_number_or_variable(tokens[0]);
        char op = tokens[1][0];
        double b = parse_number_or_variable(tokens[2]);
        
        switch (op) {
            case '+': return a + b;
            case '-': return a - b;
            case '*': return a * b;
            case '/': return (b != 0.0) ? a / b : 0.0;
            case '^': return pow(a, b);
            case '%': return fmod(a, b);
            default: return 0.0;
        }
    }
    
    // Default: try to parse as number
    return atof(expr);
}

double parse_number_or_variable(const char *token) {
    if (!token) return 0.0;
    
    // Check if it's a variable (single letter a-z)
    if (strlen(token) == 1 && token[0] >= 'a' && token[0] <= 'z') {
        return get_variable(token[0]);
    }
    
    // Parse as number
    return atof(token);
}

void set_variable(char var, double value) {
    if (var >= 'a' && var <= 'z') {
        int index = var - 'a';
        variables[index] = value;
        var_set[index] = 1;
    }
}

double get_variable(char var) {
    if (var >= 'a' && var <= 'z') {
        int index = var - 'a';
        if (var_set[index]) {
            return variables[index];
        }
    }
    return 0.0;
}
/*
 * MiniOS Shell Command Parser
 * Parse and execute shell commands
 */

#include "shell.h"
#include "kernel.h"

// Parse command line into structured format
int parse_command_line(const char *input, struct command_line *cmd)
{
    if (!input || !cmd) {
        return SHELL_EINVAL;
    }
    
    // Initialize command line structure
    memset(cmd, 0, sizeof(struct command_line));
    
    // Allocate argument array
    cmd->arguments = (char **)kmalloc(SHELL_MAX_ARGS * sizeof(char *));
    if (!cmd->arguments) {
        return SHELL_ENOMEM;
    }
    
    // Parse arguments using existing parser
    cmd->argument_count = shell_parse_command(input, cmd->arguments, SHELL_MAX_ARGS);
    
    if (cmd->argument_count == 0) {
        kfree(cmd->arguments);
        return SHELL_EINVAL;
    }
    
    // First argument is the command
    cmd->command = cmd->arguments[0];
    
    // Check for I/O redirection (simple implementation)
    for (int i = 1; i < cmd->argument_count; i++) {
        if (cmd->arguments[i][0] == '>') {
            // Output redirection
            if (strlen(cmd->arguments[i]) > 1) {
                // Format: >filename
                cmd->output_redirect = cmd->arguments[i] + 1;
            } else if (i + 1 < cmd->argument_count) {
                // Format: > filename
                cmd->output_redirect = cmd->arguments[i + 1];
                // Remove redirection from arguments
                for (int j = i; j < cmd->argument_count - 2; j++) {
                    cmd->arguments[j] = cmd->arguments[j + 2];
                }
                cmd->argument_count -= 2;
            }
            break;
        } else if (cmd->arguments[i][0] == '<') {
            // Input redirection
            if (strlen(cmd->arguments[i]) > 1) {
                // Format: <filename
                cmd->input_redirect = cmd->arguments[i] + 1;
            } else if (i + 1 < cmd->argument_count) {
                // Format: < filename
                cmd->input_redirect = cmd->arguments[i + 1];
                // Remove redirection from arguments
                for (int j = i; j < cmd->argument_count - 2; j++) {
                    cmd->arguments[j] = cmd->arguments[j + 2];
                }
                cmd->argument_count -= 2;
            }
            break;
        } else if (cmd->arguments[i][0] == '&') {
            // Background execution
            cmd->background = 1;
            // Remove & from arguments
            for (int j = i; j < cmd->argument_count - 1; j++) {
                cmd->arguments[j] = cmd->arguments[j + 1];
            }
            cmd->argument_count--;
            break;
        }
    }
    
    return SHELL_SUCCESS;
}

// Execute parsed command
int execute_command(struct shell_context *ctx, struct command_line *cmd)
{
    if (!ctx || !cmd || !cmd->command) {
        return SHELL_EINVAL;
    }
    
    // Store output redirection in context so commands can access it
    // Save the original value
    char *saved_output_redirect = ctx->output_redirect_file;
    ctx->output_redirect_file = cmd->output_redirect;
    
    // Set up I/O redirection if specified
    if (setup_io_redirection(cmd) < 0) {
        shell_print_error("Failed to set up I/O redirection\n");
        ctx->output_redirect_file = saved_output_redirect;
        return SHELL_ERROR;
    }
    
    // Try to execute as built-in command first
    int result = execute_builtin_command(ctx, cmd);
    
    if (result == SHELL_ENOENT) {
        // Command not found as built-in, try external program
        result = execute_external_program(ctx, cmd);
        
        if (result == SHELL_ENOENT) {
            shell_printf("Command not found: %s\n", cmd->command);
            ctx->output_redirect_file = saved_output_redirect;
            return SHELL_ENOENT;
        }
    }
    
    // Restore context
    ctx->output_redirect_file = saved_output_redirect;
    
    return result;
}

// Free command line resources
void free_command_line(struct command_line *cmd)
{
    if (!cmd) {
        return;
    }
    
    if (cmd->arguments) {
        kfree(cmd->arguments);
        cmd->arguments = NULL;
    }
    
    cmd->argument_count = 0;
    cmd->command = NULL;
    cmd->input_redirect = NULL;
    cmd->output_redirect = NULL;
    cmd->background = 0;
}

// Execute built-in command
int execute_builtin_command(struct shell_context *ctx, struct command_line *cmd)
{
    if (!ctx || !cmd || !cmd->command) {
        return SHELL_EINVAL;
    }
    
    // Find command in registry
    struct shell_command *shell_cmd = shell_find_command(cmd->command);
    if (!shell_cmd) {
        return SHELL_ENOENT;
    }
    
    // Validate argument count
    if (shell_cmd->min_args >= 0 && cmd->argument_count - 1 < shell_cmd->min_args) {
        shell_printf("Error: %s requires at least %d arguments\n", 
                    cmd->command, shell_cmd->min_args);
        return SHELL_EINVAL;
    }
    
    if (shell_cmd->max_args >= 0 && cmd->argument_count - 1 > shell_cmd->max_args) {
        shell_printf("Error: %s takes at most %d arguments\n", 
                    cmd->command, shell_cmd->max_args);
        return SHELL_EINVAL;
    }
    
    // Execute command handler
    return shell_cmd->handler(ctx, cmd->argument_count, cmd->arguments);
}

// Execute external program (future implementation)
int execute_external_program(struct shell_context *ctx, struct command_line *cmd)
{
    if (!ctx || !cmd || !cmd->command) {
        return SHELL_EINVAL;
    }
    
    // For now, just return not found
    // In the future, this would:
    // 1. Search PATH for executable
    // 2. Load program from filesystem
    // 3. Create new process
    // 4. Execute program with arguments
    
    return SHELL_ENOENT;
}

// Set up I/O redirection
int setup_io_redirection(struct command_line *cmd)
{
    if (!cmd) {
        return SHELL_EINVAL;
    }
    
    // Simple implementation - just validate filenames for now
    // In the future, this would open files and redirect file descriptors
    
    if (cmd->input_redirect) {
        // Validate input file exists
        if (strlen(cmd->input_redirect) == 0) {
            shell_print_error("Invalid input redirection filename\n");
            return SHELL_EINVAL;
        }
    }
    
    if (cmd->output_redirect) {
        // Validate output filename
        if (strlen(cmd->output_redirect) == 0) {
            shell_print_error("Invalid output redirection filename\n");
            return SHELL_EINVAL;
        }
    }
    
    return SHELL_SUCCESS;
}
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
            // Check for append redirection (>>) first
            if (cmd->arguments[i][1] == '>') {
                // Append redirection
                if (strlen(cmd->arguments[i]) > 2) {
                    // Format: >>filename
                    cmd->output_redirect = cmd->arguments[i] + 2;
                } else if (i + 1 < cmd->argument_count) {
                    // Format: >> filename
                    cmd->output_redirect = cmd->arguments[i + 1];
                    // Remove redirection from arguments
                    for (int j = i; j < cmd->argument_count - 2; j++) {
                        cmd->arguments[j] = cmd->arguments[j + 2];
                    }
                    cmd->argument_count -= 2;
                }
                cmd->output_append = 1;
            } else {
                // Regular output redirection
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
                cmd->output_append = 0;
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
        } else if (cmd->arguments[i][0] == '|') {
            // Pipe segments handled during execution phase
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

    // Detect basic pipes (cmd1 | cmd2) and execute via temporary files
    int pipe_index = -1;
    for (int i = 1; i < cmd->argument_count; i++) {
        if (cmd->arguments[i] && cmd->arguments[i][0] == '|' && cmd->arguments[i][1] == '\0') {
            pipe_index = i;
            break;
        }
    }

    if (pipe_index != -1) {
        // Ensure both sides of the pipe have commands
        if (pipe_index == 0 || pipe_index >= cmd->argument_count - 1) {
            shell_print_error("Invalid pipe syntax\n");
            return SHELL_EINVAL;
        }

        if (!cmd->arguments[pipe_index + 1] || cmd->arguments[pipe_index + 1][0] == '\0') {
            shell_print_error("Missing command after pipe\n");
            return SHELL_EINVAL;
        }

        // Create a simple temp file path using context pointer as unique ID
        char *temp_file = (char *)kmalloc(32);
        if (!temp_file) {
            shell_print_error("Failed to allocate pipe temp file\n");
            return SHELL_ENOMEM;
        }

        // Use pointer address as unique ID (simple but effective)
        unsigned long id = ((unsigned long)temp_file >> 4) & 0xFFFFF;

        // Build path manually to avoid any complex functions
        const char *prefix = "/tmp/pipe_";
        int pos = 0;
        while (prefix[pos]) {
            temp_file[pos] = prefix[pos];
            pos++;
        }

        // Convert id to hex string (simpler than decimal)
        for (int i = 4; i >= 0; i--) {
            int digit = (id >> (i * 4)) & 0xF;
            if (digit < 10) {
                temp_file[pos++] = '0' + digit;
            } else {
                temp_file[pos++] = 'a' + (digit - 10);
            }
        }
        temp_file[pos] = '\0';

        // Deep copy arguments for left command to avoid shared pointer issues
        char **left_args = (char **)kmalloc((pipe_index + 1) * sizeof(char *));
        if (!left_args) {
            kfree(temp_file);
            return SHELL_ENOMEM;
        }

        // Copy argument pointers for left command
        for (int i = 0; i < pipe_index; i++) {
            left_args[i] = cmd->arguments[i];
        }
        left_args[pipe_index] = NULL;  // Null terminate

        // Deep copy arguments for right command
        char **right_args = (char **)kmalloc((cmd->argument_count - pipe_index) * sizeof(char *));
        if (!right_args) {
            kfree(left_args);
            kfree(temp_file);
            return SHELL_ENOMEM;
        }

        // Copy argument pointers for right command
        for (int i = 0; i < cmd->argument_count - pipe_index - 1; i++) {
            right_args[i] = cmd->arguments[pipe_index + 1 + i];
        }
        right_args[cmd->argument_count - pipe_index - 1] = NULL;  // Null terminate

        // Prepare left command (before pipe) to write into temp file
        struct command_line left_cmd;
        memset(&left_cmd, 0, sizeof(left_cmd));
        left_cmd.command = cmd->command;
        left_cmd.arguments = left_args;
        left_cmd.argument_count = pipe_index;
        left_cmd.output_redirect = temp_file;
        left_cmd.output_append = 0;
        left_cmd.input_redirect = cmd->input_redirect;  // Pass through input redirect if any
        left_cmd.pipe_next = NULL;
        left_cmd.background = 0;

        // Prepare right command (after pipe) to read from temp file
        struct command_line right_cmd;
        memset(&right_cmd, 0, sizeof(right_cmd));
        right_cmd.command = cmd->arguments[pipe_index + 1];
        right_cmd.arguments = right_args;
        right_cmd.argument_count = cmd->argument_count - pipe_index - 1;
        right_cmd.input_redirect = temp_file;
        right_cmd.output_redirect = cmd->output_redirect;  // Pass through output redirect if any
        right_cmd.output_append = cmd->output_append;
        right_cmd.background = cmd->background;
        right_cmd.pipe_next = NULL;

        // Execute left command first
        int result1 = execute_command(ctx, &left_cmd);

        // Execute right command only if left succeeded
        int result2 = SHELL_ERROR;
        if (result1 == SHELL_SUCCESS) {
            result2 = execute_command(ctx, &right_cmd);
        }

        // Clean up allocated memory
        kfree(left_args);
        kfree(right_args);

        // Try to remove temp file (ignore errors if it doesn't exist)
        vfs_unlink(temp_file);
        kfree(temp_file);

        // Return the result of the pipeline (right command's result)
        return (result1 == SHELL_SUCCESS) ? result2 : result1;
    }


    // Store output redirection in context so commands can access it
    // Save the original values
    char *saved_output_redirect = ctx->output_redirect_file;
    int saved_append_mode = ctx->output_append_mode;
    char *saved_input_redirect = ctx->input_redirect_file;
    ctx->output_redirect_file = cmd->output_redirect;
    ctx->output_append_mode = cmd->output_append;
    ctx->input_redirect_file = cmd->input_redirect;
    
    // Set up I/O redirection if specified
    if (setup_io_redirection(cmd) < 0) {
        shell_print_error("Failed to set up I/O redirection\n");
        ctx->output_redirect_file = saved_output_redirect;
        ctx->output_append_mode = saved_append_mode;
        ctx->input_redirect_file = saved_input_redirect;
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
            ctx->output_append_mode = saved_append_mode;
            ctx->input_redirect_file = saved_input_redirect;
            return SHELL_ENOENT;
        }
    }
    
    // Restore context
    ctx->output_redirect_file = saved_output_redirect;
    ctx->output_append_mode = saved_append_mode;
    ctx->input_redirect_file = saved_input_redirect;
    
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
    cmd->output_append = 0;
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
    
    // Validate argument count, considering input/output redirection
    int effective_min_args = shell_cmd->min_args;
    
    // Special case: if input redirection is active, some commands don't need additional arguments
    if (ctx->input_redirect_file && (
        strcmp(cmd->command, "cat") == 0 ||
        strcmp(cmd->command, "wc") == 0 ||
        strcmp(cmd->command, "head") == 0 ||
        strcmp(cmd->command, "tail") == 0
    )) {
        effective_min_args = 0;  // These commands can work without filename when input is redirected
    }
    
    if (effective_min_args >= 0 && cmd->argument_count - 1 < effective_min_args) {
        shell_printf("Error: %s requires at least %d arguments\n", 
                    cmd->command, effective_min_args);
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
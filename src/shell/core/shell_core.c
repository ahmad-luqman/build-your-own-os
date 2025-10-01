/*
 * MiniOS Shell Core Implementation
 * Main shell initialization, execution loop, and management
 */

#include "shell.h"
#include "kernel.h"
#include "process.h"
#include "syscall.h"
#include "uart.h"

// Global shell command registry
static struct shell_command shell_commands[] = {
    // Directory commands
    {"cd", "Change directory", cmd_cd, 0, 1},
    {"pwd", "Print working directory", cmd_pwd, 0, 0},
    
    // File listing and content
    {"ls", "List directory contents", cmd_ls, 0, 2},
    {"cat", "Display file contents", cmd_cat, 1, 1},
    
    // File management
    {"mkdir", "Create directory", cmd_mkdir, 1, 1},
    {"rmdir", "Remove directory", cmd_rmdir, 1, 1},
    {"rm", "Remove file", cmd_rm, 1, 2},
    {"cp", "Copy file", cmd_cp, 2, 2},
    {"mv", "Move/rename file", cmd_mv, 2, 2},
    
    // Output commands
    {"echo", "Display text", cmd_echo, 0, -1},  // -1 means unlimited args
    {"clear", "Clear screen", cmd_clear, 0, 0},
    
    // System information
    {"ps", "List processes", cmd_ps, 0, 1},
    {"free", "Show memory usage", cmd_free, 0, 0},
    {"uname", "Show system information", cmd_uname, 0, 1},
    {"date", "Show current date/time", cmd_date, 0, 0},
    {"uptime", "Show system uptime", cmd_uptime, 0, 0},
    
    // Shell commands
    {"help", "Show available commands", cmd_help, 0, 1},
    {"exit", "Exit shell", cmd_exit, 0, 1},
    
    // Sentinel
    {NULL, NULL, NULL, 0, 0}
};

// Initialize shell context
int shell_init(struct shell_context *ctx)
{
    early_print("shell_init: Entry\n");
    
    if (!ctx) {
        early_print("shell_init: NULL context\n");
        return SHELL_EINVAL;
    }
    
    early_print("shell_init: Context OK\n");
    
    // Initialize only essential fields, avoid touching large arrays
    ctx->argc = 0;
    early_print("shell_init: argc set\n");
    
    ctx->exit_requested = 0;
    early_print("shell_init: exit_requested set\n");
    
    ctx->history_count = 0;
    early_print("shell_init: history_count set\n");
    
    ctx->history_index = 0;
    early_print("shell_init: history_index set\n");
    
    ctx->fd_table = NULL;
    early_print("shell_init: fd_table set\n");
    
    ctx->stdin_fd = 0;
    early_print("shell_init: stdin_fd set\n");
    
    ctx->stdout_fd = 1;
    early_print("shell_init: stdout_fd set\n");
    
    ctx->stderr_fd = 2;
    early_print("shell_init: stderr_fd set\n");
    
    // Set directory - try to avoid writing to array directly
    ctx->current_directory[0] = '/';
    early_print("shell_init: directory[0] set\n");
    
    ctx->current_directory[1] = '\0';
    early_print("shell_init: directory[1] set\n");
    
    // Set command buffer
    ctx->command_buffer[0] = '\0';
    early_print("shell_init: command_buffer set\n");
    
    // Initialize Phase 7 fields to NULL/0
    ctx->input_buffer = NULL;
    ctx->buffer_size = 0;
    ctx->cursor_pos = 0;
    ctx->buffer_length = 0;
    ctx->history = NULL;
    ctx->completion = NULL;
    ctx->environment = NULL;
    ctx->env_count = 0;
    early_print("shell_init: Phase 7 fields set\n");
    
    early_print("shell_init: Success\n");
    return SHELL_SUCCESS;
}

// Main shell execution loop
void shell_run(struct shell_context *ctx)
{
    if (!ctx) {
        early_print("Invalid shell context\n");
        return;
    }
    
    shell_print("MiniOS Shell v1.0\n");
    shell_print("Type 'help' for available commands\n\n");
    
    while (!ctx->exit_requested) {
        // Print prompt
        shell_print_prompt(ctx);
        
        // Read command
        if (shell_read_command(ctx) < 0) {
            continue;
        }
        
        // Skip empty commands
        if (strlen(ctx->command_buffer) == 0) {
            continue;
        }
        
        // Add to history
        if (ctx->history_count < SHELL_HISTORY_SIZE) {
            strcpy(ctx->command_history[ctx->history_count], ctx->command_buffer);
            ctx->history_count++;
        } else {
            // Shift history and add new command
            for (int i = 0; i < SHELL_HISTORY_SIZE - 1; i++) {
                strcpy(ctx->command_history[i], ctx->command_history[i + 1]);
            }
            strcpy(ctx->command_history[SHELL_HISTORY_SIZE - 1], ctx->command_buffer);
        }
        
        // Parse and execute command
        struct command_line cmd;
        if (parse_command_line(ctx->command_buffer, &cmd) == 0) {
            execute_command(ctx, &cmd);
            free_command_line(&cmd);
        } else {
            early_print("Failed to parse command\n");
        }
        
        shell_print("\n");
    }
    
    shell_print("Shell exiting...\n");
}

// Clean up shell resources
void shell_cleanup(struct shell_context *ctx)
{
    if (!ctx) {
        return;
    }
    
    // Clear sensitive data
    memset(ctx->command_buffer, 0, sizeof(ctx->command_buffer));
    memset(ctx->command_history, 0, sizeof(ctx->command_history));
    
    // No dynamic memory to free in current implementation
}

// Parse command into arguments
int shell_parse_command(const char *input, char *argv[], int max_argc)
{
    if (!input || !argv || max_argc <= 0) {
        return 0;
    }
    
    int argc = 0;
    static char buffer[SHELL_MAX_COMMAND_LENGTH];
    char *buf_ptr = buffer;
    
    // Copy input to buffer for tokenization
    strncpy(buffer, input, SHELL_MAX_COMMAND_LENGTH - 1);
    buffer[SHELL_MAX_COMMAND_LENGTH - 1] = '\0';
    
    // Skip leading whitespace
    while (*buf_ptr == ' ' || *buf_ptr == '\t') {
        buf_ptr++;
    }
    
    while (*buf_ptr != '\0' && argc < max_argc - 1) {
        // Start of argument
        argv[argc] = buf_ptr;
        argc++;
        
        // Find end of argument
        while (*buf_ptr != '\0' && *buf_ptr != ' ' && *buf_ptr != '\t') {
            buf_ptr++;
        }
        
        // Null-terminate argument
        if (*buf_ptr != '\0') {
            *buf_ptr = '\0';
            buf_ptr++;
            
            // Skip whitespace
            while (*buf_ptr == ' ' || *buf_ptr == '\t') {
                buf_ptr++;
            }
        }
    }
    
    // Null-terminate argument array
    argv[argc] = NULL;
    
    return argc;
}

// Find command by name
struct shell_command *shell_find_command(const char *name)
{
    if (!name) {
        return NULL;
    }
    
    for (int i = 0; shell_commands[i].name != NULL; i++) {
        if (strcmp(shell_commands[i].name, name) == 0) {
            return &shell_commands[i];
        }
    }
    
    return NULL;
}

// Shell initialization for system
int shell_init_system(void)
{
    // Initialize shell subsystem
    early_print("Shell system initialized\n");
    return SHELL_SUCCESS;
}

// Global shell context pointer (avoid large static structure)
// We'll allocate this dynamically to avoid BSS issues
static struct shell_context *global_shell_context_ptr = NULL;

// Main shell task entry point
void shell_main_task(void *arg)
{
    (void)arg;  // Suppress unused parameter warning
    
    early_print("shell_main_task: Entry\n");
    
    // Allocate shell context from heap to avoid BSS/stack issues
    early_print("Allocating shell context...\n");
    struct shell_context *ctx = (struct shell_context *)kmalloc(sizeof(struct shell_context));
    
    if (!ctx) {
        early_print("ERROR: Failed to allocate shell context\n");
        return;
    }
    
    early_print("Shell context allocated successfully\n");
    global_shell_context_ptr = ctx;
    
    early_print("Calling shell_init\n");
    
    // Initialize shell context - shell_init will set up the fields
    if (shell_init(ctx) != SHELL_SUCCESS) {
        early_print("Failed to initialize shell context\n");
        kfree(ctx);
        return;
    }
    
    early_print("shell_init complete, starting shell\n");
    
    // Run shell
    shell_run(ctx);
    
    // Cleanup
    shell_cleanup(ctx);
    kfree(ctx);
    global_shell_context_ptr = NULL;
    
    early_print("Shell exited\n");
}
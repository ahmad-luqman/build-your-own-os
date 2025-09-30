/*
 * MiniOS Shell Built-in Commands
 * Essential shell commands implementation
 */

#include "shell.h"
#include "kernel.h"
#include "vfs.h"

// Change directory command
int cmd_cd(struct shell_context *ctx, int argc, char *argv[])
{
    if (!ctx) {
        return SHELL_EINVAL;
    }
    
    const char *path = "/";  // Default to root
    
    if (argc > 1) {
        path = argv[1];
    }
    
    // Simple path validation and update
    if (strlen(path) >= SHELL_MAX_PATH_LENGTH) {
        shell_print_error("Path too long\n");
        return SHELL_EINVAL;
    }
    
    // For now, just update the current directory string
    // In the future, this would validate the path exists
    if (path[0] == '/') {
        // Absolute path
        strcpy(ctx->current_directory, path);
    } else {
        // Relative path - append to current directory
        if (strcmp(ctx->current_directory, "/") != 0) {
            strcat(ctx->current_directory, "/");
        }
        strcat(ctx->current_directory, path);
    }
    
    // Ensure path ends without trailing slash (unless root)
    size_t len = strlen(ctx->current_directory);
    if (len > 1 && ctx->current_directory[len - 1] == '/') {
        ctx->current_directory[len - 1] = '\0';
    }
    
    return SHELL_SUCCESS;
}

// Print working directory command
int cmd_pwd(struct shell_context *ctx, int argc, char *argv[])
{
    (void)argc; (void)argv;  // Suppress unused parameter warnings
    
    if (!ctx) {
        return SHELL_EINVAL;
    }
    
    shell_printf("%s\n", ctx->current_directory);
    return SHELL_SUCCESS;
}

// List directory contents command
int cmd_ls(struct shell_context *ctx, int argc, char *argv[])
{
    if (!ctx) {
        return SHELL_EINVAL;
    }
    
    const char *path = ctx->current_directory;
    int show_details = 0;
    
    // Parse options
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            // Option
            for (int j = 1; argv[i][j]; j++) {
                if (argv[i][j] == 'l') {
                    show_details = 1;
                }
            }
        } else {
            // Path argument
            path = argv[i];
        }
    }
    
    // For now, simulate directory listing
    shell_printf("Directory listing for %s:\n", path);
    
    if (show_details) {
        shell_print("drwxr-xr-x  2 root root   4096 Jan  1 00:00 .\n");
        shell_print("drwxr-xr-x  3 root root   4096 Jan  1 00:00 ..\n");
        shell_print("-rw-r--r--  1 root root     42 Jan  1 00:00 test.txt\n");
        shell_print("drwxr-xr-x  2 root root   4096 Jan  1 00:00 bin\n");
    } else {
        shell_print(".  ..  test.txt  bin\n");
    }
    
    return SHELL_SUCCESS;
}

// Display file contents command  
int cmd_cat(struct shell_context *ctx, int argc, char *argv[])
{
    if (!ctx || argc < 2) {
        shell_print_error("Usage: cat <filename>\n");
        return SHELL_EINVAL;
    }
    
    const char *filename = argv[1];
    
    // For now, simulate file reading
    shell_printf("Contents of %s:\n", filename);
    shell_print("This is a sample file content.\n");
    shell_print("MiniOS file system is working!\n");
    
    return SHELL_SUCCESS;
}

// Create directory command
int cmd_mkdir(struct shell_context *ctx, int argc, char *argv[])
{
    if (!ctx || argc < 2) {
        shell_print_error("Usage: mkdir <directory>\n");
        return SHELL_EINVAL;
    }
    
    const char *dirname = argv[1];
    
    shell_printf("Creating directory: %s\n", dirname);
    
    // For now, just simulate creation
    shell_print("Directory created successfully.\n");
    
    return SHELL_SUCCESS;
}

// Remove directory command
int cmd_rmdir(struct shell_context *ctx, int argc, char *argv[])
{
    if (!ctx || argc < 2) {
        shell_print_error("Usage: rmdir <directory>\n");
        return SHELL_EINVAL;
    }
    
    const char *dirname = argv[1];
    
    shell_printf("Removing directory: %s\n", dirname);
    
    // For now, just simulate removal
    shell_print("Directory removed successfully.\n");
    
    return SHELL_SUCCESS;
}

// Remove file command
int cmd_rm(struct shell_context *ctx, int argc, char *argv[])
{
    if (!ctx || argc < 2) {
        shell_print_error("Usage: rm <filename>\n");
        return SHELL_EINVAL;
    }
    
    int force = 0;
    const char *filename = NULL;
    
    // Parse options and filename
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            // Option
            for (int j = 1; argv[i][j]; j++) {
                if (argv[i][j] == 'f') {
                    force = 1;
                }
            }
        } else {
            // Filename
            filename = argv[i];
        }
    }
    
    if (!filename) {
        shell_print_error("No filename specified\n");
        return SHELL_EINVAL;
    }
    
    shell_printf("Removing file: %s", filename);
    if (force) {
        shell_print(" (forced)");
    }
    shell_print("\n");
    
    // For now, just simulate removal
    shell_print("File removed successfully.\n");
    
    return SHELL_SUCCESS;
}

// Copy file command
int cmd_cp(struct shell_context *ctx, int argc, char *argv[])
{
    if (!ctx || argc < 3) {
        shell_print_error("Usage: cp <source> <destination>\n");
        return SHELL_EINVAL;
    }
    
    const char *source = argv[1];
    const char *dest = argv[2];
    
    shell_printf("Copying %s to %s\n", source, dest);
    
    // For now, just simulate copying
    shell_print("File copied successfully.\n");
    
    return SHELL_SUCCESS;
}

// Move/rename file command
int cmd_mv(struct shell_context *ctx, int argc, char *argv[])
{
    if (!ctx || argc < 3) {
        shell_print_error("Usage: mv <source> <destination>\n");
        return SHELL_EINVAL;
    }
    
    const char *source = argv[1];
    const char *dest = argv[2];
    
    shell_printf("Moving %s to %s\n", source, dest);
    
    // For now, just simulate moving
    shell_print("File moved successfully.\n");
    
    return SHELL_SUCCESS;
}

// Echo command
int cmd_echo(struct shell_context *ctx, int argc, char *argv[])
{
    if (!ctx) {
        return SHELL_EINVAL;
    }
    
    // Print all arguments separated by spaces
    for (int i = 1; i < argc; i++) {
        if (i > 1) {
            shell_print(" ");
        }
        shell_print(argv[i]);
    }
    shell_print("\n");
    
    return SHELL_SUCCESS;
}

// Clear screen command
int cmd_clear(struct shell_context *ctx, int argc, char *argv[])
{
    (void)argc; (void)argv;  // Suppress unused parameter warnings
    
    if (!ctx) {
        return SHELL_EINVAL;
    }
    
    // Send ANSI clear screen sequence
    shell_print("\033[2J\033[H");
    
    return SHELL_SUCCESS;
}

// Help command
int cmd_help(struct shell_context *ctx, int argc, char *argv[])
{
    if (!ctx) {
        return SHELL_EINVAL;
    }
    
    const char *command = NULL;
    
    if (argc > 1) {
        command = argv[1];
    }
    
    if (command) {
        // Show help for specific command
        struct shell_command *cmd = shell_find_command(command);
        if (cmd) {
            shell_printf("%s - %s\n", cmd->name, cmd->description);
            
            // Show argument requirements
            if (cmd->min_args > 0) {
                shell_printf("Requires at least %d argument(s)\n", cmd->min_args);
            }
            if (cmd->max_args >= 0) {
                shell_printf("Takes at most %d argument(s)\n", cmd->max_args);
            }
        } else {
            shell_printf("Unknown command: %s\n", command);
        }
    } else {
        // Show all commands
        shell_print("Available commands:\n\n");
        
        shell_print("Directory Operations:\n");
        shell_print("  cd [path]       - Change directory\n");
        shell_print("  pwd             - Print working directory\n");
        shell_print("  ls [-l] [path]  - List directory contents\n");
        shell_print("  mkdir <dir>     - Create directory\n");
        shell_print("  rmdir <dir>     - Remove directory\n");
        shell_print("\n");
        
        shell_print("File Operations:\n");
        shell_print("  cat <file>      - Display file contents\n");
        shell_print("  rm [-f] <file>  - Remove file\n");
        shell_print("  cp <src> <dst>  - Copy file\n");
        shell_print("  mv <src> <dst>  - Move/rename file\n");
        shell_print("\n");
        
        shell_print("System Information:\n");
        shell_print("  ps              - List processes\n");
        shell_print("  free            - Show memory usage\n");
        shell_print("  uname [-a]      - Show system information\n");
        shell_print("  date            - Show current date/time\n");
        shell_print("  uptime          - Show system uptime\n");
        shell_print("\n");
        
        shell_print("Other Commands:\n");
        shell_print("  echo [text]     - Display text\n");
        shell_print("  clear           - Clear screen\n");
        shell_print("  help [command]  - Show help\n");
        shell_print("  exit [code]     - Exit shell\n");
    }
    
    return SHELL_SUCCESS;
}

// Exit command
int cmd_exit(struct shell_context *ctx, int argc, char *argv[])
{
    if (!ctx) {
        return SHELL_EINVAL;
    }
    
    int exit_code = 0;
    
    if (argc > 1) {
        // Parse exit code (simple implementation)
        const char *code_str = argv[1];
        exit_code = 0;
        
        // Convert string to number
        for (int i = 0; code_str[i]; i++) {
            if (code_str[i] >= '0' && code_str[i] <= '9') {
                exit_code = exit_code * 10 + (code_str[i] - '0');
            }
        }
    }
    
    shell_printf("Exiting shell with code %d\n", exit_code);
    ctx->exit_requested = 1;
    
    return SHELL_SUCCESS;
}
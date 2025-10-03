/*
 * MiniOS Shell Built-in Commands
 * Essential shell commands implementation
 */

#include "shell.h"
#include "kernel.h"
#include "vfs.h"

// Helper function to build full path from current directory and relative path
static void build_full_path(char *dest, size_t dest_size, const char *current_dir, const char *path)
{
    if (path[0] == '/') {
        // Absolute path
        strncpy(dest, path, dest_size - 1);
        dest[dest_size - 1] = '\0';
    } else {
        // Relative path
        if (strcmp(current_dir, "/") == 0) {
            dest[0] = '/';
            strncpy(dest + 1, path, dest_size - 2);
            dest[dest_size - 1] = '\0';
        } else {
            // Copy current directory
            size_t len = strlen(current_dir);
            if (len < dest_size - 1) {
                strcpy(dest, current_dir);
                if (len + 1 < dest_size - 1) {
                    dest[len] = '/';
                    strncpy(dest + len + 1, path, dest_size - len - 2);
                    dest[dest_size - 1] = '\0';
                }
            }
        }
    }
}

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
    
    // Simple path validation
    if (strlen(path) >= SHELL_MAX_PATH_LENGTH) {
        shell_print_error("Path too long\n");
        return SHELL_EINVAL;
    }
    
    // Build full path
    char full_path[SHELL_MAX_PATH_LENGTH];
    build_full_path(full_path, sizeof(full_path), ctx->current_directory, path);
    
    // Validate that directory exists using VFS
    struct inode stat_buf;
    if (vfs_stat(full_path, &stat_buf) != VFS_SUCCESS) {
        shell_print_error("Directory not found: ");
        shell_print_error(full_path);
        shell_print_error("\n");
        return SHELL_ENOENT;
    }
    
    // Check if it's actually a directory
    if ((stat_buf.mode & VFS_FILE_DIRECTORY) == 0) {
        shell_print_error("Not a directory: ");
        shell_print_error(full_path);
        shell_print_error("\n");
        return SHELL_EINVAL;
    }
    
    // Update current directory
    strcpy(ctx->current_directory, full_path);
    
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
    
    // Build full path if relative
    char full_path[SHELL_MAX_PATH_LENGTH];
    build_full_path(full_path, sizeof(full_path), ctx->current_directory, path);
    
    // Try to open directory using VFS
    int fd = vfs_open(full_path, VFS_O_RDONLY, 0);
    if (fd < 0) {
        shell_print_error("Cannot open directory: ");
        shell_print_error(full_path);
        shell_print_error("\n");
        return SHELL_ENOENT;
    }
    
    // Read directory entries
    struct dirent entries[16];  // Read up to 16 entries at a time
    int count = vfs_readdir(fd, entries, 16);
    
    if (count < 0) {
        shell_print_error("Failed to read directory\n");
        vfs_close(fd);
        return SHELL_ERROR;
    }
    
    if (count == 0) {
        shell_print("(empty directory)\n");
    } else {
        if (show_details) {
            // Show detailed listing
            for (int i = 0; i < count; i++) {
                struct dirent *ent = &entries[i];
                
                // Display file type
                char type = '-';
                if (ent->type & VFS_FILE_DIRECTORY) {
                    type = 'd';
                } else if (ent->type & VFS_FILE_SYMLINK) {
                    type = 'l';
                }
                
                shell_printf("%crw-r--r--  1 root root      0 Jan  1 00:00 %s\n", 
                           type, ent->name);
            }
        } else {
            // Simple listing
            for (int i = 0; i < count; i++) {
                shell_print(entries[i].name);
                shell_print("  ");
            }
            shell_print("\n");
        }
    }
    
    vfs_close(fd);
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
    
    // Build full path if relative
    char full_path[SHELL_MAX_PATH_LENGTH];
    build_full_path(full_path, sizeof(full_path), ctx->current_directory, filename);
    
    // Open file using VFS
    int fd = vfs_open(full_path, VFS_O_RDONLY, 0);
    if (fd < 0) {
        shell_print_error("Cannot open file: ");
        shell_print_error(full_path);
        shell_print_error("\n");
        return SHELL_ENOENT;
    }
    
    // Read and display file contents
    char buffer[256];
    ssize_t bytes_read;
    
    while ((bytes_read = vfs_read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';  // Null-terminate for printing
        shell_print(buffer);
    }
    
    if (bytes_read < 0) {
        shell_print_error("\nError reading file\n");
        vfs_close(fd);
        return SHELL_ERROR;
    }
    
    vfs_close(fd);
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
    
    // Build full path if relative
    char full_path[SHELL_MAX_PATH_LENGTH];
    build_full_path(full_path, sizeof(full_path), ctx->current_directory, dirname);
    
    // Create directory using VFS
    int result = vfs_mkdir(full_path, 0755);
    if (result != VFS_SUCCESS) {
        shell_print_error("Failed to create directory: ");
        shell_print_error(full_path);
        shell_print_error("\n");
        return SHELL_ERROR;
    }
    
    shell_printf("Directory created: %s\n", full_path);
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
    
    // Build full path if relative
    char full_path[SHELL_MAX_PATH_LENGTH];
    build_full_path(full_path, sizeof(full_path), ctx->current_directory, dirname);
    
    // Remove directory using VFS
    int result = vfs_rmdir(full_path);
    if (result != VFS_SUCCESS) {
        shell_print_error("Failed to remove directory: ");
        shell_print_error(full_path);
        shell_print_error("\n");
        return SHELL_ERROR;
    }
    
    shell_printf("Directory removed: %s\n", full_path);
    return SHELL_SUCCESS;
}

// Remove file command
int cmd_rm(struct shell_context *ctx, int argc, char *argv[])
{
    if (!ctx || argc < 2) {
        shell_print_error("Usage: rm [-f] <filename>\n");
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
    
    // Build full path if relative
    char full_path[SHELL_MAX_PATH_LENGTH];
    build_full_path(full_path, sizeof(full_path), ctx->current_directory, filename);
    
    // Remove file using VFS
    int result = vfs_unlink(full_path);
    if (result != VFS_SUCCESS) {
        if (!force) {
            shell_print_error("Failed to remove file: ");
            shell_print_error(full_path);
            shell_print_error("\n");
            return SHELL_ERROR;
        }
        // With -f flag, ignore errors
    }
    
    shell_printf("File removed: %s\n", full_path);
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
    
    // Build full paths if relative
    char src_path[SHELL_MAX_PATH_LENGTH];
    char dst_path[SHELL_MAX_PATH_LENGTH];
    
    build_full_path(src_path, sizeof(src_path), ctx->current_directory, source);
    build_full_path(dst_path, sizeof(dst_path), ctx->current_directory, dest);
    
    // Open source file
    int src_fd = vfs_open(src_path, VFS_O_RDONLY, 0);
    if (src_fd < 0) {
        shell_print_error("Cannot open source file: ");
        shell_print_error(src_path);
        shell_print_error("\n");
        return SHELL_ENOENT;
    }
    
    // Open/create destination file
    int dst_fd = vfs_open(dst_path, VFS_O_WRONLY | VFS_O_CREAT | VFS_O_TRUNC, 0644);
    if (dst_fd < 0) {
        shell_print_error("Cannot create destination file: ");
        shell_print_error(dst_path);
        shell_print_error("\n");
        vfs_close(src_fd);
        return SHELL_ERROR;
    }
    
    // Copy file contents
    char buffer[512];
    ssize_t bytes_read;
    ssize_t bytes_written;

    while ((bytes_read = vfs_read(src_fd, buffer, sizeof(buffer))) > 0) {
        bytes_written = vfs_write(dst_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            shell_print_error("Write error during copy\n");
            vfs_close(src_fd);
            vfs_close(dst_fd);
            return SHELL_ERROR;
        }
    }

    if (bytes_read < 0) {
        shell_print_error("Read error during copy\n");
        vfs_close(src_fd);
        vfs_close(dst_fd);
        return SHELL_ERROR;
    }

    vfs_close(src_fd);
    vfs_close(dst_fd);
    
    shell_printf("Copied %s to %s\n", src_path, dst_path);
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
    
    // Build full paths if relative
    char src_path[SHELL_MAX_PATH_LENGTH];
    char dst_path[SHELL_MAX_PATH_LENGTH];
    
    build_full_path(src_path, sizeof(src_path), ctx->current_directory, source);
    build_full_path(dst_path, sizeof(dst_path), ctx->current_directory, dest);
    
    // Use VFS rename operation
    int result = vfs_rename(src_path, dst_path);
    if (result != VFS_SUCCESS) {
        shell_print_error("Failed to move/rename file: ");
        shell_print_error(src_path);
        shell_print_error("\n");
        return SHELL_ERROR;
    }
    
    shell_printf("Moved %s to %s\n", src_path, dst_path);
    return SHELL_SUCCESS;
}

// Echo command with optional output redirection
int cmd_echo(struct shell_context *ctx, int argc, char *argv[])
{
    if (!ctx) {
        return SHELL_EINVAL;
    }
    
    // Check if output redirection is active (set by parser)
    if (ctx->output_redirect_file) {
        // Output redirection: echo text > file
        const char *filename = ctx->output_redirect_file;
        
        // Build full path
        char full_path[SHELL_MAX_PATH_LENGTH];
        build_full_path(full_path, sizeof(full_path), ctx->current_directory, filename);
        
        // Open/create file for writing
        int fd = vfs_open(full_path, VFS_O_WRONLY | VFS_O_CREAT | VFS_O_TRUNC, 0644);
        if (fd < 0) {
            shell_print_error("Cannot create file: ");
            shell_print_error(full_path);
            shell_print_error("\n");
            return SHELL_ERROR;
        }
        
        // Build output string from all arguments
        char buffer[1024];
        size_t pos = 0;
        for (int i = 1; i < argc; i++) {
            if (i > 1 && pos < sizeof(buffer) - 1) {
                buffer[pos++] = ' ';
            }
            const char *arg = argv[i];
            while (*arg && pos < sizeof(buffer) - 1) {
                buffer[pos++] = *arg++;
            }
        }
        buffer[pos++] = '\n';
        buffer[pos] = '\0';
        
        // Write to file
        ssize_t written = vfs_write(fd, buffer, pos);
        vfs_close(fd);
        
        if (written < 0) {
            shell_print_error("Error writing to file\n");
            return SHELL_ERROR;
        }
        
        // Success - no output to console
        return SHELL_SUCCESS;
    } else {
        // Normal echo - print to console
        for (int i = 1; i < argc; i++) {
            if (i > 1) {
                shell_print(" ");
            }
            shell_print(argv[i]);
        }
        shell_print("\n");
        
        return SHELL_SUCCESS;
    }
}

// Touch command - create an empty file or update timestamp
int cmd_touch(struct shell_context *ctx, int argc, char *argv[])
{
    if (!ctx) {
        return SHELL_EINVAL;
    }
    
    if (argc < 2) {
        shell_print_error("Usage: touch <file>\n");
        return SHELL_EINVAL;
    }
    
    const char *path = argv[1];
    
    // Build full path
    char full_path[SHELL_MAX_PATH_LENGTH];
    build_full_path(full_path, sizeof(full_path), ctx->current_directory, path);
    
    // Try to open existing file
    int fd = vfs_open(full_path, VFS_O_RDONLY, 0);
    if (fd >= 0) {
        // File exists - just update access time by opening and closing
        vfs_close(fd);
        return SHELL_SUCCESS;
    }
    
    // File doesn't exist - create it
    fd = vfs_open(full_path, VFS_O_WRONLY | VFS_O_CREAT, 0644);
    if (fd < 0) {
        shell_print_error("Cannot create file: ");
        shell_print_error(full_path);
        shell_print_error("\n");
        return SHELL_ERROR;
    }
    
    vfs_close(fd);
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
        shell_print("  touch <file>    - Create file or update timestamp\n");
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
        shell_print("  echo text > file - Write text to file\n");
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

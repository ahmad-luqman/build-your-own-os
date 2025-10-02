#ifndef SHELL_H
#define SHELL_H

// Use kernel.h which properly handles cross-compilation
#include "kernel.h"
#include "vfs.h"
#include "fd.h"

#ifdef __cplusplus
extern "C" {
#endif

// Shell configuration
#define SHELL_MAX_COMMAND_LENGTH    1024
#define SHELL_MAX_ARGS              32
#define SHELL_MAX_PATH_LENGTH       VFS_MAX_PATH
#define SHELL_PROMPT                "MiniOS> "
#define SHELL_HISTORY_SIZE          2   // Reduced from 16

// Forward declarations for Phase 7 advanced features
struct history_context;
struct completion_context;

// Shell context structure
struct shell_context {
    char current_directory[SHELL_MAX_PATH_LENGTH];
    char command_buffer[SHELL_MAX_COMMAND_LENGTH];
    char *argv[SHELL_MAX_ARGS];
    int argc;
    int exit_requested;
    struct fd_table *fd_table;
    
    // Phase 7: Enhanced input handling
    char *input_buffer;             // Dynamic input buffer
    int buffer_size;                // Buffer size
    int cursor_pos;                 // Current cursor position
    int buffer_length;              // Current content length
    
    // Phase 7: Advanced features
    struct history_context *history;     // Command history
    struct completion_context *completion; // Tab completion
    
    // Command history (legacy - kept for compatibility)
    char command_history[SHELL_HISTORY_SIZE][SHELL_MAX_COMMAND_LENGTH];
    int history_count;
    int history_index;
    
    // I/O state
    int stdin_fd;
    int stdout_fd;
    int stderr_fd;
    
    // Phase 7: Environment variables
    char **environment;             // Environment variables
    int env_count;                  // Number of environment variables
};

// Shell command structure
struct shell_command {
    const char *name;
    const char *description;
    int (*handler)(struct shell_context *ctx, int argc, char *argv[]);
    int min_args;
    int max_args;
};

// Command line structure for parser
struct command_line {
    char *command;
    char **arguments;
    int argument_count;
    char *input_redirect;
    char *output_redirect;
    int background;
};

// Shell core functions
int shell_init(struct shell_context *ctx);
void shell_run(struct shell_context *ctx);
void shell_cleanup(struct shell_context *ctx);
int shell_parse_command(const char *input, char *argv[], int max_argc);
struct shell_command *shell_find_command(const char *name);

// Shell I/O functions
void shell_print_prompt(struct shell_context *ctx);
int shell_read_command(struct shell_context *ctx);
void shell_print(const char *message);
void shell_print_error(const char *error);
void shell_printf(const char *format, ...);

// Command parser functions
int parse_command_line(const char *input, struct command_line *cmd);
int execute_command(struct shell_context *ctx, struct command_line *cmd);
void free_command_line(struct command_line *cmd);

// Command execution functions
int execute_builtin_command(struct shell_context *ctx, struct command_line *cmd);
int execute_external_program(struct shell_context *ctx, struct command_line *cmd);
int setup_io_redirection(struct command_line *cmd);

// Built-in command handlers
int cmd_cd(struct shell_context *ctx, int argc, char *argv[]);
int cmd_pwd(struct shell_context *ctx, int argc, char *argv[]);
int cmd_ls(struct shell_context *ctx, int argc, char *argv[]);
int cmd_cat(struct shell_context *ctx, int argc, char *argv[]);
int cmd_mkdir(struct shell_context *ctx, int argc, char *argv[]);
int cmd_rmdir(struct shell_context *ctx, int argc, char *argv[]);
int cmd_rm(struct shell_context *ctx, int argc, char *argv[]);
int cmd_cp(struct shell_context *ctx, int argc, char *argv[]);
int cmd_mv(struct shell_context *ctx, int argc, char *argv[]);
int cmd_touch(struct shell_context *ctx, int argc, char *argv[]);
int cmd_echo(struct shell_context *ctx, int argc, char *argv[]);
int cmd_clear(struct shell_context *ctx, int argc, char *argv[]);
int cmd_help(struct shell_context *ctx, int argc, char *argv[]);
int cmd_exit(struct shell_context *ctx, int argc, char *argv[]);

// System information command handlers
int cmd_ps(struct shell_context *ctx, int argc, char *argv[]);
int cmd_free(struct shell_context *ctx, int argc, char *argv[]);
int cmd_uname(struct shell_context *ctx, int argc, char *argv[]);
int cmd_date(struct shell_context *ctx, int argc, char *argv[]);
int cmd_uptime(struct shell_context *ctx, int argc, char *argv[]);

// Shell system functions
int shell_init_system(void);
void shell_main_task(void *arg);
void register_shell_syscalls(void);

// Shell error codes
#define SHELL_SUCCESS           0
#define SHELL_ERROR            -1
#define SHELL_EINVAL           -2
#define SHELL_ENOENT           -3
#define SHELL_EPERM            -4
#define SHELL_ENOMEM           -5

// Phase 7: Advanced shell functions
int shell_init_advanced_features(struct shell_context *ctx);
void shell_cleanup_advanced_features(struct shell_context *ctx);
int shell_handle_special_keys(struct shell_context *ctx, char ch);
void shell_clear_input_line(struct shell_context *ctx);

// Phase 7: Environment variable functions
int shell_set_env(struct shell_context *ctx, const char *name, const char *value);
const char *shell_get_env(struct shell_context *ctx, const char *name);
int shell_unset_env(struct shell_context *ctx, const char *name);
void shell_display_env(struct shell_context *ctx);

// Phase 7: User program execution
int shell_exec_user_program(struct shell_context *ctx, const char *path, int argc, char *argv[]);
int shell_run_command(struct shell_context *ctx, const char *command);

// Phase 7: I/O redirection support
int shell_parse_with_redirection(struct shell_context *ctx, const char *command_line);

#ifdef __cplusplus
}
#endif

#endif /* SHELL_H */
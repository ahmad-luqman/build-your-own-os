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
#define SHELL_HISTORY_SIZE          16

// Shell context structure
struct shell_context {
    char current_directory[SHELL_MAX_PATH_LENGTH];
    char command_buffer[SHELL_MAX_COMMAND_LENGTH];
    char *argv[SHELL_MAX_ARGS];
    int argc;
    int exit_requested;
    struct fd_table *fd_table;
    
    // Command history
    char command_history[SHELL_HISTORY_SIZE][SHELL_MAX_COMMAND_LENGTH];
    int history_count;
    int history_index;
    
    // I/O state
    int stdin_fd;
    int stdout_fd;
    int stderr_fd;
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

#ifdef __cplusplus
}
#endif

#endif /* SHELL_H */
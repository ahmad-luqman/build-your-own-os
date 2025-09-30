#ifndef SHELL_COMPLETION_H
#define SHELL_COMPLETION_H

#include "shell.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Tab completion context
struct completion_context {
    char *partial_text;             // Partial text being completed
    char **completions;             // Array of completion suggestions
    int completion_count;           // Number of completions found
    int current_completion;         // Currently selected completion
    int completion_start;           // Start position in line
    int completion_end;             // End position in line
    
    // Completion type
    enum {
        COMPLETION_COMMAND,         // Command name completion
        COMPLETION_FILENAME,        // Filename completion
        COMPLETION_DIRECTORY,       // Directory completion
        COMPLETION_VARIABLE,        // Environment variable completion
        COMPLETION_NONE             // No completion available
    } type;
};

// Tab completion functions
int shell_handle_tab_completion(struct shell_context *ctx);
int completion_init(struct completion_context *comp);
void completion_cleanup(struct completion_context *comp);

// Completion generators
char **complete_command_name(const char *partial, int *count);
char **complete_filename(const char *partial, const char *directory, int *count);
char **complete_directory(const char *partial, int *count);
char **complete_builtin_command(const char *partial, int *count);

// Completion utilities
int find_completion_context(const char *line, int cursor_pos, struct completion_context *comp);
char *get_common_prefix(char **completions, int count);
void display_completions(struct shell_context *ctx, struct completion_context *comp);
int apply_completion(struct shell_context *ctx, struct completion_context *comp, int selection);

// Helper functions
int is_directory(const char *path);
int is_executable(const char *path);
char *extract_directory_path(const char *path);
char *extract_filename(const char *path);

// Built-in command list for completion
extern const char *builtin_commands[];
extern const int builtin_command_count;

#ifdef __cplusplus
}
#endif

#endif /* SHELL_COMPLETION_H */
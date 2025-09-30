#include "completion.h"
#include "vfs.h"
#include "memory.h"
#include "uart.h"

// Forward declarations for string functions
extern void *memset(void *s, int c, size_t n);
extern void *memcpy(void *dest, const void *src, size_t n);
extern char *strcpy(char *dest, const char *src);
extern char *strncpy(char *dest, const char *src, size_t n);
extern char *strcat(char *dest, const char *src);
extern int strcmp(const char *s1, const char *s2);
extern int strncmp(const char *s1, const char *s2, size_t n);
extern char *strrchr(const char *s, int c);
extern char *strstr(const char *haystack, const char *needle);
extern int snprintf(char *str, size_t size, const char *format, ...);

// Built-in commands for completion
const char *builtin_commands[] = {
    "help", "exit", "clear", "echo", "pwd", "cd", "ls", "mkdir", "rmdir",
    "touch", "rm", "cp", "mv", "cat", "ps", "meminfo", "version", "date",
    "exec", "run", "kill", "history"
};

const int builtin_command_count = sizeof(builtin_commands) / sizeof(builtin_commands[0]);

// Initialize completion context
int completion_init(struct completion_context *comp) {
    if (!comp) {
        return -1;
    }
    
    memset(comp, 0, sizeof(struct completion_context));
    comp->type = COMPLETION_NONE;
    comp->current_completion = -1;
    
    return 0;
}

// Clean up completion context
void completion_cleanup(struct completion_context *comp) {
    if (!comp) {
        return;
    }
    
    if (comp->completions) {
        for (int i = 0; i < comp->completion_count; i++) {
            if (comp->completions[i]) {
                kfree(comp->completions[i]);
            }
        }
        kfree(comp->completions);
    }
    
    if (comp->partial_text) {
        kfree(comp->partial_text);
    }
    
    memset(comp, 0, sizeof(struct completion_context));
}

// Handle tab completion in shell (simplified)
int shell_handle_tab_completion(struct shell_context *ctx) {
    if (!ctx || !ctx->input_buffer) {
        return -1;
    }
    
    // For Phase 7, provide a simplified tab completion
    // Just complete basic commands
    
    char *input = ctx->input_buffer;
    int len = strlen(input);
    
    // Find matching commands
    for (int i = 0; i < builtin_command_count; i++) {
        if (strncmp(builtin_commands[i], input, len) == 0) {
            // Found a match - complete it
            if (len < ctx->buffer_size - 1) {
                strcpy(ctx->input_buffer, builtin_commands[i]);
                ctx->cursor_pos = strlen(builtin_commands[i]);
                return 0;
            }
        }
    }
    
    return -1;
}

// Complete command names
char **complete_command_name(const char *partial, int *count) {
    if (!partial || !count) {
        return NULL;
    }
    
    *count = 0;
    int capacity = 16;
    char **results = kmalloc(sizeof(char *) * capacity);
    if (!results) {
        return NULL;
    }
    
    int partial_len = strlen(partial);
    
    // Check built-in commands
    for (int i = 0; i < builtin_command_count; i++) {
        if (strncmp(builtin_commands[i], partial, partial_len) == 0) {
            if (*count >= capacity) {
                capacity *= 2;
                char **new_results = kmalloc(sizeof(char *) * capacity);
                if (!new_results) {
                    break;
                }
                memcpy(new_results, results, sizeof(char *) * *count);
                kfree(results);
                results = new_results;
            }
            
            results[*count] = kmalloc(strlen(builtin_commands[i]) + 1);
            if (results[*count]) {
                strcpy(results[*count], builtin_commands[i]);
                (*count)++;
            }
        }
    }
    
    return results;
}

// Complete filenames (simplified)
char **complete_filename(const char *partial, const char *directory, int *count) {
    if (!partial || !directory || !count) {
        return NULL;
    }
    
    *count = 0;
    
    // For Phase 7, return empty completion list
    // In a full implementation, this would scan directories
    return NULL;
}

// Complete directory names (simplified)
char **complete_directory(const char *partial, int *count) {
    return complete_filename(partial, ".", count);
}

// Find completion context (simplified)
int find_completion_context(const char *line, int cursor_pos, struct completion_context *comp) {
    if (!line || !comp || cursor_pos < 0) {
        return -1;
    }
    
    // For Phase 7, assume we're always completing commands
    comp->type = COMPLETION_COMMAND;
    comp->completion_start = 0;
    comp->completion_end = strlen(line);
    
    comp->partial_text = kmalloc(strlen(line) + 1);
    if (comp->partial_text) {
        strcpy(comp->partial_text, line);
    }
    
    return 0;
}

// Get common prefix (simplified)
char *get_common_prefix(char **completions, int count) {
    if (!completions || count == 0) {
        return NULL;
    }
    
    if (count == 1) {
        char *prefix = kmalloc(strlen(completions[0]) + 1);
        if (prefix) {
            strcpy(prefix, completions[0]);
        }
        return prefix;
    }
    
    // Find length of common prefix
    int prefix_len = strlen(completions[0]);
    for (int i = 1; i < count; i++) {
        int j = 0;
        while (j < prefix_len && j < (int)strlen(completions[i]) && 
               completions[0][j] == completions[i][j]) {
            j++;
        }
        prefix_len = j;
    }
    
    if (prefix_len == 0) {
        return NULL;
    }
    
    char *prefix = kmalloc(prefix_len + 1);
    if (prefix) {
        strncpy(prefix, completions[0], prefix_len);
        prefix[prefix_len] = '\0';
    }
    
    return prefix;
}

// Display completions (simplified)
void display_completions(struct shell_context *ctx, struct completion_context *comp) {
    if (!ctx || !comp || !comp->completions) {
        return;
    }
    
    // For Phase 7, just print the first few completions
    for (int i = 0; i < comp->completion_count && i < 4; i++) {
        // Would output completions here
    }
}

// Apply completion (simplified)
int apply_completion(struct shell_context *ctx, struct completion_context *comp, int selection) {
    if (!ctx || !comp || selection < 0 || selection >= comp->completion_count) {
        return -1;
    }
    
    const char *completion = comp->completions[selection];
    if (!completion) {
        return -1;
    }
    
    // Replace input buffer with completion
    int new_len = strlen(completion);
    if (new_len < ctx->buffer_size) {
        strcpy(ctx->input_buffer, completion);
        ctx->cursor_pos = new_len;
        return 0;
    }
    
    return -1;
}

// Utility functions (simplified)
int is_directory(const char *path) {
    if (!path) {
        return 0;
    }
    
    int len = strlen(path);
    return (len > 0 && path[len - 1] == '/');
}

int is_executable(const char *path) {
    if (!path || is_directory(path)) {
        return 0;
    }
    
    int fd = vfs_open(path, 0, 0);
    if (fd >= 0) {
        vfs_close(fd);
        return 1;
    }
    
    return 0;
}

char *extract_directory_path(const char *path) {
    if (!path) {
        return NULL;
    }
    
    char *last_slash = strrchr(path, '/');
    if (!last_slash) {
        return NULL;
    }
    
    int len = last_slash - path;
    if (len == 0) {
        char *root = kmalloc(2);
        if (root) {
            strcpy(root, "/");
        }
        return root;
    }
    
    char *dir = kmalloc(len + 1);
    if (dir) {
        strncpy(dir, path, len);
        dir[len] = '\0';
    }
    
    return dir;
}

char *extract_filename(const char *path) {
    if (!path) {
        return NULL;
    }
    
    char *last_slash = strrchr(path, '/');
    const char *filename = last_slash ? last_slash + 1 : path;
    
    char *result = kmalloc(strlen(filename) + 1);
    if (result) {
        strcpy(result, filename);
    }
    
    return result;
}
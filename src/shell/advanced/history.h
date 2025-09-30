#ifndef SHELL_HISTORY_H
#define SHELL_HISTORY_H

#include "shell.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// History configuration
#define HISTORY_MAX_ENTRIES     100
#define HISTORY_MAX_LINE_LENGTH 512

// History entry structure
struct history_entry {
    char *command;                  // Command text
    int timestamp;                  // When command was executed
    struct history_entry *next;    // Next entry in list
    struct history_entry *prev;    // Previous entry in list
};

// History context
struct history_context {
    struct history_entry *head;    // First (oldest) entry
    struct history_entry *tail;    // Last (newest) entry
    struct history_entry *current; // Current position for navigation
    int count;                      // Number of entries
    int max_entries;               // Maximum entries to keep
    
    // Navigation state
    int navigating;                 // Currently navigating history
    char *saved_line;              // Saved current input during navigation
    int saved_cursor_pos;          // Saved cursor position
};

// History management functions
int history_init(struct history_context *hist);
void history_cleanup(struct history_context *hist);
int history_add_command(struct history_context *hist, const char *command);
void history_clear(struct history_context *hist);

// History navigation
const char *history_get_previous(struct history_context *hist);
const char *history_get_next(struct history_context *hist);
int history_start_navigation(struct history_context *hist, const char *current_line, int cursor_pos);
void history_end_navigation(struct history_context *hist);

// History search
const char *history_search_backward(struct history_context *hist, const char *pattern);
const char *history_search_forward(struct history_context *hist, const char *pattern);
int history_find_matching_entry(struct history_context *hist, const char *pattern, int direction);

// Shell integration
int shell_handle_arrow_up(struct shell_context *ctx);
int shell_handle_arrow_down(struct shell_context *ctx);
int shell_handle_ctrl_r(struct shell_context *ctx);  // Reverse search

// History display
void history_display_all(struct history_context *hist);
void history_display_recent(struct history_context *hist, int count);
const char *history_get_entry(struct history_context *hist, int index);

// Persistence (for future implementation)
int history_save_to_file(struct history_context *hist, const char *filename);
int history_load_from_file(struct history_context *hist, const char *filename);

// Utilities
int history_should_add_command(const char *command);
char *history_expand_command(const char *command);  // For !n, !!, !pattern expansion
int history_get_entry_index(struct history_context *hist, struct history_entry *entry);

#ifdef __cplusplus
}
#endif

#endif /* SHELL_HISTORY_H */
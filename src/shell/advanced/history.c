#include "history.h"
#include "memory.h"
#include "uart.h"
#include "timer.h"

// Forward declarations for string functions
extern void *memset(void *s, int c, size_t n);
extern char *strcpy(char *dest, const char *src);
extern char *strncpy(char *dest, const char *src, size_t n);
extern int strcmp(const char *s1, const char *s2);
extern char *strstr(const char *haystack, const char *needle);
extern int snprintf(char *str, size_t size, const char *format, ...);

// Initialize history context
int history_init(struct history_context *hist) {
    if (!hist) {
        return -1;
    }
    
    memset(hist, 0, sizeof(struct history_context));
    hist->max_entries = HISTORY_MAX_ENTRIES;
    
    return 0;
}

// Clean up history context
void history_cleanup(struct history_context *hist) {
    if (!hist) {
        return;
    }
    
    // Free all history entries
    struct history_entry *entry = hist->head;
    while (entry) {
        struct history_entry *next = entry->next;
        if (entry->command) {
            kfree(entry->command);
        }
        kfree(entry);
        entry = next;
    }
    
    if (hist->saved_line) {
        kfree(hist->saved_line);
    }
    
    memset(hist, 0, sizeof(struct history_context));
}

// Add command to history
int history_add_command(struct history_context *hist, const char *command) {
    if (!hist || !command || !history_should_add_command(command)) {
        return -1;
    }
    
    // Don't add duplicate of last command
    if (hist->tail && strcmp(hist->tail->command, command) == 0) {
        return 0;
    }
    
    // Create new entry
    struct history_entry *entry = kmalloc(sizeof(struct history_entry));
    if (!entry) {
        return -1;
    }
    
    entry->command = kmalloc(strlen(command) + 1);
    if (!entry->command) {
        kfree(entry);
        return -1;
    }
    
    strcpy(entry->command, command);
    entry->timestamp = timer_get_ticks();
    entry->next = NULL;
    entry->prev = hist->tail;
    
    // Add to list
    if (hist->tail) {
        hist->tail->next = entry;
    } else {
        hist->head = entry;
    }
    hist->tail = entry;
    hist->count++;
    
    // Remove oldest entries if we exceed max
    while (hist->count > hist->max_entries) {
        struct history_entry *old_head = hist->head;
        hist->head = old_head->next;
        if (hist->head) {
            hist->head->prev = NULL;
        } else {
            hist->tail = NULL;
        }
        
        if (old_head->command) {
            kfree(old_head->command);
        }
        kfree(old_head);
        hist->count--;
    }
    
    return 0;
}

// Start history navigation
int history_start_navigation(struct history_context *hist, const char *current_line, int cursor_pos) {
    if (!hist) {
        return -1;
    }
    
    if (!hist->navigating) {
        // Save current input
        if (current_line) {
            hist->saved_line = kmalloc(strlen(current_line) + 1);
            if (hist->saved_line) {
                strcpy(hist->saved_line, current_line);
                hist->saved_cursor_pos = cursor_pos;
            }
        }
        hist->navigating = 1;
        hist->current = hist->tail;
    }
    
    return 0;
}

// End history navigation
void history_end_navigation(struct history_context *hist) {
    if (!hist || !hist->navigating) {
        return;
    }
    
    hist->navigating = 0;
    hist->current = NULL;
    
    if (hist->saved_line) {
        kfree(hist->saved_line);
        hist->saved_line = NULL;
    }
}

// Get previous command in history
const char *history_get_previous(struct history_context *hist) {
    if (!hist || !hist->navigating) {
        return NULL;
    }
    
    if (!hist->current) {
        // At beginning, return saved line
        return hist->saved_line;
    }
    
    const char *command = hist->current->command;
    hist->current = hist->current->prev;
    
    return command;
}

// Get next command in history
const char *history_get_next(struct history_context *hist) {
    if (!hist || !hist->navigating) {
        return NULL;
    }
    
    if (!hist->current) {
        // Already at newest, stay at saved line
        return hist->saved_line;
    }
    
    hist->current = hist->current->next;
    
    if (hist->current) {
        return hist->current->command;
    } else {
        // Reached end, return to saved line
        return hist->saved_line;
    }
}

// Handle up arrow key (simplified)
int shell_handle_arrow_up(struct shell_context *ctx) {
    if (!ctx || !ctx->history) {
        return -1;
    }
    
    // Start navigation if not already navigating
    if (!ctx->history->navigating) {
        history_start_navigation(ctx->history, ctx->input_buffer, ctx->cursor_pos);
    }
    
    const char *prev_command = history_get_previous(ctx->history);
    if (prev_command) {
        // Copy previous command to input buffer
        strncpy(ctx->input_buffer, prev_command, ctx->buffer_size - 1);
        ctx->input_buffer[ctx->buffer_size - 1] = '\0';
        ctx->cursor_pos = strlen(ctx->input_buffer);
        return 0;
    }
    
    return -1;
}

// Handle down arrow key (simplified)
int shell_handle_arrow_down(struct shell_context *ctx) {
    if (!ctx || !ctx->history || !ctx->history->navigating) {
        return -1;
    }
    
    const char *next_command = history_get_next(ctx->history);
    if (next_command) {
        // Copy next command to input buffer
        strncpy(ctx->input_buffer, next_command, ctx->buffer_size - 1);
        ctx->input_buffer[ctx->buffer_size - 1] = '\0';
        ctx->cursor_pos = strlen(ctx->input_buffer);
        return 0;
    }
    
    return -1;
}

// Handle Ctrl+R (simplified)
int shell_handle_ctrl_r(struct shell_context *ctx) {
    // For Phase 7, just return - not implemented
    (void)ctx;
    return -1;
}

// Display all history (simplified)
void history_display_all(struct history_context *hist) {
    if (!hist) {
        return;
    }
    
    struct history_entry *entry = hist->head;
    int index = 1;
    
    while (entry) {
        // Would display history here
        entry = entry->next;
        index++;
    }
}

// Display recent history entries
void history_display_recent(struct history_context *hist, int count) {
    if (!hist || count <= 0) {
        return;
    }
    
    // Find starting point
    struct history_entry *entry = hist->tail;
    int entries_to_skip = hist->count - count;
    
    for (int i = 0; i < entries_to_skip && entry; i++) {
        entry = entry->prev;
    }
    
    // Display entries (simplified)
    int index = hist->count - count + 1;
    while (entry) {
        entry = entry->next;
        index++;
    }
}

// Get history entry by index (1-based)
const char *history_get_entry(struct history_context *hist, int index) {
    if (!hist || index < 1 || index > hist->count) {
        return NULL;
    }
    
    struct history_entry *entry = hist->head;
    for (int i = 1; i < index && entry; i++) {
        entry = entry->next;
    }
    
    return entry ? entry->command : NULL;
}

// Check if command should be added to history
int history_should_add_command(const char *command) {
    if (!command || strlen(command) == 0) {
        return 0;
    }
    
    // Skip commands that start with space (common convention)
    if (command[0] == ' ') {
        return 0;
    }
    
    // Skip very short commands
    if (strlen(command) < 2) {
        return 0;
    }
    
    return 1;
}

// Clear all history
void history_clear(struct history_context *hist) {
    if (!hist) {
        return;
    }
    
    struct history_entry *entry = hist->head;
    while (entry) {
        struct history_entry *next = entry->next;
        if (entry->command) {
            kfree(entry->command);
        }
        kfree(entry);
        entry = next;
    }
    
    hist->head = NULL;
    hist->tail = NULL;
    hist->current = NULL;
    hist->count = 0;
    hist->navigating = 0;
    
    if (hist->saved_line) {
        kfree(hist->saved_line);
        hist->saved_line = NULL;
    }
}

// Search backward in history (simplified)
const char *history_search_backward(struct history_context *hist, const char *pattern) {
    if (!hist || !pattern) {
        return NULL;
    }
    
    struct history_entry *entry = hist->current ? hist->current->prev : hist->tail;
    
    while (entry) {
        if (strstr(entry->command, pattern)) {
            hist->current = entry;
            return entry->command;
        }
        entry = entry->prev;
    }
    
    return NULL;
}

// Search forward in history (simplified)
const char *history_search_forward(struct history_context *hist, const char *pattern) {
    if (!hist || !pattern) {
        return NULL;
    }
    
    struct history_entry *entry = hist->current ? hist->current->next : hist->head;
    
    while (entry) {
        if (strstr(entry->command, pattern)) {
            hist->current = entry;
            return entry->command;
        }
        entry = entry->next;
    }
    
    return NULL;
}
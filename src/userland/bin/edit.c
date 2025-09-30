/*
 * MiniOS Text Editor - Simple text editor demonstrating enhanced applications
 * Phase 8.1: Enhanced Applications with full C library support
 */

#include "../lib/minios_libc/stdio.h"
#include "../lib/minios_libc/stdlib.h"
#include "../lib/minios_libc/string.h"

#define MAX_LINES 1000
#define MAX_LINE_LENGTH 256
#define EDITOR_VERSION "1.0"

// Editor state
struct editor_state {
    char *lines[MAX_LINES];
    int line_count;
    int current_line;
    char filename[256];
    int modified;
    int insert_mode;
};

static struct editor_state editor;

// Function prototypes
void init_editor(void);
void cleanup_editor(void);
int load_file(const char *filename);
int save_file(const char *filename);
void display_editor(void);
void display_help(void);
void display_status(void);
void handle_command(char *command);
void insert_line(int line_num, const char *text);
void delete_line(int line_num);
void edit_line(int line_num, const char *new_text);
int find_text(const char *search_text);
void show_line_numbers(void);

int main(int argc, char *argv[]) {
    printf("MiniOS Text Editor v%s\n", EDITOR_VERSION);
    printf("========================\n");
    
    init_editor();
    
    // Load file if specified
    if (argc > 1) {
        if (load_file(argv[1])) {
            printf("Loaded file: %s (%d lines)\n", argv[1], editor.line_count);
            strncpy(editor.filename, argv[1], sizeof(editor.filename) - 1);
        } else {
            printf("Creating new file: %s\n", argv[1]);
            strncpy(editor.filename, argv[1], sizeof(editor.filename) - 1);
        }
    } else {
        printf("New document (untitled)\n");
        strcpy(editor.filename, "untitled.txt");
    }
    
    printf("Type 'help' for commands, 'quit' to exit\n\n");
    
    // Main editor loop
    char command[256];
    while (1) {
        display_status();
        printf("ed> ");
        
        if (!fgets(command, sizeof(command), stdin)) {
            break;
        }
        
        // Remove newline
        char *newline = strchr(command, '\n');
        if (newline) *newline = '\0';
        
        // Handle command
        handle_command(command);
    }
    
    cleanup_editor();
    return 0;
}

void init_editor(void) {
    memset(&editor, 0, sizeof(editor));
    editor.insert_mode = 1;  // Start in insert mode
    strcpy(editor.filename, "untitled.txt");
}

void cleanup_editor(void) {
    for (int i = 0; i < editor.line_count; i++) {
        if (editor.lines[i]) {
            free(editor.lines[i]);
        }
    }
}

int load_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        return 0;  // File doesn't exist or can't open
    }
    
    char buffer[MAX_LINE_LENGTH];
    editor.line_count = 0;
    
    while (fgets(buffer, sizeof(buffer), file) && editor.line_count < MAX_LINES) {
        // Remove newline if present
        char *newline = strchr(buffer, '\n');
        if (newline) *newline = '\0';
        
        // Allocate and copy line
        editor.lines[editor.line_count] = malloc(strlen(buffer) + 1);
        if (editor.lines[editor.line_count]) {
            strcpy(editor.lines[editor.line_count], buffer);
            editor.line_count++;
        }
    }
    
    fclose(file);
    editor.modified = 0;
    return 1;
}

int save_file(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        printf("Error: Cannot open file for writing: %s\n", filename);
        return 0;
    }
    
    for (int i = 0; i < editor.line_count; i++) {
        if (editor.lines[i]) {
            fprintf(file, "%s\n", editor.lines[i]);
        }
    }
    
    fclose(file);
    editor.modified = 0;
    printf("File saved: %s (%d lines)\n", filename, editor.line_count);
    return 1;
}

void display_editor(void) {
    printf("\n=== %s ===\n", editor.filename);
    if (editor.line_count == 0) {
        printf("(empty file)\n");
    } else {
        for (int i = 0; i < editor.line_count; i++) {
            char marker = (i == editor.current_line) ? '>' : ' ';
            printf("%c%3d: %s\n", marker, i + 1, 
                   editor.lines[i] ? editor.lines[i] : "");
        }
    }
    printf("\n");
}

void display_help(void) {
    printf("\nMiniOS Text Editor Commands:\n");
    printf("============================\n");
    printf("Navigation:\n");
    printf("  list, l         - List all lines with numbers\n");
    printf("  goto <n>        - Go to line number n\n");
    printf("  top             - Go to first line\n");
    printf("  bottom          - Go to last line\n");
    printf("\nEditing:\n");
    printf("  insert <text>   - Insert line at current position\n");
    printf("  append <text>   - Append line after current position\n");
    printf("  delete          - Delete current line\n");
    printf("  delete <n>      - Delete line number n\n");
    printf("  edit <text>     - Replace current line with text\n");
    printf("  edit <n> <text> - Replace line n with text\n");
    printf("\nFile Operations:\n");
    printf("  save            - Save file\n");
    printf("  saveas <name>   - Save file with new name\n");
    printf("  load <name>     - Load file (discards current)\n");
    printf("\nSearch:\n");
    printf("  find <text>     - Find text in file\n");
    printf("  count           - Show line count\n");
    printf("\nOther:\n");
    printf("  help            - Show this help\n");
    printf("  status          - Show editor status\n");
    printf("  quit            - Exit editor\n");
    printf("\n");
}

void display_status(void) {
    printf("[%s] Line %d/%d %s\n", 
           editor.filename,
           editor.current_line + 1, 
           editor.line_count,
           editor.modified ? "(modified)" : "");
}

void handle_command(char *command) {
    if (strlen(command) == 0) {
        return;
    }
    
    // Tokenize command
    char *cmd = strtok(command, " \t");
    if (!cmd) return;
    
    // Navigation commands
    if (strcmp(cmd, "list") == 0 || strcmp(cmd, "l") == 0) {
        display_editor();
    } else if (strcmp(cmd, "goto") == 0) {
        char *line_str = strtok(NULL, " \t");
        if (line_str) {
            int line = atoi(line_str) - 1;  // Convert to 0-based
            if (line >= 0 && line < editor.line_count) {
                editor.current_line = line;
                printf("Now at line %d: %s\n", line + 1, 
                       editor.lines[line] ? editor.lines[line] : "");
            } else {
                printf("Invalid line number. Range: 1-%d\n", editor.line_count);
            }
        }
    } else if (strcmp(cmd, "top") == 0) {
        editor.current_line = 0;
        printf("At top of file\n");
    } else if (strcmp(cmd, "bottom") == 0) {
        editor.current_line = editor.line_count > 0 ? editor.line_count - 1 : 0;
        printf("At bottom of file\n");
    
    // Editing commands
    } else if (strcmp(cmd, "insert") == 0) {
        char *text = strtok(NULL, "");  // Get rest of line
        insert_line(editor.current_line, text ? text : "");
        editor.modified = 1;
    } else if (strcmp(cmd, "append") == 0) {
        char *text = strtok(NULL, "");
        insert_line(editor.current_line + 1, text ? text : "");
        editor.current_line++;
        editor.modified = 1;
    } else if (strcmp(cmd, "delete") == 0) {
        char *line_str = strtok(NULL, " \t");
        if (line_str) {
            int line = atoi(line_str) - 1;
            delete_line(line);
        } else {
            delete_line(editor.current_line);
        }
        editor.modified = 1;
    } else if (strcmp(cmd, "edit") == 0) {
        char *arg1 = strtok(NULL, " \t");
        if (arg1 && isdigit(arg1[0])) {
            // edit <n> <text>
            int line = atoi(arg1) - 1;
            char *text = strtok(NULL, "");
            edit_line(line, text ? text : "");
        } else {
            // edit <text>
            char *text = strtok(NULL, "");
            edit_line(editor.current_line, text ? text : "");
        }
        editor.modified = 1;
    
    // File operations
    } else if (strcmp(cmd, "save") == 0) {
        save_file(editor.filename);
    } else if (strcmp(cmd, "saveas") == 0) {
        char *filename = strtok(NULL, " \t");
        if (filename) {
            if (save_file(filename)) {
                strncpy(editor.filename, filename, sizeof(editor.filename) - 1);
            }
        } else {
            printf("Usage: saveas <filename>\n");
        }
    } else if (strcmp(cmd, "load") == 0) {
        char *filename = strtok(NULL, " \t");
        if (filename) {
            if (editor.modified) {
                printf("Warning: Current file is modified. Save first? (y/n): ");
                char response = getchar();
                if (response == 'y' || response == 'Y') {
                    save_file(editor.filename);
                }
                // Clear input buffer
                while (getchar() != '\n');
            }
            
            cleanup_editor();
            init_editor();
            if (load_file(filename)) {
                strncpy(editor.filename, filename, sizeof(editor.filename) - 1);
                printf("Loaded: %s\n", filename);
            } else {
                printf("Could not load file: %s\n", filename);
            }
        } else {
            printf("Usage: load <filename>\n");
        }
    
    // Search commands
    } else if (strcmp(cmd, "find") == 0) {
        char *text = strtok(NULL, "");
        if (text) {
            int line = find_text(text);
            if (line >= 0) {
                editor.current_line = line;
                printf("Found at line %d: %s\n", line + 1, editor.lines[line]);
            } else {
                printf("Text not found: %s\n", text);
            }
        } else {
            printf("Usage: find <text>\n");
        }
    } else if (strcmp(cmd, "count") == 0) {
        printf("Line count: %d\n", editor.line_count);
    
    // Help and status
    } else if (strcmp(cmd, "help") == 0) {
        display_help();
    } else if (strcmp(cmd, "status") == 0) {
        display_status();
        
    // Exit
    } else if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) {
        if (editor.modified) {
            printf("File is modified. Save before quitting? (y/n): ");
            char response = getchar();
            if (response == 'y' || response == 'Y') {
                save_file(editor.filename);
            }
            // Clear input buffer
            while (getchar() != '\n');
        }
        printf("Goodbye!\n");
        exit(0);
    } else {
        printf("Unknown command: %s (type 'help' for commands)\n", cmd);
    }
}

void insert_line(int line_num, const char *text) {
    if (editor.line_count >= MAX_LINES) {
        printf("Error: Maximum lines reached\n");
        return;
    }
    
    // Shift lines down
    for (int i = editor.line_count; i > line_num; i--) {
        editor.lines[i] = editor.lines[i - 1];
    }
    
    // Insert new line
    editor.lines[line_num] = malloc(strlen(text) + 1);
    if (editor.lines[line_num]) {
        strcpy(editor.lines[line_num], text);
        editor.line_count++;
        printf("Inserted line %d\n", line_num + 1);
    } else {
        printf("Error: Out of memory\n");
    }
}

void delete_line(int line_num) {
    if (line_num < 0 || line_num >= editor.line_count) {
        printf("Invalid line number\n");
        return;
    }
    
    // Free the line
    if (editor.lines[line_num]) {
        free(editor.lines[line_num]);
    }
    
    // Shift lines up
    for (int i = line_num; i < editor.line_count - 1; i++) {
        editor.lines[i] = editor.lines[i + 1];
    }
    
    editor.line_count--;
    if (editor.current_line >= editor.line_count && editor.line_count > 0) {
        editor.current_line = editor.line_count - 1;
    }
    
    printf("Deleted line %d\n", line_num + 1);
}

void edit_line(int line_num, const char *new_text) {
    if (line_num < 0 || line_num >= editor.line_count) {
        printf("Invalid line number\n");
        return;
    }
    
    // Free old line
    if (editor.lines[line_num]) {
        free(editor.lines[line_num]);
    }
    
    // Set new line
    editor.lines[line_num] = malloc(strlen(new_text) + 1);
    if (editor.lines[line_num]) {
        strcpy(editor.lines[line_num], new_text);
        printf("Updated line %d\n", line_num + 1);
    } else {
        printf("Error: Out of memory\n");
        editor.lines[line_num] = NULL;
    }
}

int find_text(const char *search_text) {
    for (int i = 0; i < editor.line_count; i++) {
        if (editor.lines[i] && strstr(editor.lines[i], search_text)) {
            return i;
        }
    }
    return -1;
}

// Helper function to check if character is digit
int isdigit(int c) {
    return c >= '0' && c <= '9';
}
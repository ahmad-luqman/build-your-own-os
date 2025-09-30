/*
 * More - Paged file viewer for MiniOS
 * Displays files one page at a time with user controls
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// User I/O functions
int user_printf(const char *format, ...);
int user_getchar(void);
int user_puts(const char *str);
int user_open(const char *path, int flags);
int user_read(int fd, void *buf, size_t count);
int user_close(int fd);

#define LINES_PER_PAGE  20
#define BUFFER_SIZE     1024

struct more_context {
    int fd;
    char buffer[BUFFER_SIZE];
    int buffer_pos;
    int buffer_len;
    int line_count;
    int page_count;
    int total_lines;
    int quit_requested;
};

// Initialize more context
int more_init(struct more_context *ctx, const char *filename) {
    ctx->fd = user_open(filename, 0);
    if (ctx->fd < 0) {
        user_printf("more: cannot open '%s'\n", filename);
        return -1;
    }
    
    ctx->buffer_pos = 0;
    ctx->buffer_len = 0;
    ctx->line_count = 0;
    ctx->page_count = 0;
    ctx->total_lines = 0;
    ctx->quit_requested = 0;
    
    return 0;
}

// Get next character from file
int more_getchar(struct more_context *ctx) {
    if (ctx->buffer_pos >= ctx->buffer_len) {
        // Refill buffer
        ctx->buffer_len = user_read(ctx->fd, ctx->buffer, BUFFER_SIZE);
        if (ctx->buffer_len <= 0) {
            return EOF;
        }
        ctx->buffer_pos = 0;
    }
    
    return (unsigned char)ctx->buffer[ctx->buffer_pos++];
}

// Display one page of text
int more_display_page(struct more_context *ctx) {
    int lines_displayed = 0;
    int ch;
    
    while (lines_displayed < LINES_PER_PAGE && (ch = more_getchar(ctx)) != EOF) {
        user_printf("%c", ch);
        
        if (ch == '\n') {
            lines_displayed++;
            ctx->total_lines++;
        }
    }
    
    ctx->line_count += lines_displayed;
    ctx->page_count++;
    
    return (ch != EOF) ? 1 : 0;  // Return 1 if more content available
}

// Handle user input for paging
void more_handle_input(struct more_context *ctx, int more_content) {
    if (!more_content) {
        user_puts("\n(END)");
        return;
    }
    
    user_printf("--More--(%d%%)", (ctx->total_lines * 100) / (ctx->total_lines + 1));
    
    int ch = user_getchar();
    
    switch (ch) {
        case ' ':   // Next page
            user_puts("\r                                        \r");
            break;
            
        case '\n':  // Next line
        case '\r':
            user_puts("\r                                        \r");
            // Display one more line
            ch = more_getchar(ctx);
            if (ch != EOF) {
                user_printf("%c", ch);
                if (ch == '\n') {
                    ctx->total_lines++;
                }
            }
            more_handle_input(ctx, ch != EOF);
            return;
            
        case 'q':   // Quit
        case 'Q':
            ctx->quit_requested = 1;
            user_puts("\r                                        \r");
            break;
            
        case 'h':   // Help
        case 'H':
        case '?':
            user_puts("\r                                        \r");
            user_puts("More help:\n");
            user_puts("  SPACE    - Next page\n");
            user_puts("  ENTER    - Next line\n");
            user_puts("  q        - Quit\n");
            user_puts("  h        - This help\n");
            more_handle_input(ctx, more_content);
            return;
            
        default:
            user_puts("\r                                        \r");
            user_puts("Press SPACE for next page, q to quit, h for help\n");
            more_handle_input(ctx, more_content);
            return;
    }
}

// Display file with paging
int more_file(const char *filename) {
    struct more_context ctx;
    
    if (more_init(&ctx, filename) != 0) {
        return -1;
    }
    
    while (!ctx.quit_requested) {
        int more_content = more_display_page(&ctx);
        
        if (!more_content) {
            user_puts("(END)\n");
            break;
        }
        
        more_handle_input(&ctx, more_content);
        
        if (ctx.quit_requested) {
            break;
        }
    }
    
    user_close(ctx.fd);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        user_puts("Usage: more <file1> [file2] ...");
        return 1;
    }
    
    for (int i = 1; i < argc; i++) {
        if (argc > 2) {
            user_printf("\n==> %s <==\n", argv[i]);
        }
        
        if (more_file(argv[i]) != 0) {
            continue;
        }
        
        if (i < argc - 1) {
            user_puts("\nPress any key for next file...");
            user_getchar();
        }
    }
    
    return 0;
}

// Stub implementations
int user_printf(const char *format, ...) { return 0; }
int user_getchar(void) { return 0; }
int user_puts(const char *str) { return 0; }
int user_open(const char *path, int flags) { return -1; }
int user_read(int fd, void *buf, size_t count) { return -1; }
int user_close(int fd) { return -1; }
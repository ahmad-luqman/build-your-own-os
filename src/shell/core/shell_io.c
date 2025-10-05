/*
 * MiniOS Shell I/O Implementation
 * Input/output handling for shell interface
 */

#include "shell.h"
#include "kernel.h"
#include "shell_completion.h"
#include <stdarg.h>
#include <string.h>

// Architecture-specific UART register access for input
#ifdef __aarch64__
// ARM64 PL011 UART registers (same as used in arch_early_print)
#define UART_BASE   0x09000000
#define UART_DR     (UART_BASE + 0x000)
#define UART_FR     (UART_BASE + 0x018)
#define UART_FR_RXFE (1 << 4)  // RX FIFO empty
#define UART_FR_TXFF (1 << 5)  // TX FIFO full

static inline uint32_t mmio_read(uint64_t addr) {
    return *(volatile uint32_t*)addr;
}

static inline void mmio_write(uint64_t addr, uint32_t value) {
    *(volatile uint32_t*)addr = value;
}

// Simple character input function
static int shell_getc(void) {
    // Check if data is available (RX FIFO not empty)
    if (mmio_read(UART_FR) & UART_FR_RXFE) {
        return -1;  // No data available
    }
    
    // Read character from UART data register
    uint32_t data = mmio_read(UART_DR);
    
    // Add small debouncing delay to prevent multiple reads
    for (volatile int i = 0; i < 5000; i++);
    
    return (int)(data & 0xFF);
}

// Simple character output function
static void shell_putc(char c) {
    // Wait for UART to be ready
    while (mmio_read(UART_FR) & UART_FR_TXFF)
        ;
    
    // Write character to UART
    mmio_write(UART_DR, c);
}

#elif defined(__x86_64__)
// x86-64 16550 UART registers
#define SHELL_UART_BASE   0x3F8
#define SHELL_UART_THR    (SHELL_UART_BASE + 0)  // Transmit Holding Register
#define SHELL_UART_RBR    (SHELL_UART_BASE + 0)  // Receive Buffer Register
#define SHELL_UART_LSR    (SHELL_UART_BASE + 5)  // Line Status Register
#define SHELL_UART_LSR_DR (1 << 0)         // Data Ready
#define SHELL_UART_LSR_THRE (1 << 5)       // Transmit Holding Register Empty

static inline uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

// Simple character input function
static int shell_getc(void) {
    // Check if data is available
    if (!(inb(SHELL_UART_LSR) & SHELL_UART_LSR_DR)) {
        return -1;  // No data available
    }
    
    // Read character from UART
    return (int)inb(SHELL_UART_RBR);
}

// Simple character output function
static void shell_putc(char c) {
    // Wait for transmit register to be empty
    while (!(inb(SHELL_UART_LSR) & SHELL_UART_LSR_THRE))
        ;
    
    // Send character
    outb(SHELL_UART_THR, c);
}

#endif

static void shell_display_matches(struct shell_context *ctx,
                                  const char *matches[],
                                  size_t match_count)
{
    if (!ctx || !matches || match_count == 0) {
        return;
    }

    shell_print("\n");
    for (size_t i = 0; i < match_count; ++i) {
        shell_print(matches[i]);
        if (i + 1 < match_count) {
            shell_print("  ");
        }
    }
    shell_print("\n");
    shell_print_prompt(ctx);
    shell_print(ctx->command_buffer);
}

static void shell_handle_tab_completion(struct shell_context *ctx, int *cursor_pos)
{
    if (!ctx || !cursor_pos) {
        return;
    }

    int pos = *cursor_pos;
    if (pos < 0) {
        pos = 0;
    }

    int word_start = pos;
    while (word_start > 0) {
        char c = ctx->command_buffer[word_start - 1];
        if (c == ' ' || c == '\t') {
            break;
        }
        word_start--;
    }

    size_t prefix_len = (size_t)(pos - word_start);
    char prefix[SHELL_MAX_COMMAND_LENGTH];
    if (prefix_len > 0) {
        memcpy(prefix, &ctx->command_buffer[word_start], prefix_len);
    }
    prefix[prefix_len] = '\0';

    const char *commands[SHELL_MAX_BUILTINS];
    size_t command_count = shell_get_builtin_command_names(commands, SHELL_MAX_BUILTINS);
    if (command_count == 0) {
        shell_putc('\a');
        return;
    }

    const char *matches[SHELL_MAX_BUILTINS];
    size_t match_count = shell_completion_collect_matches(prefix, commands, command_count, matches, SHELL_MAX_BUILTINS);

    if (match_count == 0) {
        shell_putc('\a');
        return;
    }

    if (match_count == 1) {
        const char *completion = matches[0];
        size_t completion_len = strlen(completion);
        if ((size_t)word_start + completion_len >= SHELL_MAX_COMMAND_LENGTH) {
            shell_putc('\a');
            return;
        }

        memcpy(&ctx->command_buffer[word_start], completion, completion_len);
        pos = word_start + (int)completion_len;
        ctx->command_buffer[pos] = '\0';

        if (completion_len > prefix_len) {
            shell_print(completion + prefix_len);
        }

        if ((size_t)pos < SHELL_MAX_COMMAND_LENGTH - 1) {
            ctx->command_buffer[pos] = ' ';
            ctx->command_buffer[pos + 1] = '\0';
            pos++;
            shell_putc(' ');
        }

        *cursor_pos = pos;
        return;
    }

    char common_prefix[SHELL_MAX_COMMAND_LENGTH];
    size_t common_len = shell_completion_common_prefix(matches, match_count, common_prefix, sizeof(common_prefix));
    if (common_len > prefix_len) {
        size_t additional = common_len - prefix_len;
        size_t available = SHELL_MAX_COMMAND_LENGTH - 1 - (size_t)pos;
        if (additional > available) {
            additional = available;
        }

        size_t offset = prefix_len;
        for (size_t i = 0; i < additional; ++i) {
            char ch = common_prefix[offset + i];
            ctx->command_buffer[pos] = ch;
            pos++;
            ctx->command_buffer[pos] = '\0';
            shell_putc(ch);
        }
    }

    shell_display_matches(ctx, matches, match_count);
    *cursor_pos = pos;
}

// Print shell prompt
void shell_print_prompt(struct shell_context *ctx)
{
    if (!ctx) {
        return;
    }
    
    // Print current directory and prompt
    shell_print(ctx->current_directory);
    shell_print(SHELL_PROMPT);
}

// Read command from input
int shell_read_command(struct shell_context *ctx)
{
    if (!ctx) {
        return SHELL_EINVAL;
    }
    
    int pos = 0;
    
    // Clear command buffer
    ctx->command_buffer[0] = '\0';
    
    while (pos < SHELL_MAX_COMMAND_LENGTH - 1) {
        // Read one character from UART
        int ch = shell_getc();
        if (ch < 0) {
            // Add small delay when no input is available to prevent tight looping
            for (volatile int i = 0; i < 1000; i++);
            continue;  // No input available, keep trying
        }
        
        char c = (char)ch;
        
        // Handle special characters
        switch (c) {
            case '\r':  // Carriage return
            case '\n':  // Line feed
                // End of command
                ctx->command_buffer[pos] = '\0';
                shell_putc('\n');  // Echo newline
                return pos;
                
            case '\b':  // Backspace
            case 127:   // DEL
                if (pos > 0) {
                    pos--;
                    ctx->command_buffer[pos] = '\0';
                    // Echo backspace sequence
                    shell_putc('\b');
                    shell_putc(' ');
                    shell_putc('\b');
                }
                break;
                
            case 3:     // Ctrl+C
                // Cancel current command
                ctx->command_buffer[0] = '\0';
                shell_print("\n^C\n");
                return 0;

            case 4:     // Ctrl+D (EOF)
                if (pos == 0) {
                    // Exit shell on empty line
                    ctx->exit_requested = 1;
                    shell_print("\n");
                    return 0;
                }
                break;

            case '\t':  // Tab completion
                shell_handle_tab_completion(ctx, &pos);
                break;
                
            default:
                // Regular character
                if (c >= 32 && c < 127) {  // Printable ASCII
                    ctx->command_buffer[pos] = c;
                    pos++;
                    shell_putc(c);  // Echo character
                }
                break;
        }
    }
    
    // Command too long
    ctx->command_buffer[pos] = '\0';
    shell_print_error("\nCommand too long\n");
    return SHELL_ERROR;
}

// Print message to output
void shell_print(const char *message)
{
    if (!message) {
        return;
    }
    
    // Use early_print for output (same as kernel messages)
    early_print(message);
}

// Print error message
void shell_print_error(const char *error)
{
    if (!error) {
        return;
    }
    
    shell_print("Error: ");
    shell_print(error);
}

// Simple printf implementation for shell
void shell_printf(const char *format, ...)
{
    if (!format) {
        return;
    }

    char buffer[512];
    size_t pos = 0;

    va_list args;
    va_start(args, format);

    for (const char *p = format; *p && pos < sizeof(buffer) - 1; ++p) {
        if (*p != '%' || *(p + 1) == '\0') {
            buffer[pos++] = *p;
            continue;
        }

        ++p;  // Skip '%'

        switch (*p) {
            case 's': {
                const char *str = va_arg(args, const char *);
                if (!str) {
                    str = "(null)";
                }
                while (*str && pos < sizeof(buffer) - 1) {
                    buffer[pos++] = *str++;
                }
                break;
            }
            case 'd': {
                int value = va_arg(args, int);
                char temp[16];
                int idx = 0;
                if (value < 0) {
                    if (pos < sizeof(buffer) - 1) {
                        buffer[pos++] = '-';
                    }
                    value = -value;
                }
                if (value == 0) {
                    if (pos < sizeof(buffer) - 1) {
                        buffer[pos++] = '0';
                    }
                    break;
                }
                while (value > 0 && idx < (int)sizeof(temp)) {
                    temp[idx++] = (char)('0' + (value % 10));
                    value /= 10;
                }
                while (idx-- > 0 && pos < sizeof(buffer) - 1) {
                    buffer[pos++] = temp[idx];
                }
                break;
            }
            case 'x': {
                unsigned int value = va_arg(args, unsigned int);
                char temp[16];
                int idx = 0;
                if (value == 0) {
                    if (pos < sizeof(buffer) - 1) {
                        buffer[pos++] = '0';
                    }
                    break;
                }
                while (value > 0 && idx < (int)sizeof(temp)) {
                    unsigned int digit = value & 0xF;
                    temp[idx++] = (char)((digit < 10) ? '0' + digit : 'a' + (digit - 10));
                    value >>= 4;
                }
                while (idx-- > 0 && pos < sizeof(buffer) - 1) {
                    buffer[pos++] = temp[idx];
                }
                break;
            }
            case '%':
                if (pos < sizeof(buffer) - 1) {
                    buffer[pos++] = '%';
                }
                break;
            default:
                if (pos < sizeof(buffer) - 1) {
                    buffer[pos++] = '%';
                }
                if (pos < sizeof(buffer) - 1) {
                    buffer[pos++] = *p;
                }
                break;
        }
    }

    va_end(args);

    buffer[pos] = '\0';
    shell_print(buffer);
}

/*
 * MiniOS Shell I/O Implementation
 * Input/output handling for shell interface
 */

#include "shell.h"
#include "kernel.h"
#include "shell_completion.h"
#include "vfs.h"
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

        // Replace the word in the buffer
        memcpy(&ctx->command_buffer[word_start], completion, completion_len);
        int new_pos = word_start + (int)completion_len;
        ctx->command_buffer[new_pos] = '\0';

        // Only print the characters we're adding (after the prefix)
        if (completion_len > prefix_len) {
            shell_print(completion + prefix_len);
        }

        pos = new_pos;

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
    shell_print("\n");
    shell_error("Command too long");
    return SHELL_ERROR;
}

// Write straight to the console. Errors always take this path, so they stay
// on screen even while a command's normal output is redirected (stderr vs
// stdout).
static void shell_console_write(const char *message)
{
    early_print(message);
}

/*
 * Output redirection ("cmd > file", "cmd >> file", and the left side of a
 * pipe). While a redirect is active, shell_print() output is collected in a
 * small chunk buffer and written to the target file when the buffer fills
 * and when the command finishes. Errors (shell_error) bypass this and still
 * go to the console.
 *
 * Why a chunk buffer instead of writing on every shell_print(): kfree() is a
 * no-op (bump allocator) and RAMFS reallocates the whole file on every
 * growing write, so per-call writes would leak O(n^2) heap. The buffer itself
 * is allocated once and reused for every redirect.
 *
 * An explicit target ("> file", ">> file") is created/truncated BEFORE the
 * command runs, as a Unix shell does: a bad path fails without running the
 * command. It is then closed and reopened (append, no create) on the first
 * flush, so a command that removes its own target ("rm f > f") leaves it
 * removed; the output is discarded, matching the Unix outcome of writing to
 * an unlinked file. No fd is held open across the command's own changes.
 *
 * A pipe temp file is created lazily on the first flush instead, so a
 * command whose output fits in one chunk (e.g. "ls /tmp | cat") does not see
 * its own pipe file.
 */
#define SHELL_REDIRECT_CHUNK 512

static struct {
    char *buf;              // SHELL_REDIRECT_CHUNK bytes, allocated on first use
    size_t len;             // bytes pending in buf
    const char *path;       // absolute target path (owned by the caller)
    int fd;                 // -1 until the first flush opens the target
    int open_flags;         // flags for that open
    int created;            // target already created by shell_redirect_begin()
    int discard;            // target vanished during the command; drop output
    int active;
    int error;              // first VFS error, reported by shell_redirect_end()
} shell_redirect;

static void shell_redirect_flush(void)
{
    if (shell_redirect.error != VFS_SUCCESS || shell_redirect.discard) {
        shell_redirect.len = 0;
        return;
    }

    if (shell_redirect.fd < 0) {
        int fd = vfs_open(shell_redirect.path, shell_redirect.open_flags, 0644);
        if (fd < 0) {
            if (shell_redirect.created && fd == VFS_ENOENT) {
                // The command removed its own target ("rm f > f")
                shell_redirect.discard = 1;
            } else {
                shell_redirect.error = fd;
            }
            shell_redirect.len = 0;
            return;
        }
        shell_redirect.fd = fd;
    }

    if (shell_redirect.len > 0) {
        ssize_t written = vfs_write(shell_redirect.fd, shell_redirect.buf,
                                    shell_redirect.len);
        if (written != (ssize_t)shell_redirect.len) {
            shell_redirect.error = (written < 0) ? (int)written : VFS_ENOSPC;
        }
        shell_redirect.len = 0;
    }
}

// Start capturing shell_print() output for path (absolute). If create_now,
// create/truncate the target immediately (explicit "> file"); otherwise
// create it on the first flush (pipe temp file). Returns VFS_SUCCESS or a
// VFS_E* code, in which case the command must not run.
int shell_redirect_begin(const char *path, int append, int create_now)
{
    if (!path || shell_redirect.active) {
        return VFS_EINVAL;
    }

    if (!shell_redirect.buf) {
        shell_redirect.buf = (char *)kmalloc(SHELL_REDIRECT_CHUNK);
        if (!shell_redirect.buf) {
            return VFS_ENOMEM;
        }
    }

    int create_flags = VFS_O_WRONLY | VFS_O_CREAT | (append ? VFS_O_APPEND : VFS_O_TRUNC);
    if (create_now) {
        int fd = vfs_open(path, create_flags, 0644);
        if (fd < 0) {
            return fd;
        }
        vfs_close(fd);
        shell_redirect.open_flags = VFS_O_WRONLY | VFS_O_APPEND;  // never re-create
    } else {
        shell_redirect.open_flags = create_flags;
    }

    shell_redirect.len = 0;
    shell_redirect.path = path;
    shell_redirect.fd = -1;
    shell_redirect.created = create_now;
    shell_redirect.discard = 0;
    shell_redirect.error = VFS_SUCCESS;
    shell_redirect.active = 1;
    return VFS_SUCCESS;
}

// Stop capturing. If commit, write any pending output (creating a pipe
// target even when the command printed nothing, so the reader can open it);
// otherwise discard it. Returns VFS_SUCCESS or the first VFS error seen.
int shell_redirect_end(int commit)
{
    if (!shell_redirect.active) {
        return VFS_SUCCESS;
    }

    if (commit && (shell_redirect.len > 0 || !shell_redirect.created)) {
        shell_redirect_flush();
    }

    if (shell_redirect.fd >= 0) {
        vfs_close(shell_redirect.fd);
    }

    int error = commit ? shell_redirect.error : VFS_SUCCESS;
    shell_redirect.len = 0;
    shell_redirect.path = NULL;
    shell_redirect.fd = -1;
    shell_redirect.active = 0;
    return error;
}

// Print message to output (the redirect target if one is active)
void shell_print(const char *message)
{
    if (!message) {
        return;
    }

    if (!shell_redirect.active) {
        shell_console_write(message);
        return;
    }

    while (*message) {
        if (shell_redirect.len == SHELL_REDIRECT_CHUNK) {
            shell_redirect_flush();
        }
        shell_redirect.buf[shell_redirect.len++] = *message++;
    }
}

// Format into buf (always NUL-terminated). Supports %s, %d, %x and %%.
static void shell_vformat(char *buf, size_t size, const char *format, va_list ap)
{
    size_t pos = 0;

    if (!buf || size == 0) {
        return;
    }

    for (const char *p = format; *p && pos < size - 1; ++p) {
        if (*p != '%' || *(p + 1) == '\0') {
            buf[pos++] = *p;
            continue;
        }

        ++p;  // Skip '%'

        switch (*p) {
            case 's': {
                const char *str = va_arg(ap, const char *);
                if (!str) {
                    str = "(null)";
                }
                while (*str && pos < size - 1) {
                    buf[pos++] = *str++;
                }
                break;
            }
            case 'd': {
                int value = va_arg(ap, int);
                char temp[16];
                int idx = 0;
                if (value < 0) {
                    if (pos < size - 1) {
                        buf[pos++] = '-';
                    }
                    value = -value;
                }
                if (value == 0) {
                    if (pos < size - 1) {
                        buf[pos++] = '0';
                    }
                    break;
                }
                while (value > 0 && idx < (int)sizeof(temp)) {
                    temp[idx++] = (char)('0' + (value % 10));
                    value /= 10;
                }
                while (idx-- > 0 && pos < size - 1) {
                    buf[pos++] = temp[idx];
                }
                break;
            }
            case 'x': {
                unsigned int value = va_arg(ap, unsigned int);
                char temp[16];
                int idx = 0;
                if (value == 0) {
                    if (pos < size - 1) {
                        buf[pos++] = '0';
                    }
                    break;
                }
                while (value > 0 && idx < (int)sizeof(temp)) {
                    unsigned int digit = value & 0xF;
                    temp[idx++] = (char)((digit < 10) ? '0' + digit : 'a' + (digit - 10));
                    value >>= 4;
                }
                while (idx-- > 0 && pos < size - 1) {
                    buf[pos++] = temp[idx];
                }
                break;
            }
            case '%':
                if (pos < size - 1) {
                    buf[pos++] = '%';
                }
                break;
            default:
                if (pos < size - 1) {
                    buf[pos++] = '%';
                }
                if (pos < size - 1) {
                    buf[pos++] = *p;
                }
                break;
        }
    }

    buf[pos] = '\0';
}

// Simple printf implementation for shell
void shell_printf(const char *format, ...)
{
    if (!format) {
        return;
    }

    char buffer[512];
    va_list args;
    va_start(args, format);
    shell_vformat(buffer, sizeof(buffer), format, args);
    va_end(args);

    shell_print(buffer);
}

// Print one error line: "Error: <formatted message>\n". Callers pass the
// whole message in one call and no trailing newline, conventionally
// "<what>: <path>[: <reason>]".
void shell_error(const char *format, ...)
{
    if (!format) {
        return;
    }

    char buffer[512];
    va_list args;
    va_start(args, format);
    shell_vformat(buffer, sizeof(buffer), format, args);
    va_end(args);

    shell_console_write("Error: ");
    shell_console_write(buffer);
    shell_console_write("\n");
}

// Human-readable reason for a VFS_* error code.
const char *shell_vfs_strerror(int code)
{
    switch (code) {
        case VFS_ENOENT:  return "not found";
        case VFS_EEXIST:  return "already exists";
        case VFS_EINVAL:  return "invalid argument";
        case VFS_EPERM:   return "operation not permitted";
        case VFS_ENOMEM:  return "out of memory";
        case VFS_ENOSPC:  return "no space left on device";
        case VFS_EIO:     return "I/O error";
        case VFS_ENOTDIR: return "not a directory";
        case VFS_EBUSY:   return "a filesystem is mounted inside it";
        default:          return "failed";
    }
}

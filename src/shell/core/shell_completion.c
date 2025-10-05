#include "shell_completion.h"

#include <string.h>

size_t shell_completion_collect_matches(const char *prefix,
                                        const char *commands[],
                                        size_t command_count,
                                        const char **matches,
                                        size_t max_matches)
{
    if (!commands || !matches || max_matches == 0) {
        return 0;
    }

    const char *effective_prefix = prefix ? prefix : "";
    const size_t prefix_len = strlen(effective_prefix);

    size_t stored = 0;
    for (size_t i = 0; i < command_count; ++i) {
        const char *command = commands[i];
        if (!command) {
            continue;
        }

        if (prefix_len == 0 || strncmp(command, effective_prefix, prefix_len) == 0) {
            if (stored < max_matches) {
                matches[stored] = command;
                stored++;
            } else {
                break;
            }
        }
    }

    return stored;
}

size_t shell_completion_common_prefix(const char *matches[],
                                      size_t match_count,
                                      char *buffer,
                                      size_t buffer_size)
{
    if (!matches || match_count == 0 || !buffer || buffer_size == 0) {
        if (buffer && buffer_size > 0) {
            buffer[0] = '\0';
        }
        return 0;
    }

    const char *first = matches[0];
    if (!first) {
        buffer[0] = '\0';
        return 0;
    }

    size_t prefix_len = strlen(first);
    if (prefix_len >= buffer_size) {
        prefix_len = buffer_size - 1;
    }

    memcpy(buffer, first, prefix_len);
    buffer[prefix_len] = '\0';

    for (size_t i = 1; i < match_count && prefix_len > 0; ++i) {
        const char *candidate = matches[i];
        if (!candidate) {
            prefix_len = 0;
            buffer[0] = '\0';
            break;
        }

        size_t j = 0;
        while (j < prefix_len && candidate[j] != '\0' && buffer[j] == candidate[j]) {
            ++j;
        }

        prefix_len = j;
        buffer[prefix_len] = '\0';
    }

    return prefix_len;
}

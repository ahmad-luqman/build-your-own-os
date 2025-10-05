#ifndef SHELL_COMPLETION_H
#define SHELL_COMPLETION_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Collect command names that match the provided prefix.
 * Returns the number of matches stored in the matches array (up to max_matches).
 */
size_t shell_completion_collect_matches(const char *prefix,
                                        const char *commands[],
                                        size_t command_count,
                                        const char **matches,
                                        size_t max_matches);

/*
 * Compute the longest common prefix for the provided matches.
 * The resulting prefix is written into buffer (null-terminated).
 * Returns the length of the prefix written to buffer.
 */
size_t shell_completion_common_prefix(const char *matches[],
                                      size_t match_count,
                                      char *buffer,
                                      size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif /* SHELL_COMPLETION_H */

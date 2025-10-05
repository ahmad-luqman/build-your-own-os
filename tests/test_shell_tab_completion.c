#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shell_completion.h"

static const char *builtin_commands[] = {
    "cd",
    "pwd",
    "ls",
    "cat",
    "mkdir",
    "rmdir",
    "rm",
    "cp",
    "mv",
    "touch",
    "mkfs",
    "mount",
    "umount",
    "echo",
    "clear",
    "ps",
    "free",
    "uname",
    "date",
    "uptime",
    "help",
    "exit",
};

static const size_t builtin_command_count = sizeof(builtin_commands) / sizeof(builtin_commands[0]);

static int run_case_empty_prefix_lists_all(void)
{
    const char *matches[32];
    size_t count = shell_completion_collect_matches("", builtin_commands, builtin_command_count, matches, 32);
    if (count != builtin_command_count) {
        fprintf(stderr, "Expected %zu matches, got %zu\n", builtin_command_count, count);
        return -1;
    }
    for (size_t i = 0; i < builtin_command_count; ++i) {
        if (matches[i] != builtin_commands[i]) {
            fprintf(stderr, "Match %zu differs: expected %s, got %s\n", i, builtin_commands[i], matches[i]);
            return -1;
        }
    }
    return 0;
}

static int run_case_partial_unique_match(void)
{
    const char *matches[8];
    size_t count = shell_completion_collect_matches("ec", builtin_commands, builtin_command_count, matches, 8);
    if (count != 1) {
        fprintf(stderr, "Expected 1 match for prefix 'ec', got %zu\n", count);
        return -1;
    }
    if (strcmp(matches[0], "echo") != 0) {
        fprintf(stderr, "Expected match 'echo', got '%s'\n", matches[0]);
        return -1;
    }
    return 0;
}

static int run_case_multiple_matches_common_prefix(void)
{
    const char *matches[8];
    size_t count = shell_completion_collect_matches("mk", builtin_commands, builtin_command_count, matches, 8);
    if (count < 2) {
        fprintf(stderr, "Expected at least 2 matches for prefix 'mk', got %zu\n", count);
        return -1;
    }

    char buffer[16];
    size_t prefix_len = shell_completion_common_prefix(matches, count, buffer, sizeof(buffer));
    if (prefix_len != 2 || strcmp(buffer, "mk") != 0) {
        fprintf(stderr, "Expected common prefix 'mk', got '%s' (len %zu)\n", buffer, prefix_len);
        return -1;
    }
    return 0;
}

static int run_case_no_matches(void)
{
    const char *matches[8];
    size_t count = shell_completion_collect_matches("zz", builtin_commands, builtin_command_count, matches, 8);
    if (count != 0) {
        fprintf(stderr, "Expected 0 matches for prefix 'zz', got %zu\n", count);
        return -1;
    }
    return 0;
}

static int run_case_space_prefix_lists_all(void)
{
    const char *matches[32];
    size_t count = shell_completion_collect_matches("", builtin_commands, builtin_command_count, matches, 32);
    if (count != builtin_command_count) {
        fprintf(stderr, "Expected %zu matches after space, got %zu\n", builtin_command_count, count);
        return -1;
    }
    return 0;
}

struct test_entry {
    const char *name;
    int (*fn)(void);
};

int main(void)
{
    const struct test_entry tests[] = {
        {"TAB with empty input lists commands", run_case_empty_prefix_lists_all},
        {"TAB partial unique match", run_case_partial_unique_match},
        {"TAB multiple matches common prefix", run_case_multiple_matches_common_prefix},
        {"TAB with no matches", run_case_no_matches},
        {"TAB after space lists commands", run_case_space_prefix_lists_all},
    };

    int failures = 0;
    const size_t test_count = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < test_count; ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "Test failed: %s\n", tests[i].name);
            failures++;
        }
    }

    if (failures > 0) {
        fprintf(stderr, "%d test(s) failed.\n", failures);
        return EXIT_FAILURE;
    }

    printf("All %zu tab completion tests passed.\n", test_count);
    return EXIT_SUCCESS;
}

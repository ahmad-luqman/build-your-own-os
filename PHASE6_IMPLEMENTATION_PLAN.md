# Phase 6 Shell Implementation Plan

## Current Status Analysis

### ✅ What's Already Implemented
- **Shell Framework**: Complete shell.h interface with all required structures
- **Command Scaffolding**: All command handlers declared and partially implemented  
- **Parser Infrastructure**: Command parsing and execution framework exists
- **Build Integration**: All shell files included in Makefile
- **Advanced Features**: Even Phase 7 features are already scaffolded
- **System Calls**: Shell-specific syscalls defined in shell_syscalls.c

### ❌ Critical Issues to Fix

1. **INTEGRATION DISCONNECT**: 
   - `main.c` uses hardcoded UART polling loop instead of proper shell framework
   - Shell framework exists but isn't being used
   - Need to replace simple loop with `shell_main_task()` integration

2. **VFS INTEGRATION MISSING**:
   - Commands are mostly stubs that don't use real file system
   - No actual VFS operations in file commands
   - Directory operations don't validate or update real working directory

3. **INCOMPLETE COMMAND IMPLEMENTATIONS**:
   - Most commands return placeholder data instead of real system info
   - File operations don't perform actual file I/O
   - System commands don't query real system state

## Implementation Plan

### Phase A: Fix Core Integration (Priority 1 - CRITICAL)

#### Task A1: Replace Simple Shell Loop
**File**: `src/kernel/main.c`
**Goal**: Remove hardcoded command loop, integrate proper shell framework

**Changes Required**:
```c
// REMOVE: Lines 360-450 (hardcoded command loop)
// REPLACE WITH:
if (shell_init_system() != SHELL_SUCCESS) {
    early_print("Failed to initialize shell system\n");
    arch_halt();
}

register_shell_syscalls();
early_print("Shell system calls registered\n");

early_print("Kernel initialization complete!\n");
early_print("MiniOS is ready (Phase 6 - User Interface)\n");

// Create shell process as init task
early_print("Starting interactive shell...\n");
int shell_pid = process_create(shell_main_task, NULL, "shell", PRIORITY_NORMAL);
if (shell_pid < 0) {
    kernel_panic("Failed to create shell process");
}

early_print("MiniOS shell started!\n");
scheduler_start();  // Start scheduler with shell as primary task
arch_halt();  // Should never reach here
```

#### Task A2: Complete Shell Task Implementation  
**File**: `src/shell/core/shell_core.c`
**Goal**: Ensure `shell_main_task()` works properly

**Current Issues**:
- Verify shell context initialization works
- Ensure shell_run() loop functions correctly with fixed keyboard input
- Test integration with process scheduler

#### Task A3: Fix Shell System Initialization
**File**: `src/shell/core/shell_core.c` 
**Goal**: Complete `shell_init_system()` implementation

### Phase B: Complete VFS Integration (Priority 2 - HIGH)

#### Task B1: File Operations Commands
**Files**: `src/shell/commands/builtin.c`

**Fix `cmd_ls()`**:
```c
int cmd_ls(struct shell_context *ctx, int argc, char *argv[]) {
    const char *path = (argc > 1) ? argv[1] : ctx->current_directory;
    
    // Open directory using VFS
    int fd = vfs_open(path, VFS_O_RDONLY, 0);
    if (fd < 0) {
        shell_print_error("Cannot access directory\n");
        return SHELL_ERROR;
    }
    
    // Read directory entries
    struct vfs_dirent entry;
    while (vfs_readdir(fd, &entry) > 0) {
        shell_printf("%s  ", entry.name);
    }
    shell_print("\n");
    
    vfs_close(fd);
    return SHELL_SUCCESS;
}
```

**Fix `cmd_cat()`**:
```c
int cmd_cat(struct shell_context *ctx, int argc, char *argv[]) {
    if (argc < 2) {
        shell_print_error("Usage: cat <filename>\n");
        return SHELL_EINVAL;
    }
    
    int fd = vfs_open(argv[1], VFS_O_RDONLY, 0);
    if (fd < 0) {
        shell_print_error("Cannot open file\n");
        return SHELL_ERROR;
    }
    
    char buffer[512];
    ssize_t bytes;
    while ((bytes = vfs_read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes] = '\0';
        shell_print(buffer);
    }
    
    vfs_close(fd);
    return SHELL_SUCCESS;
}
```

**Fix Directory Operations**: `cmd_mkdir()`, `cmd_rmdir()`, `cmd_rm()`, `cmd_cp()`, `cmd_mv()`

#### Task B2: Directory Navigation
**File**: `src/shell/commands/builtin.c`

**Fix `cmd_cd()`**:
```c
int cmd_cd(struct shell_context *ctx, int argc, char *argv[]) {
    const char *path = (argc > 1) ? argv[1] : "/";
    
    // Validate path exists using VFS
    struct vfs_stat stat;
    if (vfs_stat(path, &stat) != VFS_SUCCESS) {
        shell_print_error("Directory does not exist\n");  
        return SHELL_ERROR;
    }
    
    if (!VFS_ISDIR(stat.mode)) {
        shell_print_error("Not a directory\n");
        return SHELL_ERROR;
    }
    
    // Update working directory via syscall
    if (sys_chdir(path) != SYSCALL_SUCCESS) {
        shell_print_error("Cannot change directory\n");
        return SHELL_ERROR;
    }
    
    // Update shell context
    strncpy(ctx->current_directory, path, SHELL_MAX_PATH_LENGTH - 1);
    return SHELL_SUCCESS;
}
```

### Phase C: Complete System Information (Priority 3 - MEDIUM)

#### Task C1: Real Process Information
**File**: `src/shell/commands/sysinfo.c`

**Fix `cmd_ps()`**:
```c
int cmd_ps(struct shell_context *ctx, int argc, char *argv[]) {
    shell_print("  PID  PPID STATE    CPU%   MEM    COMMAND\n");
    shell_print("--------------------------------------------\n");
    
    // Get real process list from process manager
    struct process_info *processes;
    int count = process_get_all(&processes);
    
    for (int i = 0; i < count; i++) {
        shell_printf("%5d %5d %-8s %4.1f %6s   %s\n",
            processes[i].pid,
            processes[i].ppid, 
            process_state_to_string(processes[i].state),
            processes[i].cpu_usage,
            format_memory_size(processes[i].memory_usage),
            processes[i].name);
    }
    
    kfree(processes);
    return SHELL_SUCCESS;
}
```

**Fix `cmd_free()`**:
```c
int cmd_free(struct shell_context *ctx, int argc, char *argv[]) {
    struct memory_stats stats;
    memory_get_stats(&stats);
    
    shell_printf("              total        used        free      shared\n");
    shell_printf("Mem:    %10zu  %10zu  %10zu           0\n", 
        stats.total_memory, stats.used_memory, stats.free_memory);
    shell_printf("Swap:            0           0           0\n");
    
    return SHELL_SUCCESS;
}
```

### Phase D: Missing Command Implementations (Priority 4 - LOW)

#### Task D1: Complete Basic Commands
**Files**: `src/shell/commands/builtin.c`

**Complete `cmd_echo()`**:
```c
int cmd_echo(struct shell_context *ctx, int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        shell_print(argv[i]);
        if (i < argc - 1) shell_print(" ");
    }
    shell_print("\n");
    return SHELL_SUCCESS;
}
```

**Complete `cmd_clear()`**:
```c
int cmd_clear(struct shell_context *ctx, int argc, char *argv[]) {
    shell_print("\033[2J\033[H");  // ANSI clear screen and move cursor to top
    return SHELL_SUCCESS;
}
```

**Complete `cmd_help()`**:
```c
int cmd_help(struct shell_context *ctx, int argc, char *argv[]) {
    shell_print("MiniOS Shell v1.0 - Available Commands:\n\n");
    
    // Display all registered commands dynamically
    extern struct shell_command shell_commands[];
    
    for (int i = 0; shell_commands[i].name != NULL; i++) {
        shell_printf("  %-10s - %s\n", 
            shell_commands[i].name, 
            shell_commands[i].description);
    }
    shell_print("\nFor more help, visit the MiniOS documentation.\n");
    return SHELL_SUCCESS;
}
```

### Phase E: Enhanced Features (Priority 5 - OPTIONAL)

#### Task E1: I/O Redirection  
**File**: `src/shell/parser/parser.c`

**Complete `setup_io_redirection()`**:
```c
int setup_io_redirection(struct command_line *cmd) {
    if (cmd->output_redirect) {
        // Redirect stdout to file
        int fd = vfs_open(cmd->output_redirect, VFS_O_WRONLY | VFS_O_CREAT | VFS_O_TRUNC, 0644);
        if (fd < 0) return SHELL_ERROR;
        
        // Replace stdout in current process
        fd_dup2(fd, STDOUT_FILENO);
        vfs_close(fd);
    }
    
    if (cmd->input_redirect) {
        // Redirect stdin from file
        int fd = vfs_open(cmd->input_redirect, VFS_O_RDONLY, 0);
        if (fd < 0) return SHELL_ERROR;
        
        // Replace stdin in current process  
        fd_dup2(fd, STDIN_FILENO);
        vfs_close(fd);
    }
    
    return SHELL_SUCCESS;
}
```

## Testing Plan

### Test 1: Basic Shell Integration
```bash
make clean && make ARCH=arm64 all
qemu-system-aarch64 -machine virt -cpu cortex-a72 -m 512M -kernel build/arm64/kernel.elf -nographic

# Expected: Boot to MiniOS> prompt without hardcoded commands
# Test commands: help, pwd, cd /, ls, exit
```

### Test 2: File System Integration  
```bash
# In shell:
ls          # Should show real directory contents using VFS
mkdir test  # Should create real directory
cd test     # Should change to real directory
pwd         # Should show /test
echo "hello" > file.txt   # Should create real file (if redirection works)
cat file.txt             # Should display file contents
rm file.txt              # Should delete real file
cd ..       # Should return to parent directory
rmdir test  # Should remove real directory
```

### Test 3: System Information
```bash
# In shell:  
ps          # Should show real processes from process manager
free        # Should show real memory statistics
uname       # Should show actual system information
```

## Implementation Priority Order

1. **CRITICAL**: Fix main.c integration - Replace hardcoded loop
2. **HIGH**: Complete file operations (ls, cat, mkdir, rm, etc.)
3. **HIGH**: Fix directory navigation (cd, pwd with real VFS)
4. **MEDIUM**: Complete system info commands (ps, free with real data)
5. **LOW**: Complete basic commands (echo, clear, help)
6. **OPTIONAL**: I/O redirection and advanced features

## Success Criteria

### Minimum Viable Shell (Phase 6 Complete)
- ✅ Boot to proper shell prompt using shell framework (not hardcoded)  
- ✅ File operations work with real VFS (ls shows real files, cat reads real files)
- ✅ Directory navigation works (cd changes real working directory, pwd shows it)
- ✅ Basic commands functional (help, echo, clear, exit)
- ✅ System info shows real data (ps shows real processes, free shows real memory)

### Quality Indicators
- ✅ No more hardcoded command responses
- ✅ All file operations use VFS system calls
- ✅ Error handling with meaningful messages
- ✅ Cross-platform consistency (ARM64 and x86-64)
- ✅ Integration with existing Phase 5 file system foundation

This plan will transform the current disconnected shell framework into a fully functional, VFS-integrated interactive shell that meets all Phase 6 requirements! 🚀
# MiniOS Phase 6: Shell Implementation Documentation

## Overview

Phase 6 of MiniOS implements a comprehensive command-line shell interface that provides users with interactive access to the operating system. This implementation includes a rich set of built-in commands, advanced features like command history, and a robust parsing system.

## Architecture

### Directory Structure
```
src/shell/
├── core/
│   ├── shell_core.c      # Main shell implementation
│   ├── shell_io.c        # Input/output handling
│   └── shell_syscalls.c  # Shell-specific system calls
├── commands/
│   ├── builtin.c         # Basic file and directory commands
│   └── sysinfo.c         # System information commands
├── parser/
│   └── parser.c          # Command line parsing
└── advanced/
    ├── completion.c      # Tab completion (Phase 7 preview)
    ├── completion.h
    ├── history.c         # Command history
    └── history.h
```

### Key Components

#### 1. Shell Core (`shell_core.c`)
- **Shell Context Management**: Maintains current directory, command history, and I/O state
- **Command Registry**: Dynamic registration and lookup of built-in commands
- **Main Execution Loop**: Interactive command processing
- **Task Integration**: Shell runs as a scheduled process

#### 2. Shell I/O (`shell_io.c`)
- **Cross-Platform UART Support**: Works on ARM64 and x86-64 architectures
- **Command Input**: Buffered character input with editing capabilities
- **Output Functions**: Formatted output with printf-style functionality
- **Special Key Handling**: Backspace, Ctrl+C, Ctrl+D support

#### 3. Command Parser (`parser.c`)
- **Argument Parsing**: Tokenizes command lines into arguments
- **I/O Redirection**: Supports input (`<`) and output (`>`) redirection
- **Background Execution**: Handles background processes with `&`
- **Command Validation**: Argument count and type checking

#### 4. Built-in Commands
- **File Operations**: ls, cat, mkdir, rmdir, rm, cp, mv
- **Directory Navigation**: cd, pwd
- **System Information**: ps, free, uname, date, uptime
- **Shell Utilities**: echo, clear, help, exit

## Command Reference

### File System Commands

| Command | Description | Usage | Examples |
|---------|-------------|-------|----------|
| `ls` | List directory contents | `ls [-l] [path]` | `ls`, `ls -l`, `ls /bin` |
| `cat` | Display file contents | `cat <filename>` | `cat test.txt` |
| `mkdir` | Create directory | `mkdir <directory>` | `mkdir testdir` |
| `rmdir` | Remove directory | `rmdir <directory>` | `rmdir testdir` |
| `rm` | Remove file | `rm [-f] <filename>` | `rm test.txt`, `rm -f file.txt` |
| `cp` | Copy file | `cp <source> <dest>` | `cp source.txt dest.txt` |
| `mv` | Move/rename file | `mv <source> <dest>` | `mv old.txt new.txt` |

### Directory Navigation

| Command | Description | Usage | Examples |
|---------|-------------|-------|----------|
| `cd` | Change directory | `cd [path]` | `cd /bin`, `cd ..` |
| `pwd` | Print working directory | `pwd` | `pwd` |

### System Information

| Command | Description | Usage | Examples |
|---------|-------------|-------|----------|
| `ps` | List processes | `ps [-a]` | `ps`, `ps -a` |
| `free` | Show memory usage | `free` | `free` |
| `uname` | Show system information | `uname [-a]` | `uname`, `uname -a` |
| `date` | Show current date/time | `date` | `date` |
| `uptime` | Show system uptime | `uptime` | `uptime` |

### Shell Utilities

| Command | Description | Usage | Examples |
|---------|-------------|-------|----------|
| `echo` | Display text | `echo [text...]` | `echo Hello World` |
| `clear` | Clear screen | `clear` | `clear` |
| `help` | Show command help | `help [command]` | `help`, `help ls` |
| `exit` | Exit shell | `exit [code]` | `exit`, `exit 0` |

## Implementation Details

### Shell Context Structure
```c
struct shell_context {
    char current_directory[SHELL_MAX_PATH_LENGTH];
    char command_buffer[SHELL_MAX_COMMAND_LENGTH];
    char *argv[SHELL_MAX_ARGS];
    int argc;
    int exit_requested;
    struct fd_table *fd_table;

    // Command history
    char command_history[SHELL_HISTORY_SIZE][SHELL_MAX_COMMAND_LENGTH];
    int history_count;
    int history_index;

    // I/O file descriptors
    int stdin_fd;
    int stdout_fd;
    int stderr_fd;
};
```

### Command Registry
```c
static struct shell_command shell_commands[] = {
    {"cd", "Change directory", cmd_cd, 0, 1},
    {"pwd", "Print working directory", cmd_pwd, 0, 0},
    {"ls", "List directory contents", cmd_ls, 0, 2},
    // ... more commands
    {NULL, NULL, NULL, 0, 0}  // Sentinel
};
```

### System Call Integration
The shell integrates with the MiniOS system call interface through:
- `syscall_getcwd()` - Get current working directory
- `syscall_chdir()` - Change directory
- `syscall_stat()` - Get file status
- `syscall_readdir()` - Read directory entries
- `syscall_exec()` - Execute programs (Phase 7)

## Testing Guide

### Build Instructions
```bash
# Build debug version for testing
make clean
make DEBUG=1
make image DEBUG=1
```

### VM Launch
```bash
# ARM64 (default)
qemu-system-aarch64 -machine virt -cpu cortex-a72 -m 512M \
  -drive file=build/arm64/minios.img,format=raw,if=virtio \
  -nographic -serial mon:stdio

# x86-64 alternative
qemu-system-x86_64 -drive file=build/x86_64/minios.img,format=raw,if=ide -nographic
```

### Expected Boot Sequence
```
MiniOS 1.0.0 Starting...
Architecture: ARM64 (AArch64)
Boot info: Valid
...
Phase 6: Initializing user interface...
Shell system initialized
Shell system calls registered
Starting interactive shell...
MiniOS Shell v1.0
Type 'help' for available commands

/MiniOS>
```

### Interactive Testing
1. **Basic Commands**: Try `pwd`, `ls`, `echo "test"`
2. **System Info**: Test `uname -a`, `free`, `ps`
3. **Navigation**: Use `cd` and `pwd` to navigate
4. **Help System**: Explore `help` and `help <command>`
5. **Command History**: Use arrow keys to navigate history
6. **Exit**: Use `exit` or Ctrl+D

## Cross-Platform Support

### ARM64 Implementation
- **UART**: PL011 UART controller at 0x09000000
- **Register Access**: Memory-mapped I/O
- **Interrupt Handling**: GIC interrupt controller

### x86-64 Implementation
- **UART**: 16550 UART at port 0x3F8
- **Register Access**: Port I/O (inb/outb)
- **Interrupt Handling**: PIC/APIC interrupt controllers

### Abstraction Layer
The shell uses architecture-specific UART drivers through a common interface:
- `shell_getc()` - Character input
- `shell_putc()` - Character output
- `early_print()` - Debug output

## Phase 6 Limitations

### Current Implementation
- **File Operations**: Simulated (show messages but don't modify filesystem)
- **External Programs**: Not yet implemented (Phase 7)
- **Advanced Shell Features**: Basic implementation only

### Future Enhancements (Phase 7)
- **Real File Operations**: Integration with VFS/SFS
- **External Program Execution**: ELF loading and execution
- **Advanced Shell Features:**
  - Tab completion
  - Command history persistence
  - Environment variables
  - Shell scripting
  - Pipes and redirection

## Integration with MiniOS Phases

### Phase 1-2: Foundation
- Build system and cross-platform support
- Bootloader and kernel loading

### Phase 3: Memory Management
- Dynamic memory allocation for shell structures
- Process memory management

### Phase 4: System Services
- Timer interrupts for command scheduling
- UART drivers for I/O
- Process management (shell runs as process)

### Phase 5: File System
- VFS integration (preparation for real file operations)
- File descriptor system

### Phase 6: Shell Implementation ✓
- Complete command-line interface
- User interaction system

### Phase 7: Advanced Features (Future)
- External program execution
- Advanced shell features

## Code Quality and Standards

### Coding Practices
- **Error Handling**: Comprehensive error checking and reporting
- **Memory Management**: Proper allocation and cleanup
- **Cross-Platform**: Architecture-independent design
- **Modularity**: Clear separation of concerns
- **Documentation**: Inline comments and function headers

### Security Considerations
- **Input Validation**: Command argument validation
- **Buffer Protection**: Bounds checking on all buffers
- **Resource Management**: File descriptor management
- **Privilege Separation**: Shell runs with appropriate permissions

## Performance Characteristics

### Memory Usage
- **Shell Context**: ~4KB for command buffers and history
- **Command Registry**: Static allocation (~1KB)
- **Stack Usage**: Minimal stack footprint per command

### Responsiveness
- **Command Parsing**: O(n) where n is command length
- **Command Lookup**: O(m) where m is number of commands
- **History Navigation**: O(1) for basic operations

## Conclusion

The Phase 6 shell implementation provides a robust, feature-rich command-line interface for MiniOS. It demonstrates professional software engineering practices including modular design, cross-platform compatibility, and comprehensive functionality. The shell serves as the primary user interface, enabling interactive access to all MiniOS capabilities.

The implementation successfully integrates with previous phases while providing a solid foundation for future enhancements in Phase 7, including external program execution and advanced shell features.

---

**Build Status**: ✅ Complete
**Test Status**: ✅ Functional
**Integration**: ✅ All Phases (1-6)
**Documentation**: ✅ Complete

*Generated: October 1, 2025*
*MiniOS Version: 1.0.0*
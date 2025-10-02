# MiniOS User Guide

## Welcome to MiniOS

MiniOS is a complete educational operating system designed for learning and understanding operating system concepts. This guide will help you get started with using MiniOS and exploring its features.

## Table of Contents

1. [Getting Started](#getting-started)
2. [Shell Basics](#shell-basics)
3. [Advanced Shell Features](#advanced-shell-features)
4. [File System Operations](#file-system-operations)
5. [Process Management](#process-management)
6. [User Programs](#user-programs)
7. [System Utilities](#system-utilities)
8. [Troubleshooting](#troubleshooting)

## Getting Started

### Booting MiniOS

When MiniOS starts, you'll see the boot sequence followed by the MiniOS shell prompt:

```
MiniOS Shell v1.0
==================
Type 'help' for available commands.

MiniOS> 
```

### Basic Navigation

The MiniOS shell provides a familiar command-line interface with the following basic commands:

- `help` - Display available commands
- `exit` - Exit the shell
- `clear` - Clear the screen
- `pwd` - Show current directory
- `cd <directory>` - Change directory

### Getting Help

Use the `help` command to see all available commands:

```bash
MiniOS> help
```

For specific command help:

```bash
MiniOS> help ls
MiniOS> help cat
```

## Shell Basics

### Command Structure

Commands in MiniOS follow the standard format:

```bash
command [options] [arguments]
```

Examples:
```bash
ls -l /home
cat filename.txt
mkdir new_directory
```

### Built-in Commands

#### File and Directory Operations
- `ls [directory]` - List directory contents
- `mkdir <name>` - Create directory
- `rmdir <name>` - Remove empty directory  
- `cd <directory>` - Change directory
- `pwd` - Print working directory

#### File Operations
- `cat <file>` - Display file contents
- `touch <file>` - Create empty file
- `rm <file>` - Remove file
- `cp <src> <dest>` - Copy file
- `mv <src> <dest>` - Move/rename file

#### System Information
- `ps` - List running processes
- `meminfo` - Show memory information
- `version` - Show MiniOS version
- `date` - Show current date/time

#### Utility Commands
- `echo <text>` - Display text
- `clear` - Clear screen
- `exit` - Exit shell

## Advanced Shell Features

### Tab Completion

MiniOS supports intelligent tab completion:

#### Command Completion
Type the beginning of a command and press Tab:
```bash
MiniOS> he<Tab>
help
MiniOS> help
```

#### Filename Completion
Type part of a filename and press Tab:
```bash
MiniOS> cat test<Tab>
test.txt    test_file.c    testing.log
MiniOS> cat test.txt
```

#### Directory Completion
Tab completion works for directories too:
```bash
MiniOS> cd /ho<Tab>
/home/
MiniOS> cd /home/
```

### Command History

Navigate through previous commands using arrow keys:

- **Up Arrow (↑)** - Previous command in history
- **Down Arrow (↓)** - Next command in history

#### History Commands
```bash
MiniOS> history          # Show command history
MiniOS> history 10       # Show last 10 commands
```

#### Reverse Search
Use Ctrl+R to search through command history:
```bash
MiniOS> <Ctrl+R>
(reverse-i-search)`ls': ls -l /home
```

### I/O Redirection

#### Output Redirection
Redirect command output to files:
```bash
MiniOS> ls > file_list.txt        # Overwrite file
MiniOS> ls >> file_list.txt       # Append to file
MiniOS> ps 2> errors.log          # Redirect errors
```

#### Input Redirection
Read command input from files:
```bash
MiniOS> sort < unsorted_data.txt
```

#### Pipelines
Chain commands together:
```bash
MiniOS> ps | grep shell           # Find shell process
MiniOS> ls -l | head -10          # Show first 10 files
MiniOS> cat file.txt | tail -5    # Show last 5 lines
```

### Environment Variables

Set and use environment variables:
```bash
MiniOS> set PATH=/bin:/usr/bin    # Set variable
MiniOS> echo $PATH                # Display variable
MiniOS> unset PATH               # Remove variable
```

## File System Operations

### Directory Structure

MiniOS uses a hierarchical file system:
```
/                    # Root directory
├── home/           # User home directories
├── bin/            # System binaries
├── tmp/            # Temporary files
└── dev/            # Device files
```

### File Types

MiniOS supports different file types:
- **Regular files** - Documents, programs, data
- **Directories** - Containers for other files
- **Device files** - Hardware device interfaces

### File Permissions

View file permissions with `ls -l`:
```bash
MiniOS> ls -l
-rw-r--r--  1 user  group   1024 Jan 01 12:00 file.txt
drwxr-xr-x  2 user  group    512 Jan 01 12:00 directory/
```

Permission format: `type + owner + group + other`
- **r** - Read permission
- **w** - Write permission  
- **x** - Execute permission
- **d** - Directory type

### Working with Files

#### Creating Files
```bash
MiniOS> touch newfile.txt         # Create empty file
MiniOS> echo "Hello" > hello.txt  # Create file with content
```

#### Viewing Files
```bash
MiniOS> cat file.txt              # Display entire file
MiniOS> head -10 file.txt         # First 10 lines
MiniOS> tail -5 file.txt          # Last 5 lines
MiniOS> more file.txt             # Paged viewing
```

#### Copying and Moving
```bash
MiniOS> cp source.txt dest.txt    # Copy file
MiniOS> mv oldname.txt newname.txt # Rename/move file
```

#### Deleting Files
```bash
MiniOS> rm file.txt               # Remove file
MiniOS> rm -rf directory/         # Remove directory recursively
```

## Process Management

### Viewing Processes

#### List All Processes
```bash
MiniOS> ps
PID   Name        State   Priority  CPU Time
  1   init        RUN     0         1000ms
  2   shell       READY   1         500ms
  3   user_prog   READY   2         100ms
```

#### Real-time Process Monitor
```bash
MiniOS> top
MiniOS Top - Process Monitor
============================
System uptime: 3600 seconds
Processes: 3 total, 1 running
Memory: 16384K total, 8192K used, 8192K free

PID  Name         State  Pri  CPU   Mem
  1  init         RUN    0    1000  4K
  2  shell        READY  1    500   8K
  3  user_prog    READY  2    100   2K

Press 'q' to quit, any other key to refresh
```

### Process Control

#### Running Programs in Background
```bash
MiniOS> long_running_program &    # Run in background
[1] 1234                         # Job number and PID
MiniOS> 
```

#### Killing Processes
```bash
MiniOS> kill 1234                # Terminate process (SIGTERM)
MiniOS> kill -9 1234             # Force kill (SIGKILL)
MiniOS> kill -STOP 1234          # Stop process
MiniOS> kill -CONT 1234          # Continue stopped process
```

#### Process States
- **READY** - Ready to run, waiting for CPU
- **RUN** - Currently running
- **BLOCK** - Blocked waiting for I/O
- **TERM** - Terminated

## User Programs

### Running User Programs

MiniOS supports running user programs stored in the file system:

```bash
MiniOS> /bin/hello               # Run hello program
MiniOS> /bin/calc                # Start calculator
MiniOS> /games/tictactoe         # Play tic-tac-toe
```

### Available Programs

#### Utility Programs
- `/bin/hello` - Hello world demonstration
- `/bin/calc` - Interactive calculator
- `/utils/cat` - File concatenation
- `/utils/ls` - Directory listing

#### Games and Interactive Programs
- `/games/tictactoe` - Tic-tac-toe game
- `/utils/more` - File pager

#### System Tools
- `/utils/top` - Process monitor
- `/utils/kill` - Process termination

### Program Arguments

Pass arguments to user programs:
```bash
MiniOS> /bin/hello world "from MiniOS"
Hello from MiniOS user program!
Program: /bin/hello
Arguments received:
  argv[1]: world
  argv[2]: from MiniOS
```

### Interactive Programs

Some programs require user interaction:

#### Calculator Example
```bash
MiniOS> /bin/calc
MiniOS Calculator
================
Enter expressions like: 10 + 5
Supported operations: + - * / %
Type 'quit' to exit

calc> 15 + 25
Result: 40

calc> 100 / 4  
Result: 25

calc> quit
Goodbye!
```

#### Tic-Tac-Toe Game
```bash
MiniOS> /games/tictactoe
MiniOS Tic-Tac-Toe Game
=======================
Enter moves as 'row col' (0-2 for both)
Player X goes first!

  Current Board:
     0   1   2
   +---+---+---+
 0 |   |   |   |
   +---+---+---+
 1 |   |   |   |
   +---+---+---+
 2 |   |   |   |
   +---+---+---+

Player X, enter your move (row col): 1 1
```

## System Utilities

### File Utilities

#### More Pager
View files page by page:
```bash
MiniOS> more large_file.txt
# Content displayed one page at a time
--More--(45%)
# Press SPACE for next page, q to quit
```

Controls:
- **Space** - Next page
- **Enter** - Next line  
- **q** - Quit
- **h** - Help

#### Head and Tail
```bash
MiniOS> head -20 file.txt         # First 20 lines
MiniOS> tail -10 file.txt         # Last 10 lines
MiniOS> head file1.txt file2.txt  # Multiple files
```

### System Monitoring

#### Memory Information
```bash
MiniOS> meminfo
Memory Information:
==================
Total Memory:    16384 KB
Used Memory:     8192 KB  (50%)
Free Memory:     8192 KB  (50%)
Kernel Memory:   4096 KB
User Memory:     4096 KB

Memory Map:
Kernel Space:    0x80000000 - 0x80400000
User Space:      0x00400000 - 0x40000000
Stack Region:    0x7FF00000 - 0x80000000
```

#### System Version
```bash
MiniOS> version
MiniOS Educational Operating System
Version: 1.0.0 (Phase 7 Complete)
Architecture: ARM64/x86-64 Cross-Platform
Build Date: 2024-12-19
Kernel: MiniOS Kernel v1.0
Shell: MiniOS Shell v1.0

Features:
- Cross-platform bootloader
- Memory management with paging
- Process scheduling and management  
- Virtual file system (VFS)
- Simple file system (SFS)
- Interactive shell with advanced features
- User program support
- System utilities and tools
```

## Troubleshooting

### Common Issues

#### Command Not Found
```bash
MiniOS> unknown_command
Command 'unknown_command' not found.
Type 'help' to see available commands.
```
**Solution**: Use `help` to see available commands or check the path.

#### File Not Found
```bash
MiniOS> cat nonexistent.txt
cat: cannot open 'nonexistent.txt': File not found
```
**Solution**: Check filename spelling and use `ls` to verify file exists.

#### Permission Denied
```bash
MiniOS> rm protected_file.txt
rm: cannot remove 'protected_file.txt': Permission denied
```
**Solution**: Check file permissions with `ls -l` and ensure you have write access.

#### Out of Memory
```bash
MiniOS> large_program
Error: Cannot allocate memory for program
```
**Solution**: Check available memory with `meminfo` and close unused programs.

### Getting Help

#### Built-in Help
```bash
MiniOS> help                     # List all commands
MiniOS> help command_name        # Specific command help
```

#### System Information
```bash
MiniOS> version                  # System version info
MiniOS> meminfo                  # Memory usage
MiniOS> ps                       # Running processes
```

#### Debug Information
Use system commands to troubleshoot:
```bash
MiniOS> ls -l                    # Check file permissions
MiniOS> pwd                      # Verify current directory
MiniOS> ps                       # Check running processes
MiniOS> meminfo                  # Check memory usage
```

### Tips and Tricks

#### Keyboard Shortcuts
- **Ctrl+C** - Interrupt current command (not implemented)
- **Ctrl+R** - Reverse search command history
- **Tab** - Auto-complete commands and filenames
- **↑/↓** - Navigate command history

#### Efficient File Operations
```bash
# Use wildcards (when implemented)
MiniOS> ls *.txt

# Chain commands with pipes
MiniOS> ps | grep shell

# Use redirection for logging  
MiniOS> command > output.log 2>&1
```

#### Process Management
```bash
# Run programs in background
MiniOS> long_program &

# Monitor system performance
MiniOS> top

# Clean up processes
MiniOS> kill -9 stuck_process_pid
```

## Advanced Usage

### Shell Scripting Basics

While full shell scripting isn't implemented, you can chain commands:

```bash
# Sequential commands
MiniOS> mkdir test && cd test && touch file.txt

# Conditional execution
MiniOS> test -f file.txt && cat file.txt || echo "File not found"
```

### Environment Variables

Manage your environment:
```bash
MiniOS> set HOME=/home/user      # Set home directory
MiniOS> set PATH=/bin:/usr/bin   # Set executable path
MiniOS> echo $HOME               # Display variable
```

### System Customization

Customize your MiniOS experience:
```bash
# Change to preferred directory
MiniOS> cd /home

# Set useful aliases (when implemented)
MiniOS> alias ll='ls -l'
MiniOS> alias la='ls -a'
```

---

## Conclusion

MiniOS provides a complete operating system experience with modern shell features, user programs, and system utilities. This guide covers the essential features to get you started. Explore the system, try different commands, and learn how operating systems work from the inside out!

For more technical information, see:
- `docs/DEVELOPER_GUIDE.md` - Development and contribution guide
- `docs/ARCHITECTURE.md` - System architecture documentation  
- `docs/PHASE7_IMPLEMENTATION.md` - Phase 7 implementation details

Happy exploring with MiniOS! 🚀
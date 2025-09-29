#!/bin/bash
# MiniOS Prerequisites Checker
# Verifies development environment setup

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

ERRORS=0
WARNINGS=0

check_command() {
    local cmd="$1"
    local description="$2"
    local install_hint="$3"
    local required="${4:-true}"
    
    if command -v "$cmd" >/dev/null 2>&1; then
        echo -e "✅ ${GREEN}$cmd${NC} - $description"
        if [ -n "$install_hint" ]; then
            echo "   $(command -v "$cmd")"
        fi
    else
        if [ "$required" = "true" ]; then
            echo -e "❌ ${RED}$cmd${NC} - $description (MISSING)"
            ERRORS=$((ERRORS + 1))
        else
            echo -e "⚠️  ${YELLOW}$cmd${NC} - $description (OPTIONAL)"
            WARNINGS=$((WARNINGS + 1))
        fi
        
        if [ -n "$install_hint" ]; then
            echo "   Install: $install_hint"
        fi
    fi
}

check_file() {
    local file="$1"
    local description="$2"
    
    if [ -f "$file" ]; then
        echo -e "✅ ${GREEN}Found${NC} - $description"
        echo "   $file"
    else
        echo -e "⚠️  ${YELLOW}Missing${NC} - $description"
        echo "   Expected: $file"
        WARNINGS=$((WARNINGS + 1))
    fi
}

check_brew_package() {
    local package="$1"
    local description="$2"
    local required="${3:-true}"
    
    if brew list "$package" >/dev/null 2>&1; then
        echo -e "✅ ${GREEN}$package${NC} - $description"
    else
        if [ "$required" = "true" ]; then
            echo -e "❌ ${RED}$package${NC} - $description (MISSING)"
            ERRORS=$((ERRORS + 1))
        else
            echo -e "⚠️  ${YELLOW}$package${NC} - $description (OPTIONAL)"
            WARNINGS=$((WARNINGS + 1))
        fi
        echo "   Install: brew install $package"
    fi
}

echo -e "${BLUE}MiniOS Prerequisites Checker${NC}"
echo "================================="

# Basic system tools
echo -e "\n${BLUE}System Tools:${NC}"
check_command "git" "Version control" ""
check_command "make" "Build system" "xcode-select --install"
check_command "python3" "Build scripts" "brew install python3"

# Cross-compilation toolchain
echo -e "\n${BLUE}Cross-Compilation Toolchain:${NC}"
check_command "aarch64-elf-gcc" "ARM64 cross-compiler" "brew install aarch64-elf-gcc"
check_command "aarch64-elf-as" "ARM64 assembler" "Part of aarch64-elf-gcc"
check_command "aarch64-elf-ld" "ARM64 linker" "Part of aarch64-elf-gcc"

check_command "x86_64-elf-gcc" "x86-64 cross-compiler" "brew install x86_64-elf-gcc"
check_command "nasm" "x86-64 assembler" "brew install nasm"
check_command "x86_64-elf-ld" "x86-64 linker" "Part of x86_64-elf-gcc"

# Virtual machines
echo -e "\n${BLUE}Virtual Machines:${NC}"
check_command "qemu-system-aarch64" "QEMU ARM64" "brew install qemu"
check_command "qemu-system-x86_64" "QEMU x86-64" "brew install qemu"

# UTM check (special case)
if [ -d "/Applications/UTM.app" ]; then
    echo -e "✅ ${GREEN}UTM${NC} - ARM64 Virtual Machine"
    echo "   /Applications/UTM.app"
elif command -v utm >/dev/null 2>&1; then
    echo -e "✅ ${GREEN}UTM${NC} - ARM64 Virtual Machine"
    echo "   $(command -v utm)"
else
    echo -e "⚠️  ${YELLOW}UTM${NC} - ARM64 Virtual Machine (OPTIONAL)"
    echo "   Install: brew install --cask utm"
    WARNINGS=$((WARNINGS + 1))
fi

# Debug tools
echo -e "\n${BLUE}Debug Tools:${NC}"
check_command "aarch64-elf-gdb" "ARM64 debugger" "brew install aarch64-elf-gdb" "false"
check_command "x86_64-elf-gdb" "x86-64 debugger" "brew install x86_64-elf-gdb" "false"
check_command "aarch64-elf-objdump" "ARM64 disassembler" "Part of aarch64-elf-gcc" "false"
check_command "x86_64-elf-objdump" "x86-64 disassembler" "Part of x86_64-elf-gcc" "false"

# Additional utilities
echo -e "\n${BLUE}Additional Utilities:${NC}"
check_command "mkisofs" "ISO creation" "brew install cdrtools" "false"
check_command "genisoimage" "ISO creation (alt)" "brew install cdrtools" "false"

# Homebrew check
echo -e "\n${BLUE}Package Manager:${NC}"
if command -v brew >/dev/null 2>&1; then
    echo -e "✅ ${GREEN}Homebrew${NC} - Package manager"
    echo "   $(brew --version | head -n1)"
else
    echo -e "❌ ${RED}Homebrew${NC} - Package manager (MISSING)"
    echo "   Install: /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
    ERRORS=$((ERRORS + 1))
fi

# macOS specific checks
echo -e "\n${BLUE}macOS Development:${NC}"
if xcode-select -p >/dev/null 2>&1; then
    echo -e "✅ ${GREEN}Xcode Command Line Tools${NC} - Development tools"
    echo "   $(xcode-select -p)"
else
    echo -e "❌ ${RED}Xcode Command Line Tools${NC} - Development tools (MISSING)"
    echo "   Install: xcode-select --install"
    ERRORS=$((ERRORS + 1))
fi

# Project structure check
echo -e "\n${BLUE}Project Structure:${NC}"
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

check_file "$PROJECT_ROOT/Makefile" "Main build file"
check_file "$PROJECT_ROOT/tools/build/arch-arm64.mk" "ARM64 build config"
check_file "$PROJECT_ROOT/tools/build/arch-x86_64.mk" "x86-64 build config"
check_file "$PROJECT_ROOT/tools/create_image.py" "Image creation tool"
check_file "$PROJECT_ROOT/tools/test-vm.sh" "VM testing script"

# Check if scripts are executable
echo -e "\n${BLUE}Script Permissions:${NC}"
for script in "tools/test-vm.sh" "tools/debug.sh" "tools/check-prerequisites.sh"; do
    if [ -x "$PROJECT_ROOT/$script" ]; then
        echo -e "✅ ${GREEN}$script${NC} - Executable"
    else
        echo -e "⚠️  ${YELLOW}$script${NC} - Not executable"
        echo "   Fix: chmod +x $PROJECT_ROOT/$script"
        WARNINGS=$((WARNINGS + 1))
    fi
done

# Summary
echo -e "\n${BLUE}Summary:${NC}"
echo "========"

if [ $ERRORS -eq 0 ] && [ $WARNINGS -eq 0 ]; then
    echo -e "🎉 ${GREEN}All prerequisites satisfied!${NC}"
    echo "You're ready to start MiniOS development."
    echo ""
    echo "Next steps:"
    echo "  1. Build for ARM64: make"
    echo "  2. Build for x86-64: make ARCH=x86_64"
    echo "  3. Test in VM: make test"
elif [ $ERRORS -eq 0 ]; then
    echo -e "✅ ${GREEN}Core requirements satisfied${NC} (${WARNINGS} optional items missing)"
    echo "You can start development, but consider installing optional tools."
    echo ""
    echo "Quick start:"
    echo "  make        # This will show what's missing"
else
    echo -e "❌ ${RED}Missing ${ERRORS} required items${NC}"
    if [ $WARNINGS -gt 0 ]; then
        echo -e "⚠️  ${YELLOW}${WARNINGS} optional items missing${NC}"
    fi
    echo ""
    echo "Install missing requirements before proceeding."
    echo "See docs/PREREQUISITES.md for detailed instructions."
fi

# Auto-installer option
if [ $ERRORS -gt 0 ] && command -v brew >/dev/null 2>&1; then
    echo ""
    read -p "Would you like to auto-install missing Homebrew packages? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo "Installing missing packages..."
        
        # Install common packages
        brew install qemu nasm cdrtools || true
        
        # Try to install cross-compilers (may not be available)
        echo "Attempting to install cross-compilers..."
        brew install aarch64-elf-gcc x86_64-elf-gcc || {
            echo "Cross-compilers not available via Homebrew."
            echo "See docs/PREREQUISITES.md for alternative installation methods."
        }
        
        echo "Re-run this script to check updated status."
    fi
fi

exit $ERRORS
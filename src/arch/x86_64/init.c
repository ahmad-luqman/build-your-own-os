/*
 * x86-64 Architecture Initialization
 */

#include "kernel.h"

// VGA text mode buffer
#define VGA_MEMORY 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

// Serial port (COM1) for debug output
#define SERIAL_PORT 0x3F8

static uint16_t *vga_buffer = (uint16_t*)VGA_MEMORY;
static int vga_row = 0;
static int vga_col = 0;

// Port I/O functions
static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Initialize serial port for debug output
static void init_serial(void)
{
    outb(SERIAL_PORT + 1, 0x00);    // Disable interrupts
    outb(SERIAL_PORT + 3, 0x80);    // Enable DLAB
    outb(SERIAL_PORT + 0, 0x03);    // Set divisor low (38400 baud)
    outb(SERIAL_PORT + 1, 0x00);    // Set divisor high
    outb(SERIAL_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(SERIAL_PORT + 2, 0xC7);    // Enable FIFO, clear, 14-byte threshold
    outb(SERIAL_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

// Write character to serial port
static void serial_putchar(char c)
{
    // Wait for transmit buffer to be empty
    while ((inb(SERIAL_PORT + 5) & 0x20) == 0)
        ;
    
    outb(SERIAL_PORT, c);
}

// VGA text mode functions
static void vga_putchar(char c)
{
    if (c == '\n') {
        vga_col = 0;
        vga_row++;
    } else {
        uint16_t attr = 0x0F00;  // White on black
        vga_buffer[vga_row * VGA_WIDTH + vga_col] = attr | c;
        vga_col++;
    }
    
    if (vga_col >= VGA_WIDTH) {
        vga_col = 0;
        vga_row++;
    }
    
    if (vga_row >= VGA_HEIGHT) {
        // Simple scroll: clear screen and start over
        for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
            vga_buffer[i] = 0x0F20;  // Space character
        }
        vga_row = 0;
        vga_col = 0;
    }
}

void arch_init(void)
{
    // Initialize serial port for debug output
    init_serial();
    
    // Clear VGA screen
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = 0x0F20;  // White space on black background
    }
    vga_row = 0;
    vga_col = 0;
    
    arch_early_print("x86-64 architecture initialization\n");
}

void arch_early_print(const char *str)
{
    // Output to both VGA and serial for maximum compatibility
    while (*str) {
        vga_putchar(*str);
        serial_putchar(*str);
        str++;
    }
}

void arch_halt(void)
{
    arch_early_print("x86-64: Halting processor\n");
    
    // Disable interrupts and halt
    __asm__ volatile (
        "cli\n"
        "1: hlt\n"
        "jmp 1b\n"
        ::: "memory"
    );
    
    // Should never reach here
    __builtin_unreachable();
}
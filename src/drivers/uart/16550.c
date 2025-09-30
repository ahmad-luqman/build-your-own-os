/*
 * MiniOS x86-64 16550 UART Driver
 * Phase 4: Device Drivers & System Services
 */

#include "uart.h"
#include "device.h"
#include "driver.h"
#include "memory.h"
#include "kernel.h"

#ifdef ARCH_X86_64

// 16550 UART I/O ports (relative to base address)
#define UART_RBR            0   // Receive buffer register (read)
#define UART_THR            0   // Transmit holding register (write)
#define UART_DLL            0   // Divisor latch low (when DLAB=1)
#define UART_IER            1   // Interrupt enable register
#define UART_DLH            1   // Divisor latch high (when DLAB=1)
#define UART_IIR            2   // Interrupt identification register (read)
#define UART_FCR            2   // FIFO control register (write)
#define UART_LCR            3   // Line control register
#define UART_MCR            4   // Modem control register
#define UART_LSR            5   // Line status register
#define UART_MSR            6   // Modem status register
#define UART_SCR            7   // Scratch register

// Line status register bits
#define UART_LSR_DR         (1 << 0)    // Data ready
#define UART_LSR_OE         (1 << 1)    // Overrun error
#define UART_LSR_PE         (1 << 2)    // Parity error
#define UART_LSR_FE         (1 << 3)    // Frame error
#define UART_LSR_BI         (1 << 4)    // Break interrupt
#define UART_LSR_THRE       (1 << 5)    // Transmit holding register empty
#define UART_LSR_TEMT       (1 << 6)    // Transmitter empty
#define UART_LSR_IFIFO      (1 << 7)    // Impending FIFO error

// Line control register bits
#define UART_LCR_WLS_5      0x00        // 5 data bits
#define UART_LCR_WLS_6      0x01        // 6 data bits
#define UART_LCR_WLS_7      0x02        // 7 data bits
#define UART_LCR_WLS_8      0x03        // 8 data bits
#define UART_LCR_STB        (1 << 2)    // 2 stop bits
#define UART_LCR_PEN        (1 << 3)    // Parity enable
#define UART_LCR_EPS        (1 << 4)    // Even parity select
#define UART_LCR_STKP       (1 << 5)    // Stick parity
#define UART_LCR_SBRK       (1 << 6)    // Set break
#define UART_LCR_DLAB       (1 << 7)    // Divisor latch access bit

// Common COM port addresses
#define COM1_PORT           0x3F8
#define COM2_PORT           0x2F8
#define COM3_PORT           0x3E8
#define COM4_PORT           0x2E8

// 16550 UART device private data
struct uart_16550_device {
    uint16_t base_port;             // I/O port base address
    uint32_t baud_rate;             // Current baud rate
    uint32_t flags;                 // Configuration flags
    struct uart_stats stats;        // Statistics
    int tx_interrupts_enabled;      // Transmit interrupts enabled
    int rx_interrupts_enabled;      // Receive interrupts enabled
};

// Default 16550 configuration
#define UART_16550_DEFAULT_BAUD     115200
#define UART_16550_CLOCK            115200 * 16    // 1.8432 MHz

// I/O port access functions
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

// Helper functions for UART register access
static inline uint8_t uart_read_reg(struct uart_16550_device *uart, uint8_t reg) {
    return inb(uart->base_port + reg);
}

static inline void uart_write_reg(struct uart_16550_device *uart, uint8_t reg, uint8_t value) {
    outb(uart->base_port + reg, value);
}

// 16550 UART driver operations
int uart_16550_probe(struct device *device)
{
    if (!device) {
        return -1;
    }
    
    early_print("16550 UART: Probing device ");
    early_print(device->name);
    early_print("\n");
    
    // Check if device has valid I/O port base
    if (device->base_addr == 0) {
        // Use COM1 as default
        device->base_addr = COM1_PORT;
        early_print("16550 UART: Using COM1 (0x3F8)\n");
    }
    
    // Test if UART is present by checking scratch register
    uint16_t port = (uint16_t)device->base_addr;
    outb(port + UART_SCR, 0x55);
    if (inb(port + UART_SCR) != 0x55) {
        early_print("16550 UART: Hardware not detected\n");
        return -1;
    }
    
    outb(port + UART_SCR, 0xAA);
    if (inb(port + UART_SCR) != 0xAA) {
        early_print("16550 UART: Hardware test failed\n");
        return -1;
    }
    
    early_print("16550 UART: Probe successful\n");
    return 0;
}

int uart_16550_init(struct device *device)
{
    if (!device) {
        return -1;
    }
    
    early_print("16550 UART: Initializing device ");
    early_print(device->name);
    early_print("\n");
    
    // Allocate private data
    struct uart_16550_device *uart = memory_alloc(sizeof(struct uart_16550_device), MEMORY_ALIGN_4K);
    if (!uart) {
        early_print("16550 UART: Failed to allocate memory\n");
        return -1;
    }
    
    // Initialize UART structure
    uart->base_port = (uint16_t)device->base_addr;
    uart->baud_rate = UART_16550_DEFAULT_BAUD;
    uart->flags = UART_FLAG_8BIT | UART_FLAG_1_STOP | UART_FLAG_NO_PARITY;
    uart->tx_interrupts_enabled = 0;
    uart->rx_interrupts_enabled = 0;
    
    // Clear statistics
    uart->stats.bytes_transmitted = 0;
    uart->stats.bytes_received = 0;
    uart->stats.tx_errors = 0;
    uart->stats.rx_errors = 0;
    uart->stats.overrun_errors = 0;
    uart->stats.frame_errors = 0;
    uart->stats.parity_errors = 0;
    
    // Set device private data
    device_set_private_data(device, uart);
    
    // Configure UART hardware
    
    // 1. Disable all interrupts
    uart_write_reg(uart, UART_IER, 0x00);
    
    // 2. Enable DLAB (set baud rate divisor)
    uart_write_reg(uart, UART_LCR, UART_LCR_DLAB);
    
    // 3. Set baud rate divisor
    uint16_t divisor = UART_16550_CLOCK / uart->baud_rate;
    uart_write_reg(uart, UART_DLL, divisor & 0xFF);        // Low byte
    uart_write_reg(uart, UART_DLH, (divisor >> 8) & 0xFF); // High byte
    
    // 4. Configure line control (8 bits, 1 stop bit, no parity)
    uint8_t lcr = UART_LCR_WLS_8;  // 8 data bits
    uart_write_reg(uart, UART_LCR, lcr);
    
    // 5. Enable and clear FIFOs with 14-byte threshold
    uart_write_reg(uart, UART_FCR, 0xC7);
    
    // 6. Enable auxiliary output 2 (used as interrupt line for COM)
    uart_write_reg(uart, UART_MCR, 0x0B);
    
    // 7. Enable interrupts (optional, disabled for now)
    uart_write_reg(uart, UART_IER, 0x00);
    
    early_print("16550 UART: Initialization complete\n");
    early_print("16550 UART: Baud rate = ");
    
    // Convert baud rate to string for display
    char baud_str[16];
    int pos = 0;
    uint32_t baud = uart->baud_rate;
    if (baud == 0) {
        baud_str[pos++] = '0';
    } else {
        char temp[16];
        int temp_pos = 0;
        while (baud > 0) {
            temp[temp_pos++] = '0' + (baud % 10);
            baud /= 10;
        }
        while (temp_pos > 0) {
            baud_str[pos++] = temp[--temp_pos];
        }
    }
    baud_str[pos] = 0;
    early_print(baud_str);
    early_print(" bps\n");
    
    return 0;
}

int uart_16550_start(struct device *device)
{
    struct uart_16550_device *uart = device_get_private_data(device);
    if (!uart) {
        return -1;
    }
    
    early_print("16550 UART: Starting device\n");
    
    // UART is already configured during init
    device_set_state(device, DEVICE_STATE_ACTIVE);
    device_set_flags(device, DEVICE_FLAG_ACTIVE);
    
    return 0;
}

int uart_16550_stop(struct device *device)
{
    struct uart_16550_device *uart = device_get_private_data(device);
    if (!uart) {
        return -1;
    }
    
    early_print("16550 UART: Stopping device\n");
    
    // Disable interrupts
    uart_write_reg(uart, UART_IER, 0x00);
    
    device_clear_flags(device, DEVICE_FLAG_ACTIVE);
    
    return 0;
}

void uart_16550_cleanup(struct device *device)
{
    struct uart_16550_device *uart = device_get_private_data(device);
    if (uart) {
        // Disable interrupts
        uart_write_reg(uart, UART_IER, 0x00);
        
        // Free memory
        memory_free(uart);
        
        early_print("16550 UART: Device cleaned up\n");
    }
}

// Device operation implementations
int uart_16550_write(struct device *device, const void *buf, size_t len, off_t offset)
{
    (void)offset; // UART doesn't use offset
    
    struct uart_16550_device *uart = device_get_private_data(device);
    if (!uart || !buf) {
        return -1;
    }
    
    const char *data = (const char *)buf;
    size_t written = 0;
    
    for (size_t i = 0; i < len; i++) {
        // Wait for transmit holding register to be empty
        while (!(uart_read_reg(uart, UART_LSR) & UART_LSR_THRE)) {
            // Wait
        }
        
        // Write character to transmit holding register
        uart_write_reg(uart, UART_THR, data[i]);
        written++;
        uart->stats.bytes_transmitted++;
    }
    
    return written;
}

int uart_16550_read(struct device *device, void *buf, size_t len, off_t offset)
{
    (void)offset; // UART doesn't use offset
    
    struct uart_16550_device *uart = device_get_private_data(device);
    if (!uart || !buf) {
        return -1;
    }
    
    char *data = (char *)buf;
    size_t read_count = 0;
    
    for (size_t i = 0; i < len; i++) {
        uint8_t lsr = uart_read_reg(uart, UART_LSR);
        
        // Check if data is available
        if (!(lsr & UART_LSR_DR)) {
            break; // No more data available
        }
        
        // Check for errors
        if (lsr & (UART_LSR_OE | UART_LSR_PE | UART_LSR_FE | UART_LSR_BI)) {
            if (lsr & UART_LSR_OE) uart->stats.overrun_errors++;
            if (lsr & UART_LSR_PE) uart->stats.parity_errors++;
            if (lsr & UART_LSR_FE) uart->stats.frame_errors++;
            uart->stats.rx_errors++;
            
            // Read and discard the erroneous data
            uart_read_reg(uart, UART_RBR);
            continue;
        }
        
        // Read character from receive buffer register
        data[i] = uart_read_reg(uart, UART_RBR);
        read_count++;
        uart->stats.bytes_received++;
    }
    
    return read_count;
}

int uart_16550_ioctl(struct device *device, unsigned int cmd, unsigned long arg)
{
    struct uart_16550_device *uart = device_get_private_data(device);
    if (!uart) {
        return -1;
    }
    
    // Basic IOCTL commands (can be extended)
    switch (cmd) {
        case 0x1000: // Get baud rate
            return uart->baud_rate;
        case 0x1001: // Set baud rate
            // Would reconfigure UART with new baud rate
            return 0;
        case 0x1002: // Get statistics
            if (arg) {
                struct uart_stats *stats = (struct uart_stats *)arg;
                *stats = uart->stats;
                return 0;
            }
            return -1;
        default:
            return -1;
    }
}

// UART API implementations
int uart_16550_putc(struct device *device, char c)
{
    return uart_16550_write(device, &c, 1, 0) == 1 ? 0 : -1;
}

int uart_16550_getc(struct device *device)
{
    char c;
    int result = uart_16550_read(device, &c, 1, 0);
    return result == 1 ? (unsigned char)c : -1;
}

int uart_16550_puts(struct device *device, const char *str)
{
    if (!str) {
        return -1;
    }
    
    // Calculate string length
    size_t len = 0;
    while (str[len]) len++;
    
    return uart_16550_write(device, str, len, 0);
}

int uart_16550_tx_ready(struct device *device)
{
    struct uart_16550_device *uart = device_get_private_data(device);
    if (!uart) {
        return -1;
    }
    
    return !!(uart_read_reg(uart, UART_LSR) & UART_LSR_THRE);
}

int uart_16550_rx_ready(struct device *device)
{
    struct uart_16550_device *uart = device_get_private_data(device);
    if (!uart) {
        return -1;
    }
    
    return !!(uart_read_reg(uart, UART_LSR) & UART_LSR_DR);
}

// Device driver structure
static struct device_id uart_16550_ids[] = {
    { 0, 0, NULL, DEVICE_TYPE_UART },
    DEVICE_ID_TABLE_END
};

struct device_driver uart_16550_driver = {
    .name = "16550-uart",
    .version = "1.0",
    .type = DRIVER_TYPE_UART,
    .flags = DRIVER_FLAG_IRQ_HANDLER,
    .id_table = uart_16550_ids,
    .num_ids = 1,
    .probe = uart_16550_probe,
    .init = uart_16550_init,
    .start = uart_16550_start,
    .stop = uart_16550_stop,
    .cleanup = uart_16550_cleanup,
    .read = uart_16550_read,
    .write = uart_16550_write,
    .ioctl = uart_16550_ioctl,
    .author = "MiniOS",
    .description = "x86-64 16550 UART Driver",
    .license = "GPL",
};

// Register 16550 UART driver
int uart_16550_driver_register(void)
{
    return driver_register(&uart_16550_driver);
}

#endif // ARCH_X86_64
/*
 * MiniOS ARM64 PL011 UART Driver
 * Phase 4: Device Drivers & System Services
 */

#include "uart.h"
#include "device.h"
#include "driver.h"
#include "memory.h"
#include "kernel.h"

#ifdef ARCH_ARM64

// PL011 UART registers (offsets from base address)
#define PL011_UARTDR        0x000   // Data register
#define PL011_UARTRSR       0x004   // Receive status register
#define PL011_UARTFR        0x018   // Flag register
#define PL011_UARTILPR      0x020   // IrDA low-power counter register
#define PL011_UARTIBRD      0x024   // Integer baud rate divisor
#define PL011_UARTFBRD      0x028   // Fractional baud rate divisor
#define PL011_UARTLCR_H     0x02C   // Line control register
#define PL011_UARTCR        0x030   // Control register
#define PL011_UARTIFLS      0x034   // Interrupt FIFO level select register
#define PL011_UARTIMSC      0x038   // Interrupt mask set/clear register
#define PL011_UARTRIS       0x03C   // Raw interrupt status register
#define PL011_UARTMIS       0x040   // Masked interrupt status register
#define PL011_UARTICR       0x044   // Interrupt clear register

// PL011 flag register bits
#define PL011_FR_TXFE       (1 << 7)    // Transmit FIFO empty
#define PL011_FR_RXFF       (1 << 6)    // Receive FIFO full
#define PL011_FR_TXFF       (1 << 5)    // Transmit FIFO full
#define PL011_FR_RXFE       (1 << 4)    // Receive FIFO empty
#define PL011_FR_BUSY       (1 << 3)    // UART busy

// PL011 control register bits
#define PL011_CR_CTSEN      (1 << 15)   // CTS hardware flow control enable
#define PL011_CR_RTSEN      (1 << 14)   // RTS hardware flow control enable
#define PL011_CR_RTS        (1 << 11)   // Request to send
#define PL011_CR_RXE        (1 << 9)    // Receive enable
#define PL011_CR_TXE        (1 << 8)    // Transmit enable
#define PL011_CR_LBE        (1 << 7)    // Loopback enable
#define PL011_CR_UARTEN     (1 << 0)    // UART enable

// PL011 UART device private data
struct pl011_uart_device {
    volatile uint32_t *base;        // MMIO base address
    uint32_t baud_rate;             // Current baud rate
    uint32_t clock_rate;            // Input clock rate (typically 24MHz)
    uint32_t flags;                 // Configuration flags
    struct uart_stats stats;        // Statistics
    int tx_interrupts_enabled;      // Transmit interrupts enabled
    int rx_interrupts_enabled;      // Receive interrupts enabled
};

// Default PL011 configuration
#define PL011_DEFAULT_BAUD      115200
#define PL011_DEFAULT_CLOCK     24000000    // 24 MHz
#define PL011_DEFAULT_BASE      0x09000000  // Typical QEMU address

// Helper functions for MMIO access
static inline uint32_t pl011_read(struct pl011_uart_device *uart, uint32_t offset) {
    return uart->base[offset / 4];
}

static inline void pl011_write(struct pl011_uart_device *uart, uint32_t offset, uint32_t value) {
    uart->base[offset / 4] = value;
}

// PL011 UART driver operations
int pl011_uart_probe(struct device *device)
{
    if (!device) {
        return -1;
    }
    
    early_print("PL011 UART: Probing device ");
    early_print(device->name);
    early_print("\n");
    
    // Check if device has valid MMIO base address
    if (device->base_addr == 0) {
        // Use default address if not specified
        device->base_addr = PL011_DEFAULT_BASE;
        early_print("PL011 UART: Using default base address\n");
    }
    
    early_print("PL011 UART: Probe successful\n");
    return 0;
}

int pl011_uart_init(struct device *device)
{
    if (!device) {
        return -1;
    }
    
    early_print("PL011 UART: Initializing device ");
    early_print(device->name);
    early_print("\n");
    
    // Allocate private data
    struct pl011_uart_device *uart = memory_alloc(sizeof(struct pl011_uart_device), MEMORY_ALIGN_4K);
    if (!uart) {
        early_print("PL011 UART: Failed to allocate memory\n");
        return -1;
    }
    
    // Initialize UART structure
    uart->base = (volatile uint32_t *)device->base_addr;
    uart->baud_rate = PL011_DEFAULT_BAUD;
    uart->clock_rate = PL011_DEFAULT_CLOCK;
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
    
    // 1. Disable UART
    pl011_write(uart, PL011_UARTCR, 0);
    
    // 2. Wait for end of transmission or reception of current character
    while (pl011_read(uart, PL011_UARTFR) & PL011_FR_BUSY) {
        // Wait
    }
    
    // 3. Flush transmit FIFO by setting FEN bit to 0 in line control register
    pl011_write(uart, PL011_UARTLCR_H, 0);
    
    // 4. Program baud rate divisors
    // Baud rate divisor = FUARTCLK / (16 * baud_rate)
    uint32_t divisor = uart->clock_rate / (16 * uart->baud_rate);
    uint32_t remainder = uart->clock_rate % (16 * uart->baud_rate);
    uint32_t fractional = (remainder * 64 + uart->baud_rate / 2) / uart->baud_rate;
    
    pl011_write(uart, PL011_UARTIBRD, divisor);
    pl011_write(uart, PL011_UARTFBRD, fractional);
    
    // 5. Program line control register (8 bits, 1 stop bit, no parity, FIFOs enabled)
    uint32_t lcr = 0;
    lcr |= (3 << 5);    // 8 data bits (WLEN = 11)
    lcr |= (1 << 4);    // Enable FIFOs (FEN = 1)
    pl011_write(uart, PL011_UARTLCR_H, lcr);
    
    // 6. Disable all interrupts initially
    pl011_write(uart, PL011_UARTIMSC, 0);
    
    // 7. Clear all interrupt flags
    pl011_write(uart, PL011_UARTICR, 0x7FF);
    
    // 8. Enable UART, transmit and receive
    uint32_t cr = PL011_CR_UARTEN | PL011_CR_TXE | PL011_CR_RXE;
    pl011_write(uart, PL011_UARTCR, cr);
    
    early_print("PL011 UART: Initialization complete\n");
    early_print("PL011 UART: Baud rate = ");
    
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

int pl011_uart_start(struct device *device)
{
    struct pl011_uart_device *uart = device_get_private_data(device);
    if (!uart) {
        return -1;
    }
    
    early_print("PL011 UART: Starting device\n");
    
    // UART is already enabled during init
    device_set_state(device, DEVICE_STATE_ACTIVE);
    device_set_flags(device, DEVICE_FLAG_ACTIVE);
    
    return 0;
}

int pl011_uart_stop(struct device *device)
{
    struct pl011_uart_device *uart = device_get_private_data(device);
    if (!uart) {
        return -1;
    }
    
    early_print("PL011 UART: Stopping device\n");
    
    // Disable UART
    pl011_write(uart, PL011_UARTCR, 0);
    
    device_clear_flags(device, DEVICE_FLAG_ACTIVE);
    
    return 0;
}

void pl011_uart_cleanup(struct device *device)
{
    struct pl011_uart_device *uart = device_get_private_data(device);
    if (uart) {
        // Disable UART
        pl011_write(uart, PL011_UARTCR, 0);
        
        // Free memory
        memory_free(uart);
        
        early_print("PL011 UART: Device cleaned up\n");
    }
}

// Device operation implementations
int pl011_uart_write(struct device *device, const void *buf, size_t len, off_t offset)
{
    (void)offset; // UART doesn't use offset
    
    struct pl011_uart_device *uart = device_get_private_data(device);
    if (!uart || !buf) {
        return -1;
    }
    
    const char *data = (const char *)buf;
    size_t written = 0;
    
    for (size_t i = 0; i < len; i++) {
        // Wait for transmit FIFO to have space
        while (pl011_read(uart, PL011_UARTFR) & PL011_FR_TXFF) {
            // Wait
        }
        
        // Write character to data register
        pl011_write(uart, PL011_UARTDR, data[i]);
        written++;
        uart->stats.bytes_transmitted++;
    }
    
    return written;
}

int pl011_uart_read(struct device *device, void *buf, size_t len, off_t offset)
{
    (void)offset; // UART doesn't use offset
    
    struct pl011_uart_device *uart = device_get_private_data(device);
    if (!uart || !buf) {
        return -1;
    }
    
    char *data = (char *)buf;
    size_t read_count = 0;
    
    for (size_t i = 0; i < len; i++) {
        // Check if data is available
        if (pl011_read(uart, PL011_UARTFR) & PL011_FR_RXFE) {
            break; // No more data available
        }
        
        // Read character from data register
        uint32_t dr = pl011_read(uart, PL011_UARTDR);
        
        // Check for errors
        if (dr & 0xF00) {
            if (dr & (1 << 11)) uart->stats.overrun_errors++;
            if (dr & (1 << 10)) uart->stats.frame_errors++;
            if (dr & (1 << 9)) uart->stats.parity_errors++;
            uart->stats.rx_errors++;
            continue;
        }
        
        data[i] = (char)(dr & 0xFF);
        read_count++;
        uart->stats.bytes_received++;
    }
    
    return read_count;
}

int pl011_uart_ioctl(struct device *device, unsigned int cmd, unsigned long arg)
{
    struct pl011_uart_device *uart = device_get_private_data(device);
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
int pl011_uart_putc(struct device *device, char c)
{
    return pl011_uart_write(device, &c, 1, 0) == 1 ? 0 : -1;
}

int pl011_uart_getc(struct device *device)
{
    char c;
    int result = pl011_uart_read(device, &c, 1, 0);
    return result == 1 ? (unsigned char)c : -1;
}

int pl011_uart_puts(struct device *device, const char *str)
{
    if (!str) {
        return -1;
    }
    
    // Calculate string length
    size_t len = 0;
    while (str[len]) len++;
    
    return pl011_uart_write(device, str, len, 0);
}

int pl011_uart_tx_ready(struct device *device)
{
    struct pl011_uart_device *uart = device_get_private_data(device);
    if (!uart) {
        return -1;
    }
    
    return !(pl011_read(uart, PL011_UARTFR) & PL011_FR_TXFF);
}

int pl011_uart_rx_ready(struct device *device)
{
    struct pl011_uart_device *uart = device_get_private_data(device);
    if (!uart) {
        return -1;
    }
    
    return !(pl011_read(uart, PL011_UARTFR) & PL011_FR_RXFE);
}

// Device driver structure
static struct device_id pl011_uart_ids[] = {
    { 0, 0, "arm,pl011", DEVICE_TYPE_UART },
    DEVICE_ID_TABLE_END
};

struct device_driver pl011_uart_driver = {
    .name = "pl011-uart",
    .version = "1.0",
    .type = DRIVER_TYPE_UART,
    .flags = DRIVER_FLAG_IRQ_HANDLER,
    .id_table = pl011_uart_ids,
    .num_ids = 1,
    .probe = pl011_uart_probe,
    .init = pl011_uart_init,
    .start = pl011_uart_start,
    .stop = pl011_uart_stop,
    .cleanup = pl011_uart_cleanup,
    .read = pl011_uart_read,
    .write = pl011_uart_write,
    .ioctl = pl011_uart_ioctl,
    .author = "MiniOS",
    .description = "ARM64 PL011 UART Driver",
    .license = "GPL",
};

// Register PL011 UART driver
int pl011_uart_driver_register(void)
{
    return driver_register(&pl011_uart_driver);
}

#endif // ARCH_ARM64
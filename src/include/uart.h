/*
 * MiniOS UART/Serial Communication Interface
 * Phase 4: Device Drivers & System Services
 */

#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stddef.h>

// Forward declarations
struct device;

// UART configuration flags
#define UART_FLAG_8BIT          (0 << 0)    // 8 data bits
#define UART_FLAG_7BIT          (1 << 0)    // 7 data bits
#define UART_FLAG_6BIT          (2 << 0)    // 6 data bits
#define UART_FLAG_5BIT          (3 << 0)    // 5 data bits

#define UART_FLAG_1_STOP        (0 << 2)    // 1 stop bit
#define UART_FLAG_2_STOP        (1 << 2)    // 2 stop bits

#define UART_FLAG_NO_PARITY     (0 << 3)    // No parity
#define UART_FLAG_EVEN_PARITY   (1 << 3)    // Even parity
#define UART_FLAG_ODD_PARITY    (2 << 3)    // Odd parity

#define UART_FLAG_NO_FLOW       (0 << 5)    // No flow control
#define UART_FLAG_RTS_CTS       (1 << 5)    // RTS/CTS flow control

// Common baud rates
#define UART_BAUD_9600          9600
#define UART_BAUD_19200         19200
#define UART_BAUD_38400         38400
#define UART_BAUD_57600         57600
#define UART_BAUD_115200        115200

// UART device configuration
struct uart_config {
    uint32_t baud_rate;         // Baud rate (bits per second)
    uint32_t flags;             // Configuration flags
    uint32_t tx_buffer_size;    // Transmit buffer size
    uint32_t rx_buffer_size;    // Receive buffer size
};

// UART device statistics
struct uart_stats {
    uint64_t bytes_transmitted;
    uint64_t bytes_received;
    uint32_t tx_errors;
    uint32_t rx_errors;
    uint32_t overrun_errors;
    uint32_t frame_errors;
    uint32_t parity_errors;
};

// UART subsystem API

/**
 * Initialize UART subsystem
 * @return 0 on success, negative on error
 */
int uart_init(void);

/**
 * Find UART device by name
 * @param name UART device name (e.g., "uart0", "com1")
 * @return Device pointer if found, NULL otherwise
 */
struct device *uart_find_device(const char *name);

/**
 * Configure UART device
 * @param device UART device
 * @param config Configuration parameters
 * @return 0 on success, negative on error
 */
int uart_configure(struct device *device, const struct uart_config *config);

/**
 * Write a single character to UART
 * @param device UART device
 * @param c Character to write
 * @return 0 on success, negative on error
 */
int uart_putc(struct device *device, char c);

/**
 * Read a single character from UART
 * @param device UART device
 * @return Character (0-255) or negative on error
 */
int uart_getc(struct device *device);

/**
 * Write a string to UART
 * @param device UART device
 * @param str String to write
 * @return Number of characters written, negative on error
 */
int uart_puts(struct device *device, const char *str);

/**
 * Write data to UART
 * @param device UART device
 * @param buf Data buffer
 * @param len Number of bytes to write
 * @return Number of bytes written, negative on error
 */
int uart_write(struct device *device, const void *buf, size_t len);

/**
 * Read data from UART
 * @param device UART device
 * @param buf Buffer to store data
 * @param len Number of bytes to read
 * @return Number of bytes read, negative on error
 */
int uart_read(struct device *device, void *buf, size_t len);

/**
 * Check if UART transmit is ready
 * @param device UART device
 * @return 1 if ready, 0 if not ready, negative on error
 */
int uart_tx_ready(struct device *device);

/**
 * Check if UART receive data is available
 * @param device UART device
 * @return 1 if data available, 0 if not, negative on error
 */
int uart_rx_ready(struct device *device);

/**
 * Get UART statistics
 * @param device UART device
 * @param stats Pointer to stats structure to fill
 * @return 0 on success, negative on error
 */
int uart_get_stats(struct device *device, struct uart_stats *stats);

/**
 * Enable UART interrupts
 * @param device UART device
 * @param tx_enable Enable transmit interrupts
 * @param rx_enable Enable receive interrupts
 * @return 0 on success, negative on error
 */
int uart_enable_interrupts(struct device *device, int tx_enable, int rx_enable);

/**
 * Disable UART interrupts
 * @param device UART device
 * @return 0 on success, negative on error
 */
int uart_disable_interrupts(struct device *device);

// Enhanced early_print using UART drivers
/**
 * Initialize enhanced early print with UART backend
 * @return 0 on success, negative on error
 */
int enhanced_early_print_init(void);

/**
 * Enhanced early print using UART driver
 * @param str String to print
 */
void enhanced_early_print(const char *str);

// Architecture-specific UART functions

/**
 * Initialize architecture-specific UART hardware
 * @return 0 on success, negative on error
 */
int arch_uart_init(void);

// UART device driver interface (for device framework integration)

/**
 * Register UART device drivers
 * @return 0 on success, negative on error
 */
int uart_drivers_register(void);

#ifdef ARCH_ARM64
/**
 * ARM64 PL011 UART driver
 */

// PL011 UART registers (offsets)
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

/**
 * PL011 UART device probe
 * @param device UART device
 * @return 0 on success, negative on error
 */
int pl011_uart_probe(struct device *device);

/**
 * PL011 UART device init
 * @param device UART device
 * @return 0 on success, negative on error
 */
int pl011_uart_init(struct device *device);

/**
 * Register PL011 UART driver
 * @return 0 on success, negative on error
 */
int pl011_uart_driver_register(void);

#endif // ARCH_ARM64

#ifdef ARCH_X86_64
/**
 * x86-64 16550 UART driver
 */

// 16550 UART I/O ports (relative to base)
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

/**
 * 16550 UART device probe
 * @param device UART device
 * @return 0 on success, negative on error
 */
int uart_16550_probe(struct device *device);

/**
 * 16550 UART device init
 * @param device UART device
 * @return 0 on success, negative on error
 */
int uart_16550_init(struct device *device);

/**
 * Register 16550 UART driver
 * @return 0 on success, negative on error
 */
int uart_16550_driver_register(void);

#endif // ARCH_X86_64

#endif // UART_H
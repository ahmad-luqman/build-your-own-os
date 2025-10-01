/*
 * MiniOS UART Subsystem Implementation  
 * Phase 4: Device Drivers & System Services
 */

#include "uart.h"
#include "device.h"
#include "driver.h"
#include "memory.h"
#include "kernel.h"

// UART subsystem state
static int uart_subsystem_initialized = 0;
static struct device *primary_uart_device = NULL;

// Function declarations for architecture-specific drivers
#ifdef ARCH_ARM64
extern int pl011_uart_driver_register(void);
extern int pl011_uart_putc(struct device *device, char c);
extern int pl011_uart_getc(struct device *device);
extern int pl011_uart_puts(struct device *device, const char *str);
extern int pl011_uart_tx_ready(struct device *device);
extern int pl011_uart_rx_ready(struct device *device);
#endif

#ifdef ARCH_X86_64
extern int uart_16550_driver_register(void);
extern int uart_16550_putc(struct device *device, char c);
extern int uart_16550_getc(struct device *device);
extern int uart_16550_puts(struct device *device, const char *str);
extern int uart_16550_tx_ready(struct device *device);
extern int uart_16550_rx_ready(struct device *device);
#endif

int uart_init(void)
{
    early_print("Initializing UART subsystem...\n");
    
    // Register architecture-specific UART drivers
#ifdef ARCH_ARM64
    if (pl011_uart_driver_register() < 0) {
        early_print("Failed to register PL011 UART driver\n");
        return -1;
    }
    early_print("PL011 UART driver registered\n");
#endif

#ifdef ARCH_X86_64
    if (uart_16550_driver_register() < 0) {
        early_print("Failed to register 16550 UART driver\n");
        return -1;
    }
    early_print("16550 UART driver registered\n");
#endif

    uart_subsystem_initialized = 1;
    
    // Try to find and initialize a UART device
    struct device *uart_dev = device_find_by_type(DEVICE_TYPE_UART);
    if (!uart_dev) {
        // Skip UART device creation for debugging
        early_print("No UART device found, skipping UART device initialization\n");
        uart_subsystem_initialized = 1;
        return 0;
    }
    
    // Initialize the UART device
    if (device_initialize(uart_dev) < 0) {
        early_print("Failed to initialize UART device\n");
        return -1;
    }
    
    // Start the UART device
    if (driver_start_device(uart_dev) < 0) {
        early_print("Failed to start UART device\n");
        return -1;
    }
    
    primary_uart_device = uart_dev;
    
    early_print("UART subsystem initialized successfully\n");
    return 0;
}

struct device *uart_find_device(const char *name)
{
    if (!uart_subsystem_initialized || !name) {
        return NULL;
    }
    
    return device_find_by_name(name);
}

int uart_configure(struct device *device, const struct uart_config *config)
{
    if (!device || !config || !uart_subsystem_initialized) {
        return -1;
    }
    
    // Use ioctl to configure UART
    // This is a simplified implementation - real UART configuration
    // would involve setting baud rate, parity, stop bits, etc.
    return driver_ioctl(device, 0x1001, config->baud_rate);
}

int uart_putc(struct device *device, char c)
{
    if (!device || !uart_subsystem_initialized) {
        return -1;
    }
    
#ifdef ARCH_ARM64
    return pl011_uart_putc(device, c);
#elif defined(ARCH_X86_64)
    return uart_16550_putc(device, c);
#else
    return -1;
#endif
}

int uart_getc(struct device *device)
{
    if (!device || !uart_subsystem_initialized) {
        return -1;
    }
    
#ifdef ARCH_ARM64
    return pl011_uart_getc(device);
#elif defined(ARCH_X86_64)
    return uart_16550_getc(device);
#else
    return -1;
#endif
}

int uart_puts(struct device *device, const char *str)
{
    if (!device || !str || !uart_subsystem_initialized) {
        return -1;
    }
    
#ifdef ARCH_ARM64
    return pl011_uart_puts(device, str);
#elif defined(ARCH_X86_64)
    return uart_16550_puts(device, str);
#else
    return -1;
#endif
}

int uart_write(struct device *device, const void *buf, size_t len)
{
    if (!device || !buf || !uart_subsystem_initialized) {
        return -1;
    }
    
    return driver_write(device, buf, len, 0);
}

int uart_read(struct device *device, void *buf, size_t len)
{
    if (!device || !buf || !uart_subsystem_initialized) {
        return -1;
    }
    
    return driver_read(device, buf, len, 0);
}

int uart_tx_ready(struct device *device)
{
    if (!device || !uart_subsystem_initialized) {
        return -1;
    }
    
#ifdef ARCH_ARM64
    return pl011_uart_tx_ready(device);
#elif defined(ARCH_X86_64)
    return uart_16550_tx_ready(device);
#else
    return -1;
#endif
}

int uart_rx_ready(struct device *device)
{
    if (!device || !uart_subsystem_initialized) {
        return -1;
    }
    
#ifdef ARCH_ARM64
    return pl011_uart_rx_ready(device);
#elif defined(ARCH_X86_64)
    return uart_16550_rx_ready(device);
#else
    return -1;
#endif
}

int uart_get_stats(struct device *device, struct uart_stats *stats)
{
    if (!device || !stats || !uart_subsystem_initialized) {
        return -1;
    }
    
    return driver_ioctl(device, 0x1002, (unsigned long)stats);
}

int uart_enable_interrupts(struct device *device, int tx_enable, int rx_enable)
{
    if (!device || !uart_subsystem_initialized) {
        return -1;
    }
    
    // This would be implemented through driver-specific ioctl
    // For now, return success
    (void)tx_enable;
    (void)rx_enable;
    return 0;
}

int uart_disable_interrupts(struct device *device)
{
    if (!device || !uart_subsystem_initialized) {
        return -1;
    }
    
    // This would be implemented through driver-specific ioctl
    // For now, return success
    return 0;
}

// Enhanced early_print implementation
int enhanced_early_print_init(void)
{
    if (!uart_subsystem_initialized) {
        return uart_init();
    }
    return 0;
}

void enhanced_early_print(const char *str)
{
    if (!str || !uart_subsystem_initialized || !primary_uart_device) {
        // Fallback to architecture-specific early_print
        arch_early_print(str);
        return;
    }
    
    // Use UART driver for output
    uart_puts(primary_uart_device, str);
}

// Architecture-specific functions (weak implementations)
int __attribute__((weak)) arch_uart_init(void)
{
    early_print("Warning: Using weak arch_uart_init\n");
    return 0;
}

int uart_drivers_register(void)
{
    // This function registers all available UART drivers
    int result = 0;
    
#ifdef ARCH_ARM64
    result += pl011_uart_driver_register();
#endif

#ifdef ARCH_X86_64
    result += uart_16550_driver_register();
#endif
    
    return result;
}

// UART subsystem testing and debugging functions
void uart_test_output(void)
{
    if (!uart_subsystem_initialized || !primary_uart_device) {
        early_print("UART test: No UART device available\n");
        return;
    }
    
    early_print("UART test: Testing UART output...\n");
    
    uart_puts(primary_uart_device, "Hello from UART driver!\n");
    uart_puts(primary_uart_device, "UART subsystem is working correctly.\n");
    
    // Test individual character output
    uart_putc(primary_uart_device, 'A');
    uart_putc(primary_uart_device, 'B');
    uart_putc(primary_uart_device, 'C');
    uart_putc(primary_uart_device, '\n');
    
    early_print("UART test: Output test complete\n");
}

void uart_show_devices(void)
{
    if (!uart_subsystem_initialized) {
        early_print("UART subsystem not initialized\n");
        return;
    }
    
    early_print("=== UART Devices ===\n");
    
    // Find all UART devices
    struct device *devices[4];
    int count = device_get_by_type(DEVICE_TYPE_UART, devices, 4);
    
    for (int i = 0; i < count; i++) {
        early_print("UART Device: ");
        early_print(devices[i]->name);
        
        if (devices[i] == primary_uart_device) {
            early_print(" (primary)");
        }
        
        early_print(", State: ");
        switch (devices[i]->state) {
            case DEVICE_STATE_INITIALIZED: early_print("INITIALIZED"); break;
            case DEVICE_STATE_ACTIVE: early_print("ACTIVE"); break;
            case DEVICE_STATE_ERROR: early_print("ERROR"); break;
            default: early_print("UNKNOWN"); break;
        }
        
        if (devices[i]->driver) {
            early_print(", Driver: ");
            early_print(devices[i]->driver->name);
        }
        
        early_print("\n");
    }
    
    // Convert device count to string
    char count_str[16];
    int pos = 0;
    if (count == 0) {
        count_str[pos++] = '0';
    } else {
        char temp[16];
        int temp_pos = 0;
        while (count > 0) {
            temp[temp_pos++] = '0' + (count % 10);
            count /= 10;
        }
        while (temp_pos > 0) {
            count_str[pos++] = temp[--temp_pos];
        }
    }
    count_str[pos] = 0;
    
    early_print("Total UART devices: ");
    early_print(count_str);
    early_print("\n");
}
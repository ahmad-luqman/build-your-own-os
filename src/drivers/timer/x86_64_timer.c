/*
 * MiniOS x86-64 Timer Driver (PIT - Programmable Interval Timer)
 * Phase 4: Device Drivers & System Services
 */

#include "timer.h"
#include "device.h"
#include "driver.h"
#include "memory.h"
#include "kernel.h"

#ifdef ARCH_X86_64

// PIT (8254) I/O ports
#define PIT_CHANNEL0_DATA       0x40    // Channel 0 data port
#define PIT_CHANNEL1_DATA       0x41    // Channel 1 data port
#define PIT_CHANNEL2_DATA       0x42    // Channel 2 data port
#define PIT_COMMAND             0x43    // Command/Mode register

// PIT command register bits
#define PIT_CMD_CHANNEL0        0x00    // Select channel 0
#define PIT_CMD_ACCESS_LOHI     0x30    // Access mode: lo/hi byte
#define PIT_CMD_MODE2           0x04    // Mode 2: rate generator
#define PIT_CMD_BINARY          0x00    // Binary mode

// PIT constants
#define PIT_FREQUENCY           1193182 // PIT base frequency in Hz
#define PIT_DEFAULT_HZ          100     // Default 100 Hz (10ms intervals)
#define PIT_MAX_COUNT           65535   // Maximum 16-bit count value

// x86-64 timer device private data
struct x86_64_timer_device {
    uint32_t frequency;         // Current timer frequency
    uint32_t divisor;           // Current PIT divisor
    uint64_t ticks;             // Timer tick counter
    uint32_t enabled;           // Timer enabled flag
    uint32_t interrupt_enabled; // Interrupt enabled flag
};

// Global timer state
static struct x86_64_timer_device *timer_device = NULL;
static uint64_t system_ticks = 0;
static uint64_t boot_time_us = 0;

// I/O port access functions
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

// x86-64 PIT timer driver operations
int x86_64_timer_probe(struct device *device)
{
    if (!device) {
        return -1;
    }
    
    early_print("x86-64 timer: Probing PIT (8254)\n");
    
    // Check if we already have a timer device
    if (timer_device) {
        return -1; // Only one timer device supported
    }
    
    // PIT is always available on x86-64, no specific probe needed
    early_print("x86-64 timer: PIT found\n");
    
    return 0; // Probe successful
}

int x86_64_timer_init(struct device *device)
{
    if (!device) {
        return -1;
    }
    
    early_print("x86-64 timer: Initializing PIT\n");
    
    // Allocate private data
    struct x86_64_timer_device *timer = memory_alloc(sizeof(struct x86_64_timer_device), MEMORY_ALIGN_4K);
    if (!timer) {
        early_print("x86-64 timer: Failed to allocate memory\n");
        return -1;
    }
    
    // Initialize timer device structure
    timer->frequency = PIT_DEFAULT_HZ;
    timer->divisor = PIT_FREQUENCY / PIT_DEFAULT_HZ;
    timer->ticks = 0;
    timer->enabled = 0;
    timer->interrupt_enabled = 0;
    
    // Set device private data
    device_set_private_data(device, timer);
    timer_device = timer;
    
    // Configure PIT for mode 2 (rate generator)
    outb(PIT_COMMAND, PIT_CMD_CHANNEL0 | PIT_CMD_ACCESS_LOHI | PIT_CMD_MODE2 | PIT_CMD_BINARY);
    
    // Set initial divisor
    outb(PIT_CHANNEL0_DATA, timer->divisor & 0xFF);         // Low byte
    outb(PIT_CHANNEL0_DATA, (timer->divisor >> 8) & 0xFF); // High byte
    
    early_print("x86-64 timer: PIT initialized with frequency ");
    // Convert frequency to string for display
    char freq_str[16];
    int pos = 0;
    uint32_t f = timer->frequency;
    if (f == 0) {
        freq_str[pos++] = '0';
    } else {
        char temp[16];
        int temp_pos = 0;
        while (f > 0) {
            temp[temp_pos++] = '0' + (f % 10);
            f /= 10;
        }
        while (temp_pos > 0) {
            freq_str[pos++] = temp[--temp_pos];
        }
    }
    freq_str[pos] = 0;
    early_print(freq_str);
    early_print(" Hz\n");
    
    return 0;
}

int x86_64_timer_start(struct device *device)
{
    struct x86_64_timer_device *timer = device_get_private_data(device);
    if (!timer) {
        return -1;
    }
    
    early_print("x86-64 timer: Starting PIT\n");
    
    timer->enabled = 1;
    timer->ticks = 0;
    
    // Initialize boot time
    boot_time_us = 0;
    system_ticks = 0;
    
    return 0;
}

int x86_64_timer_stop(struct device *device)
{
    struct x86_64_timer_device *timer = device_get_private_data(device);
    if (!timer) {
        return -1;
    }
    
    early_print("x86-64 timer: Stopping PIT\n");
    
    timer->enabled = 0;
    timer->interrupt_enabled = 0;
    
    return 0;
}

void x86_64_timer_cleanup(struct device *device)
{
    struct x86_64_timer_device *timer = device_get_private_data(device);
    if (timer) {
        timer->enabled = 0;
        timer->interrupt_enabled = 0;
        
        // Free memory
        memory_free(timer);
        timer_device = NULL;
        
        early_print("x86-64 timer: PIT cleaned up\n");
    }
}

// Architecture-specific timer functions implementation
int arch_timer_init(void)
{
    if (!timer_device) {
        early_print("x86-64 timer: Timer device not initialized\n");
        return -1;
    }
    
    return 0;
}

uint64_t arch_timer_get_count(void)
{
    return timer_device ? timer_device->ticks : 0;
}

uint64_t arch_timer_get_frequency(void)
{
    return timer_device ? timer_device->frequency : 0;
}

int arch_timer_set_interval(uint64_t interval_us)
{
    if (!timer_device || !timer_device->enabled) {
        return -1;
    }
    
    // Convert microseconds to frequency
    uint32_t freq_hz = 1000000 / interval_us;
    
    // Clamp frequency to valid range
    if (freq_hz > PIT_FREQUENCY) {
        freq_hz = PIT_FREQUENCY;
    }
    if (freq_hz < 1) {
        freq_hz = 1;
    }
    
    // Calculate divisor
    uint32_t divisor = PIT_FREQUENCY / freq_hz;
    if (divisor > PIT_MAX_COUNT) {
        divisor = PIT_MAX_COUNT;
        freq_hz = PIT_FREQUENCY / PIT_MAX_COUNT;
    }
    
    // Update timer settings
    timer_device->frequency = freq_hz;
    timer_device->divisor = divisor;
    
    // Program PIT with new divisor
    outb(PIT_COMMAND, PIT_CMD_CHANNEL0 | PIT_CMD_ACCESS_LOHI | PIT_CMD_MODE2 | PIT_CMD_BINARY);
    outb(PIT_CHANNEL0_DATA, divisor & 0xFF);         // Low byte
    outb(PIT_CHANNEL0_DATA, (divisor >> 8) & 0xFF); // High byte
    
    return 0;
}

int arch_timer_enable_interrupt(void)
{
    if (!timer_device || !timer_device->enabled) {
        return -1;
    }
    
    timer_device->interrupt_enabled = 1;
    
    early_print("x86-64 timer: Timer interrupts enabled\n");
    return 0;
}

void arch_timer_disable_interrupt(void)
{
    if (timer_device) {
        timer_device->interrupt_enabled = 0;
        early_print("x86-64 timer: Timer interrupts disabled\n");
    }
}

void arch_timer_ack_interrupt(void)
{
    if (timer_device && timer_device->interrupt_enabled) {
        timer_device->ticks++;
        system_ticks++;
    }
}

// Device driver structure
static struct device_id x86_64_timer_ids[] = {
    { 0, 0, NULL, DEVICE_TYPE_TIMER },
    DEVICE_ID_TABLE_END
};

struct device_driver x86_64_timer_driver = {
    .name = "x86_64-pit-timer",
    .version = "1.0",
    .type = DRIVER_TYPE_TIMER,
    .flags = DRIVER_FLAG_IRQ_HANDLER,
    .id_table = x86_64_timer_ids,
    .num_ids = 1,
    .probe = x86_64_timer_probe,
    .init = x86_64_timer_init,
    .start = x86_64_timer_start,
    .stop = x86_64_timer_stop,
    .cleanup = x86_64_timer_cleanup,
    .author = "MiniOS",
    .description = "x86-64 PIT Timer Driver",
    .license = "GPL",
};

// Register x86-64 timer driver
int x86_64_timer_driver_register(void)
{
    return driver_register(&x86_64_timer_driver);
}

#endif // ARCH_X86_64
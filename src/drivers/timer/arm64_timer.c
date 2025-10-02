/*
 * MiniOS ARM64 Generic Timer Driver
 * Phase 4: Device Drivers & System Services
 */

#include "timer.h"
#include "device.h"
#include "driver.h"
#include "memory.h"
#include "kernel.h"

#ifdef ARCH_ARM64

// Helper functions for system register access
static inline uint64_t read_cntfrq_el0(void) {
    uint64_t val;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r" (val));
    return val;
}

static inline uint64_t read_cntpct_el0(void) {
    uint64_t val;
    __asm__ volatile("mrs %0, cntpct_el0" : "=r" (val));
    return val;
}

static inline uint32_t read_cntp_ctl_el0(void) {
    uint32_t val;
    __asm__ volatile("mrs %0, cntp_ctl_el0" : "=r" (val));
    return val;
}

static inline void write_cntp_ctl_el0(uint32_t val) {
    __asm__ volatile("msr cntp_ctl_el0, %0" : : "r" (val));
    __asm__ volatile("isb");
}

static inline void write_cntp_cval_el0(uint64_t val) {
    __asm__ volatile("msr cntp_cval_el0, %0" : : "r" (val));
    __asm__ volatile("isb");
}

static inline void write_cntp_tval_el0(uint32_t val) {
    __asm__ volatile("msr cntp_tval_el0, %0" : : "r" (val));
    __asm__ volatile("isb");
}

// Timer control register bits
#define ARM64_TIMER_CTL_ENABLE      (1 << 0)   // Timer enable
#define ARM64_TIMER_CTL_IMASK       (1 << 1)   // Interrupt mask (0 = enabled)
#define ARM64_TIMER_CTL_ISTATUS     (1 << 2)   // Interrupt status

// ARM64 timer device private data
struct arm64_timer_device {
    uint64_t frequency;         // Timer frequency from CNTFRQ_EL0
    uint64_t last_count;        // Last timer count for calculations
    uint32_t enabled;           // Timer enabled flag
    uint32_t interrupt_enabled; // Interrupt enabled flag
};

// Global timer state
static struct arm64_timer_device *timer_device = NULL;
static uint64_t system_ticks = 0;
static uint64_t boot_time_us = 0;

// ARM64 timer driver operations
int arm64_timer_probe(struct device *device)
{
    if (!device) {
        return -1;
    }
    
    early_print("ARM64 timer: Probing generic timer\n");
    
    // Check if we already have a timer device
    if (timer_device) {
        return -1; // Only one timer device supported
    }
    
    // Read timer frequency
    uint64_t freq = read_cntfrq_el0();
    if (freq == 0) {
        early_print("ARM64 timer: Invalid timer frequency\n");
        return -1;
    }
    
    early_print("ARM64 timer: Timer frequency = ");
    // Convert frequency to string for display
    char freq_str[32];
    int pos = 0;
    uint64_t f = freq;
    if (f == 0) {
        freq_str[pos++] = '0';
    } else {
        char temp[32];
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
    
    return 0; // Probe successful
}

int arm64_timer_init(struct device *device)
{
    if (!device) {
        return -1;
    }
    
    early_print("ARM64 timer: Initializing generic timer\n");
    
    // Allocate private data
    struct arm64_timer_device *timer = memory_alloc(sizeof(struct arm64_timer_device), MEMORY_ALIGN_4K);
    if (!timer) {
        early_print("ARM64 timer: Failed to allocate memory\n");
        return -1;
    }
    
    // Initialize timer device structure explicitly
    timer->frequency = read_cntfrq_el0();
    timer->last_count = read_cntpct_el0();
    timer->enabled = 0;
    timer->interrupt_enabled = 0;
    
    // Set device private data
    device_set_private_data(device, timer);
    timer_device = timer;
    
    // Disable timer initially
    write_cntp_ctl_el0(ARM64_TIMER_CTL_IMASK); // Disable timer and mask interrupts
    
    early_print("ARM64 timer: Timer initialized successfully\n");
    return 0;
}

int arm64_timer_start(struct device *device)
{
    struct arm64_timer_device *timer = device_get_private_data(device);
    if (!timer) {
        return -1;
    }
    
    early_print("ARM64 timer: Starting timer\n");
    
    // Enable timer with interrupts masked initially
    write_cntp_ctl_el0(ARM64_TIMER_CTL_ENABLE | ARM64_TIMER_CTL_IMASK);
    timer->enabled = 1;
    
    // Initialize boot time
    boot_time_us = 0;
    system_ticks = 0;
    
    return 0;
}

int arm64_timer_stop(struct device *device)
{
    struct arm64_timer_device *timer = device_get_private_data(device);
    if (!timer) {
        return -1;
    }
    
    early_print("ARM64 timer: Stopping timer\n");
    
    // Disable timer and mask interrupts
    write_cntp_ctl_el0(ARM64_TIMER_CTL_IMASK);
    timer->enabled = 0;
    timer->interrupt_enabled = 0;
    
    return 0;
}

void arm64_timer_cleanup(struct device *device)
{
    struct arm64_timer_device *timer = device_get_private_data(device);
    if (timer) {
        // Disable timer
        write_cntp_ctl_el0(ARM64_TIMER_CTL_IMASK);
        
        // Free memory
        memory_free(timer);
        timer_device = NULL;
        
        early_print("ARM64 timer: Timer cleaned up\n");
    }
}

// Architecture-specific timer functions implementation
int arch_timer_init(void)
{
    if (!timer_device) {
        early_print("ARM64 timer: Timer device not initialized\n");
        return -1;
    }
    
    return 0;
}

uint64_t arch_timer_get_count(void)
{
    return read_cntpct_el0();
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
    
    // Convert microseconds to timer ticks
    uint64_t ticks = (interval_us * timer_device->frequency) / 1000000;
    
    // Set timer compare value
    uint64_t current_count = read_cntpct_el0();
    write_cntp_cval_el0(current_count + ticks);
    
    return 0;
}

int arch_timer_enable_interrupt(void)
{
    if (!timer_device || !timer_device->enabled) {
        return -1;
    }
    
    // Enable timer with interrupts unmasked
    write_cntp_ctl_el0(ARM64_TIMER_CTL_ENABLE);
    timer_device->interrupt_enabled = 1;
    
    early_print("ARM64 timer: Timer interrupts enabled\n");
    return 0;
}

void arch_timer_disable_interrupt(void)
{
    if (timer_device && timer_device->enabled) {
        // Enable timer but mask interrupts
        write_cntp_ctl_el0(ARM64_TIMER_CTL_ENABLE | ARM64_TIMER_CTL_IMASK);
        timer_device->interrupt_enabled = 0;
        
        early_print("ARM64 timer: Timer interrupts disabled\n");
    }
}

void arch_timer_ack_interrupt(void)
{
    if (timer_device && timer_device->interrupt_enabled) {
        // Timer interrupt is automatically cleared when CVAL is updated
        // We just need to update system ticks
        system_ticks++;
    }
}

// Device driver structure
static struct device_id arm64_timer_ids[] = {
    { 0, 0, "arm,generic-timer", DEVICE_TYPE_TIMER },
    DEVICE_ID_TABLE_END
};

struct device_driver arm64_timer_driver = {
    .name = "arm64-generic-timer",
    .version = "1.0",
    .type = DRIVER_TYPE_TIMER,
    .flags = DRIVER_FLAG_IRQ_HANDLER,
    .id_table = arm64_timer_ids,
    .num_ids = 1,
    .probe = arm64_timer_probe,
    .init = arm64_timer_init,
    .start = arm64_timer_start,
    .stop = arm64_timer_stop,
    .cleanup = arm64_timer_cleanup,
    .author = "MiniOS",
    .description = "ARM64 Generic Timer Driver",
    .license = "GPL",
};

// Register ARM64 timer driver
int arm64_timer_driver_register(void)
{
    return driver_register(&arm64_timer_driver);
}

#endif // ARCH_ARM64
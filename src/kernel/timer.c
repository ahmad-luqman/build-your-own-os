/*
 * MiniOS Timer Subsystem Implementation
 * Phase 4: Device Drivers & System Services
 */

#include "timer.h"
#include "device.h"
#include "driver.h"
#include "memory.h"
#include "kernel.h"

// Timer subsystem state
static int timer_subsystem_initialized = 0;
static uint32_t next_timer_id = 1;
static struct timer *timer_list = NULL;
static uint32_t scheduler_enabled = 0;
static uint32_t scheduler_frequency = TIMER_SCHEDULER_HZ;

// Timer statistics
static uint64_t total_timer_interrupts = 0;
static uint64_t scheduler_ticks = 0;

int timer_init(void)
{
    early_print("Initializing timer subsystem...\n");
    
    // Initialize timer device drivers first
#ifdef ARCH_ARM64
    extern int arm64_timer_driver_register(void);
    if (arm64_timer_driver_register() < 0) {
        early_print("Failed to register ARM64 timer driver\n");
        return -1;
    }
#endif

#ifdef ARCH_X86_64
    extern int x86_64_timer_driver_register(void);
    if (x86_64_timer_driver_register() < 0) {
        early_print("Failed to register x86-64 timer driver\n");
        return -1;
    }
#endif
    
    // Find and initialize timer device
    struct device *timer_dev = device_find_by_type(DEVICE_TYPE_TIMER);
    if (!timer_dev) {
        early_print("No timer device found, skipping timer device initialization\n");
        timer_subsystem_initialized = 1;
        return 0;
    }
    
    // Bind driver to device if not already bound
    if (!timer_dev->driver) {
        early_print("Timer device found, binding driver...\n");
        struct device_driver *timer_driver = driver_find_for_device(timer_dev);
        if (timer_driver) {
            if (driver_bind_device(timer_driver, timer_dev) < 0) {
                early_print("Failed to bind timer driver to device\n");
                return -1;
            }
            early_print("Timer driver bound successfully\n");
        } else {
            early_print("No timer driver found for device\n");
            return -1;
        }
    }
    
    // Initialize the timer device
    if (device_initialize(timer_dev) < 0) {
        early_print("Failed to initialize timer device\n");
        return -1;
    }
    
    // Start the timer device
    if (driver_start_device(timer_dev) < 0) {
        early_print("Failed to start timer device\n");
        return -1;
    }
    
    // Initialize architecture-specific timer
    if (arch_timer_init() < 0) {
        early_print("Failed to initialize architecture timer\n");
        return -1;
    }
    
    timer_subsystem_initialized = 1;
    
    early_print("Timer subsystem initialized successfully\n");
    return 0;
}

uint64_t timer_get_time_us(void)
{
    if (!timer_subsystem_initialized) {
        return 0;
    }
    
    return arch_timer_get_count() * 1000000 / arch_timer_get_frequency();
}

uint64_t timer_get_time_ms(void)
{
    return timer_get_time_us() / 1000;
}

uint64_t timer_get_ticks(void)
{
    return scheduler_ticks;
}

uint64_t timer_get_frequency(void)
{
    return arch_timer_get_frequency();
}

void timer_get_info(struct timer_info *info)
{
    if (!info || !timer_subsystem_initialized) {
        return;
    }
    
    info->frequency = arch_timer_get_frequency();
    info->resolution_ns = 1000000000 / info->frequency;
    info->max_interval_ms = (uint32_t)((0xFFFFFFFF / info->frequency) * 1000);
    info->capabilities = TIMER_FLAG_HIGH_RES | TIMER_FLAG_INTERRUPT;
}

uint32_t timer_create(uint32_t type, uint64_t interval_us, 
                     timer_callback_t callback, void *data)
{
    if (!timer_subsystem_initialized || !callback) {
        return 0;
    }
    
    // Allocate timer structure
    struct timer *timer = memory_alloc(sizeof(struct timer), MEMORY_ALIGN_4K);
    if (!timer) {
        return 0;
    }
    
    // Initialize timer
    timer->id = next_timer_id++;
    timer->type = type;
    timer->flags = 0;
    timer->interval_us = interval_us;
    timer->expiry_time = timer_get_time_us() + interval_us;
    timer->callback = callback;
    timer->callback_data = data;
    timer->next = timer_list;
    
    // Add to timer list
    timer_list = timer;
    
    return timer->id;
}

int timer_start(uint32_t timer_id)
{
    if (!timer_subsystem_initialized) {
        return -1;
    }
    
    // Find timer
    struct timer *timer = timer_list;
    while (timer) {
        if (timer->id == timer_id) {
            timer->flags |= TIMER_FLAG_ENABLED;
            timer->expiry_time = timer_get_time_us() + timer->interval_us;
            return 0;
        }
        timer = timer->next;
    }
    
    return -1; // Timer not found
}

int timer_stop(uint32_t timer_id)
{
    if (!timer_subsystem_initialized) {
        return -1;
    }
    
    // Find timer
    struct timer *timer = timer_list;
    while (timer) {
        if (timer->id == timer_id) {
            timer->flags &= ~TIMER_FLAG_ENABLED;
            return 0;
        }
        timer = timer->next;
    }
    
    return -1; // Timer not found
}

void timer_destroy(uint32_t timer_id)
{
    if (!timer_subsystem_initialized) {
        return;
    }
    
    // Remove from timer list
    struct timer *prev = NULL;
    struct timer *timer = timer_list;
    
    while (timer) {
        if (timer->id == timer_id) {
            if (prev) {
                prev->next = timer->next;
            } else {
                timer_list = timer->next;
            }
            memory_free(timer);
            return;
        }
        prev = timer;
        timer = timer->next;
    }
}

void timer_udelay(uint64_t us)
{
    if (!timer_subsystem_initialized) {
        // Fallback delay loop
        for (volatile uint64_t i = 0; i < us * 100; i++) {
            // Simple delay loop
        }
        return;
    }
    
    uint64_t start_time = timer_get_time_us();
    while ((timer_get_time_us() - start_time) < us) {
        // Wait
    }
}

void timer_mdelay(uint64_t ms)
{
    timer_udelay(ms * 1000);
}

int timer_set_scheduler_frequency(uint32_t freq_hz)
{
    if (!timer_subsystem_initialized || freq_hz == 0) {
        return -1;
    }
    
    scheduler_frequency = freq_hz;
    
    // Set timer interval
    uint64_t interval_us = 1000000 / freq_hz;
    return arch_timer_set_interval(interval_us);
}

int timer_enable_scheduler(void)
{
    if (!timer_subsystem_initialized) {
        return -1;
    }
    
    // Set scheduler frequency
    if (timer_set_scheduler_frequency(scheduler_frequency) < 0) {
        return -1;
    }
    
    // Enable timer interrupts
    if (arch_timer_enable_interrupt() < 0) {
        return -1;
    }
    
    scheduler_enabled = 1;
    
    early_print("Timer scheduler enabled at ");
    // Convert frequency to string
    char freq_str[16];
    int pos = 0;
    uint32_t f = scheduler_frequency;
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

void timer_disable_scheduler(void)
{
    if (timer_subsystem_initialized) {
        arch_timer_disable_interrupt();
        scheduler_enabled = 0;
        early_print("Timer scheduler disabled\n");
    }
}

void timer_interrupt_handler(void)
{
    if (!timer_subsystem_initialized) {
        return;
    }
    
    // Acknowledge interrupt
    arch_timer_ack_interrupt();
    
    // Update statistics
    total_timer_interrupts++;
    
    if (scheduler_enabled) {
        scheduler_ticks++;
        
        // Process expired timers
        timer_process_expired();
        
        // TODO: Call scheduler (Phase 4 - Process Management)
        // scheduler_tick();
    }
}

void timer_process_expired(void)
{
    if (!timer_subsystem_initialized) {
        return;
    }
    
    uint64_t current_time = timer_get_time_us();
    struct timer *timer = timer_list;
    
    while (timer) {
        if ((timer->flags & TIMER_FLAG_ENABLED) && 
            current_time >= timer->expiry_time) {
            
            // Timer expired, call callback
            if (timer->callback) {
                timer->callback(timer->callback_data);
            }
            
            // Handle timer type
            if (timer->type == TIMER_TYPE_ONESHOT) {
                // Disable oneshot timer
                timer->flags &= ~TIMER_FLAG_ENABLED;
            } else if (timer->type == TIMER_TYPE_PERIODIC) {
                // Reset periodic timer
                timer->expiry_time = current_time + timer->interval_us;
            }
        }
        timer = timer->next;
    }
}

// Device driver integration functions
int timer_driver_register(void)
{
    // This function is implemented in architecture-specific timer drivers
    return 0;
}

// Weak implementations for architecture-specific functions
int __attribute__((weak)) arch_timer_init(void)
{
    early_print("Warning: Using weak arch_timer_init\n");
    return 0;
}

uint64_t __attribute__((weak)) arch_timer_get_count(void)
{
    static uint64_t weak_count = 0;
    return ++weak_count;
}

uint64_t __attribute__((weak)) arch_timer_get_frequency(void)
{
    return 1000000; // Default 1 MHz
}

int __attribute__((weak)) arch_timer_set_interval(uint64_t interval_us)
{
    (void)interval_us;
    return 0;
}

int __attribute__((weak)) arch_timer_enable_interrupt(void)
{
    return 0;
}

void __attribute__((weak)) arch_timer_disable_interrupt(void)
{
    // Nothing to do
}

void __attribute__((weak)) arch_timer_ack_interrupt(void)
{
    // Nothing to do
}
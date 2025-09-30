/*
 * MiniOS Timer Services Interface
 * Phase 4: Device Drivers & System Services
 */

#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include <stddef.h>

// Forward declarations
struct device;

// Timer types
#define TIMER_TYPE_ONESHOT      0
#define TIMER_TYPE_PERIODIC     1

// Timer flags
#define TIMER_FLAG_ENABLED      (1 << 0)
#define TIMER_FLAG_INTERRUPT    (1 << 1)
#define TIMER_FLAG_HIGH_RES     (1 << 2)

// Timer frequencies (Hz)
#define TIMER_FREQ_1HZ          1
#define TIMER_FREQ_10HZ         10
#define TIMER_FREQ_100HZ        100
#define TIMER_FREQ_1KHZ         1000
#define TIMER_FREQ_10KHZ        10000

// Default scheduling frequency
#define TIMER_SCHEDULER_HZ      100  // 100 Hz = 10ms time slices

// Timer callback function type
typedef void (*timer_callback_t)(void *data);

// Timer structure
struct timer {
    uint32_t id;                    // Timer ID
    uint32_t type;                  // Timer type (oneshot/periodic)
    uint32_t flags;                 // Timer flags
    uint64_t interval_us;           // Interval in microseconds
    uint64_t expiry_time;           // Expiry time in microseconds
    timer_callback_t callback;      // Callback function
    void *callback_data;            // Callback data
    struct timer *next;             // Next timer in list
};

// System timer information
struct timer_info {
    uint64_t frequency;             // Timer frequency in Hz
    uint64_t resolution_ns;         // Timer resolution in nanoseconds
    uint32_t max_interval_ms;       // Maximum timer interval in milliseconds
    uint32_t capabilities;          // Timer capabilities
};

// Timer subsystem API

/**
 * Initialize timer subsystem
 * @return 0 on success, negative on error
 */
int timer_init(void);

/**
 * Get current system time in microseconds
 * @return Current time in microseconds since boot
 */
uint64_t timer_get_time_us(void);

/**
 * Get current system time in milliseconds
 * @return Current time in milliseconds since boot
 */
uint64_t timer_get_time_ms(void);

/**
 * Get system timer tick count
 * @return Number of timer ticks since boot
 */
uint64_t timer_get_ticks(void);

/**
 * Get timer frequency in Hz
 * @return Timer frequency
 */
uint64_t timer_get_frequency(void);

/**
 * Get timer information
 * @param info Pointer to timer_info structure to fill
 */
void timer_get_info(struct timer_info *info);

/**
 * Create a new timer
 * @param type Timer type (oneshot/periodic)
 * @param interval_us Interval in microseconds
 * @param callback Callback function
 * @param data Callback data
 * @return Timer ID on success, 0 on failure
 */
uint32_t timer_create(uint32_t type, uint64_t interval_us, 
                     timer_callback_t callback, void *data);

/**
 * Start a timer
 * @param timer_id Timer ID
 * @return 0 on success, negative on error
 */
int timer_start(uint32_t timer_id);

/**
 * Stop a timer
 * @param timer_id Timer ID
 * @return 0 on success, negative on error
 */
int timer_stop(uint32_t timer_id);

/**
 * Destroy a timer
 * @param timer_id Timer ID
 */
void timer_destroy(uint32_t timer_id);

/**
 * Sleep for specified microseconds
 * @param us Microseconds to sleep
 */
void timer_udelay(uint64_t us);

/**
 * Sleep for specified milliseconds
 * @param ms Milliseconds to sleep
 */
void timer_mdelay(uint64_t ms);

/**
 * Set scheduler timer frequency
 * @param freq_hz Frequency in Hz (e.g., 100 for 10ms slices)
 * @return 0 on success, negative on error
 */
int timer_set_scheduler_frequency(uint32_t freq_hz);

/**
 * Enable scheduler timer interrupts
 * @return 0 on success, negative on error
 */
int timer_enable_scheduler(void);

/**
 * Disable scheduler timer interrupts
 */
void timer_disable_scheduler(void);

/**
 * Timer interrupt handler (called by architecture-specific code)
 */
void timer_interrupt_handler(void);

/**
 * Process expired timers (called by timer interrupt handler)
 */
void timer_process_expired(void);

// Architecture-specific timer functions

/**
 * Initialize architecture-specific timer hardware
 * @return 0 on success, negative on error
 */
int arch_timer_init(void);

/**
 * Get architecture-specific timer count
 * @return Current timer count
 */
uint64_t arch_timer_get_count(void);

/**
 * Get architecture-specific timer frequency
 * @return Timer frequency in Hz
 */
uint64_t arch_timer_get_frequency(void);

/**
 * Set architecture-specific timer interval
 * @param interval_us Interval in microseconds
 * @return 0 on success, negative on error
 */
int arch_timer_set_interval(uint64_t interval_us);

/**
 * Enable architecture-specific timer interrupts
 * @return 0 on success, negative on error
 */
int arch_timer_enable_interrupt(void);

/**
 * Disable architecture-specific timer interrupts
 */
void arch_timer_disable_interrupt(void);

/**
 * Acknowledge architecture-specific timer interrupt
 */
void arch_timer_ack_interrupt(void);

// Timer device driver interface (for device framework integration)

/**
 * Register timer device driver
 * @return 0 on success, negative on error
 */
int timer_driver_register(void);

/**
 * Timer device probe function
 * @param device Timer device
 * @return 0 on success, negative on error
 */
int timer_device_probe(struct device *device);

/**
 * Timer device init function
 * @param device Timer device
 * @return 0 on success, negative on error
 */
int timer_device_init(struct device *device);

/**
 * Timer device start function
 * @param device Timer device
 * @return 0 on success, negative on error
 */
int timer_device_start(struct device *device);

/**
 * Timer device stop function
 * @param device Timer device
 * @return 0 on success, negative on error
 */
int timer_device_stop(struct device *device);

/**
 * Timer device cleanup function
 * @param device Timer device
 */
void timer_device_cleanup(struct device *device);

// Timer utility functions

/**
 * Convert microseconds to timer ticks
 * @param us Microseconds
 * @return Timer ticks
 */
static inline uint64_t timer_us_to_ticks(uint64_t us) {
    uint64_t freq = timer_get_frequency();
    return (us * freq) / 1000000;
}

/**
 * Convert timer ticks to microseconds
 * @param ticks Timer ticks
 * @return Microseconds
 */
static inline uint64_t timer_ticks_to_us(uint64_t ticks) {
    uint64_t freq = timer_get_frequency();
    return (ticks * 1000000) / freq;
}

/**
 * Convert milliseconds to timer ticks
 * @param ms Milliseconds
 * @return Timer ticks
 */
static inline uint64_t timer_ms_to_ticks(uint64_t ms) {
    uint64_t freq = timer_get_frequency();
    return (ms * freq) / 1000;
}

/**
 * Convert timer ticks to milliseconds
 * @param ticks Timer ticks
 * @return Milliseconds
 */
static inline uint64_t timer_ticks_to_ms(uint64_t ticks) {
    uint64_t freq = timer_get_frequency();
    return (ticks * 1000) / freq;
}

#endif // TIMER_H
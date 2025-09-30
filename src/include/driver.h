/*
 * MiniOS Device Driver Interface
 * Phase 4: Device Drivers & System Services
 */

#ifndef DRIVER_H
#define DRIVER_H

#include <stdint.h>
#include <stddef.h>

// Define off_t if not available (freestanding environment)
#ifndef off_t
typedef int64_t off_t;
#endif

// Forward declarations
struct device;
struct device_driver;

// Driver types
#define DRIVER_TYPE_TIMER       1
#define DRIVER_TYPE_UART        2
#define DRIVER_TYPE_INTERRUPT   3
#define DRIVER_TYPE_PCI         4
#define DRIVER_TYPE_BLOCK       5
#define DRIVER_TYPE_NETWORK     6

// Driver flags
#define DRIVER_FLAG_INITIALIZED (1 << 0)
#define DRIVER_FLAG_IRQ_HANDLER (1 << 1)
#define DRIVER_FLAG_DMA_CAPABLE (1 << 2)
#define DRIVER_FLAG_HOTPLUG     (1 << 3)

// Device identification structure
struct device_id {
    uint32_t vendor_id;         // Vendor ID (for PCI devices)
    uint32_t device_id;         // Device ID (for PCI devices)
    const char *compatible;     // Compatible string (for device tree)
    uint32_t type;              // Device type
};

// Device driver structure
struct device_driver {
    const char *name;           // Driver name
    const char *version;        // Driver version
    uint32_t type;              // Driver type (DRIVER_TYPE_*)
    uint32_t flags;             // Driver flags (DRIVER_FLAG_*)
    
    // Device identification
    struct device_id *id_table; // Supported device IDs
    uint32_t num_ids;           // Number of supported device IDs
    
    // Driver operations
    int (*probe)(struct device *dev);
    int (*init)(struct device *dev);
    int (*start)(struct device *dev);
    int (*stop)(struct device *dev);
    void (*cleanup)(struct device *dev);
    int (*suspend)(struct device *dev);
    int (*resume)(struct device *dev);
    
    // Device operations
    int (*read)(struct device *dev, void *buf, size_t len, off_t offset);
    int (*write)(struct device *dev, const void *buf, size_t len, off_t offset);
    int (*ioctl)(struct device *dev, unsigned int cmd, unsigned long arg);
    int (*mmap)(struct device *dev, uint64_t offset, uint64_t size, uint32_t flags);
    
    // Interrupt handling
    void (*interrupt_handler)(struct device *dev, uint32_t irq_num);
    int (*enable_interrupt)(struct device *dev);
    int (*disable_interrupt)(struct device *dev);
    
    // Power management
    int (*set_power_state)(struct device *dev, uint32_t state);
    uint32_t (*get_power_state)(struct device *dev);
    
    // Module information
    const char *author;         // Driver author
    const char *description;    // Driver description
    const char *license;        // License information
    
    // Linked list
    struct device_driver *next; // Next driver in global list
};

// Driver management API

/**
 * Initialize driver subsystem
 * @return 0 on success, negative on error
 */
int driver_init(void);

/**
 * Register a device driver
 * @param driver Driver to register
 * @return 0 on success, negative on error
 */
int driver_register(struct device_driver *driver);

/**
 * Unregister a device driver
 * @param driver Driver to unregister
 */
void driver_unregister(struct device_driver *driver);

/**
 * Find a driver by name
 * @param name Driver name to search for
 * @return Driver pointer if found, NULL otherwise
 */
struct device_driver *driver_find_by_name(const char *name);

/**
 * Find a driver for a specific device
 * @param device Device to find driver for
 * @return Driver pointer if found, NULL otherwise
 */
struct device_driver *driver_find_for_device(struct device *device);

/**
 * Match driver with device
 * @param driver Driver to check
 * @param device Device to match
 * @return 1 if driver supports device, 0 otherwise
 */
int driver_match_device(struct device_driver *driver, struct device *device);

/**
 * Bind driver to device
 * @param driver Driver to bind
 * @param device Device to bind to
 * @return 0 on success, negative on error
 */
int driver_bind_device(struct device_driver *driver, struct device *device);

/**
 * Unbind driver from device
 * @param device Device to unbind
 */
void driver_unbind_device(struct device *device);

/**
 * Probe device with driver
 * @param driver Driver to use for probing
 * @param device Device to probe
 * @return 0 if driver supports device, negative otherwise
 */
int driver_probe_device(struct device_driver *driver, struct device *device);

/**
 * Initialize device with its driver
 * @param device Device to initialize
 * @return 0 on success, negative on error
 */
int driver_init_device(struct device *device);

/**
 * Start device operation
 * @param device Device to start
 * @return 0 on success, negative on error
 */
int driver_start_device(struct device *device);

/**
 * Stop device operation
 * @param device Device to stop
 * @return 0 on success, negative on error
 */
int driver_stop_device(struct device *device);

/**
 * List all registered drivers (for debugging)
 */
void driver_list_all(void);

/**
 * Show driver information (for debugging)
 * @param driver Driver to show info for
 */
void driver_show_info(struct device_driver *driver);

/**
 * Get driver statistics
 * @param total_drivers Pointer to store total driver count
 * @param active_drivers Pointer to store active driver count
 */
void driver_get_stats(uint32_t *total_drivers, uint32_t *active_drivers);

// Helper macros for driver registration

#define DRIVER_INIT(name) \
    static int __init driver_##name##_init(void) { \
        return driver_register(&name##_driver); \
    }

#define DRIVER_EXIT(name) \
    static void __exit driver_##name##_exit(void) { \
        driver_unregister(&name##_driver); \
    }

#define DEVICE_ID_TABLE_END { 0, 0, NULL, 0 }

// Common device ID matching helpers

/**
 * Check if device ID matches PCI vendor/device
 * @param device_id Device ID structure
 * @param vendor_id PCI vendor ID
 * @param dev_id PCI device ID
 * @return 1 if matches, 0 otherwise
 */
int device_id_match_pci(struct device_id *device_id, uint32_t vendor_id, uint32_t dev_id);

/**
 * Check if device ID matches compatible string
 * @param device_id Device ID structure
 * @param compatible Compatible string
 * @return 1 if matches, 0 otherwise
 */
int device_id_match_compatible(struct device_id *device_id, const char *compatible);

/**
 * Check if device ID matches device type
 * @param device_id Device ID structure
 * @param type Device type
 * @return 1 if matches, 0 otherwise
 */
int device_id_match_type(struct device_id *device_id, uint32_t type);

// Standard driver operation wrappers

/**
 * Safe driver read operation
 * @param device Device to read from
 * @param buf Buffer to store data
 * @param len Number of bytes to read
 * @param offset Offset within device
 * @return Number of bytes read, negative on error
 */
int driver_read(struct device *device, void *buf, size_t len, off_t offset);

/**
 * Safe driver write operation
 * @param device Device to write to
 * @param buf Data to write
 * @param len Number of bytes to write
 * @param offset Offset within device
 * @return Number of bytes written, negative on error
 */
int driver_write(struct device *device, const void *buf, size_t len, off_t offset);

/**
 * Safe driver ioctl operation
 * @param device Device to control
 * @param cmd Command code
 * @param arg Command argument
 * @return Command-specific return value
 */
int driver_ioctl(struct device *device, unsigned int cmd, unsigned long arg);

/**
 * Safe driver interrupt enable
 * @param device Device to enable interrupts for
 * @return 0 on success, negative on error
 */
int driver_enable_interrupt(struct device *device);

/**
 * Safe driver interrupt disable
 * @param device Device to disable interrupts for
 * @return 0 on success, negative on error
 */
int driver_disable_interrupt(struct device *device);

#endif // DRIVER_H
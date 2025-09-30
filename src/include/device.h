/*
 * MiniOS Device Management Interface
 * Phase 4: Device Drivers & System Services
 */

#ifndef DEVICE_H
#define DEVICE_H

#include <stdint.h>
#include <stddef.h>

// Define off_t if not available (freestanding environment)
#ifndef off_t
typedef int64_t off_t;
#endif

// Forward declarations
struct device;
struct device_driver;
struct boot_info;

// Device types
#define DEVICE_TYPE_UNKNOWN     0
#define DEVICE_TYPE_TIMER       1
#define DEVICE_TYPE_UART        2
#define DEVICE_TYPE_INTERRUPT   3
#define DEVICE_TYPE_PCI         4
#define DEVICE_TYPE_BLOCK       5
#define DEVICE_TYPE_NETWORK     6

// Device flags
#define DEVICE_FLAG_INITIALIZED (1 << 0)
#define DEVICE_FLAG_ACTIVE      (1 << 1)
#define DEVICE_FLAG_ERROR       (1 << 2)
#define DEVICE_FLAG_IRQ_CAPABLE (1 << 3)

// Device states
#define DEVICE_STATE_UNKNOWN    0
#define DEVICE_STATE_PROBED     1
#define DEVICE_STATE_INITIALIZED 2
#define DEVICE_STATE_ACTIVE     3
#define DEVICE_STATE_ERROR      4

// Device instance
struct device {
    char name[64];                  // Device name (e.g., "uart0", "timer0")
    uint32_t device_id;             // Unique device ID
    uint32_t type;                  // Device type (DEVICE_TYPE_*)
    uint32_t flags;                 // Device flags (DEVICE_FLAG_*)
    uint32_t state;                 // Current state (DEVICE_STATE_*)
    
    struct device_driver *driver;   // Associated driver
    void *private_data;             // Driver-specific data
    
    // Hardware information
    uint64_t base_addr;             // Base physical address
    uint32_t irq_num;               // IRQ number (if applicable)
    uint32_t vendor_id;             // Vendor ID (for PCI devices)
    uint32_t device_id_hw;          // Hardware device ID
    
    // Device tree information (ARM64)
    void *dt_node;                  // Device tree node pointer
    
    // Linked list
    struct device *next;            // Next device in global list
};

// Device operations structure
struct device_ops {
    int (*read)(struct device *dev, void *buf, size_t len, off_t offset);
    int (*write)(struct device *dev, const void *buf, size_t len, off_t offset);
    int (*ioctl)(struct device *dev, unsigned int cmd, unsigned long arg);
    int (*mmap)(struct device *dev, uint64_t offset, uint64_t size, uint32_t flags);
    void (*interrupt_handler)(struct device *dev);
};

// Device management API

/**
 * Initialize device subsystem
 * @param boot_info Boot information structure
 * @return 0 on success, negative on error
 */
int device_init(struct boot_info *boot_info);

/**
 * Create a new device
 * @param name Device name
 * @param type Device type
 * @return Device pointer on success, NULL on failure
 */
struct device *device_create(const char *name, uint32_t type);

/**
 * Register a device in the system
 * @param device Device to register
 * @return 0 on success, negative on error
 */
int device_register(struct device *device);

/**
 * Unregister a device from the system
 * @param device Device to unregister
 */
void device_unregister(struct device *device);

/**
 * Find a device by name
 * @param name Device name to search for
 * @return Device pointer if found, NULL otherwise
 */
struct device *device_find_by_name(const char *name);

/**
 * Find a device by type
 * @param type Device type to search for
 * @return First device of specified type, NULL if none found
 */
struct device *device_find_by_type(uint32_t type);

/**
 * Get all devices of a specific type
 * @param type Device type to search for
 * @param devices Array to store device pointers
 * @param max_devices Maximum number of devices to return
 * @return Number of devices found
 */
int device_get_by_type(uint32_t type, struct device **devices, int max_devices);

/**
 * Initialize a device with its driver
 * @param device Device to initialize
 * @return 0 on success, negative on error
 */
int device_initialize(struct device *device);

/**
 * Set device state
 * @param device Device to update
 * @param state New state
 */
void device_set_state(struct device *device, uint32_t state);

/**
 * Set device flags
 * @param device Device to update
 * @param flags Flags to set
 */
void device_set_flags(struct device *device, uint32_t flags);

/**
 * Clear device flags
 * @param device Device to update
 * @param flags Flags to clear
 */
void device_clear_flags(struct device *device, uint32_t flags);

/**
 * Check if device has specific flags
 * @param device Device to check
 * @param flags Flags to test
 * @return 1 if all flags are set, 0 otherwise
 */
int device_has_flags(struct device *device, uint32_t flags);

/**
 * Set device private data
 * @param device Device to update
 * @param data Private data pointer
 */
void device_set_private_data(struct device *device, void *data);

/**
 * Get device private data
 * @param device Device to query
 * @return Private data pointer
 */
void *device_get_private_data(struct device *device);

/**
 * Read from device
 * @param device Device to read from
 * @param buf Buffer to store data
 * @param len Number of bytes to read
 * @param offset Offset within device
 * @return Number of bytes read, negative on error
 */
int device_read(struct device *device, void *buf, size_t len, off_t offset);

/**
 * Write to device
 * @param device Device to write to
 * @param buf Data to write
 * @param len Number of bytes to write
 * @param offset Offset within device
 * @return Number of bytes written, negative on error
 */
int device_write(struct device *device, const void *buf, size_t len, off_t offset);

/**
 * Device I/O control
 * @param device Device to control
 * @param cmd Command code
 * @param arg Command argument
 * @return Command-specific return value
 */
int device_ioctl(struct device *device, unsigned int cmd, unsigned long arg);

/**
 * List all registered devices (for debugging)
 */
void device_list_all(void);

/**
 * Show device information (for debugging)
 * @param device Device to show info for
 */
void device_show_info(struct device *device);

/**
 * Get device statistics
 * @param total_devices Pointer to store total device count
 * @param active_devices Pointer to store active device count
 * @param error_devices Pointer to store error device count
 */
void device_get_stats(uint32_t *total_devices, uint32_t *active_devices, uint32_t *error_devices);

// Architecture-specific device discovery functions

/**
 * Initialize architecture-specific device discovery
 * @param boot_info Boot information structure
 * @return 0 on success, negative on error
 */
int arch_device_discovery_init(struct boot_info *boot_info);

/**
 * Scan for devices using architecture-specific methods
 * @return Number of devices discovered
 */
int arch_device_scan(void);

#ifdef ARCH_ARM64
/**
 * Parse device tree for ARM64 devices
 * @param dt_base Device tree base address
 * @return Number of devices found
 */
int device_tree_parse(void *dt_base);

/**
 * Create device from device tree node
 * @param dt_node Device tree node
 * @return Device pointer on success, NULL on failure
 */
struct device *device_create_from_dt_node(void *dt_node);
#endif

#ifdef ARCH_X86_64
/**
 * Scan PCI bus for x86-64 devices
 * @return Number of devices found
 */
int pci_device_scan(void);

/**
 * Create device from PCI configuration
 * @param bus PCI bus number
 * @param device PCI device number
 * @param function PCI function number
 * @return Device pointer on success, NULL on failure
 */
struct device *device_create_from_pci(uint8_t bus, uint8_t device, uint8_t function);
#endif

#endif // DEVICE_H
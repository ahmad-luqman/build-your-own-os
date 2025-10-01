/*
 * MiniOS Device Management Implementation
 * Phase 4: Device Drivers & System Services
 */

#include "device.h"
#include "driver.h"
#include "memory.h"
#include "kernel.h"

// Global device list
static struct device *device_list = NULL;
static uint32_t device_count = 0;
static uint32_t next_device_id = 1;

// Device statistics
static uint32_t total_devices = 0;
static uint32_t active_devices = 0;
static uint32_t error_devices = 0;

// Device subsystem initialization flag
static int device_subsystem_initialized = 0;

int device_init(struct boot_info *boot_info)
{
    early_print("Initializing device subsystem...\n");
    
    // Initialize driver subsystem first
    if (driver_init() < 0) {
        early_print("Failed to initialize driver subsystem\n");
        return -1;
    }
    
    // Initialize architecture-specific device discovery
    if (arch_device_discovery_init(boot_info) < 0) {
        early_print("Failed to initialize device discovery\n");
        return -1;
    }
    
    device_subsystem_initialized = 1;
    
    // Scan for devices
    int discovered = arch_device_scan();
    
    early_print("Device subsystem initialized, ");
    // Simple number to string conversion for discovered devices
    char num_str[16];
    int pos = 0;
    if (discovered == 0) {
        num_str[pos++] = '0';
    } else {
        char temp[16];
        int temp_pos = 0;
        while (discovered > 0) {
            temp[temp_pos++] = '0' + (discovered % 10);
            discovered /= 10;
        }
        while (temp_pos > 0) {
            num_str[pos++] = temp[--temp_pos];
        }
    }
    num_str[pos] = 0;
    early_print(num_str);
    early_print(" devices discovered\n");
    
    return 0;
}

struct device *device_create(const char *name, uint32_t type)
{
    if (!device_subsystem_initialized) {
        early_print("Device subsystem not initialized\n");
        return NULL;
    }
    
    // Allocate memory for device structure
    struct device *dev = memory_alloc(sizeof(struct device), MEMORY_ALIGN_4K);
    if (!dev) {
        early_print("Failed to allocate memory for device\n");
        return NULL;
    }
    
    // Check if the allocated memory is in a valid range
    uint64_t dev_addr = (uint64_t)dev;
    if (dev_addr < 0x40000000 || dev_addr > 0x48000000) {
        early_print("WARNING: Device allocated at invalid address\n");
        return NULL;
    }
    
    // Initialize device structure
    // Copy name (simple string copy)
    int i;
    for (i = 0; i < 63 && name[i] != 0; i++) {
        dev->name[i] = name[i];
    }
    dev->name[i] = 0;
    
    dev->device_id = next_device_id++;
    dev->type = type;
    dev->flags = 0;
    dev->state = DEVICE_STATE_UNKNOWN;
    dev->driver = NULL;
    dev->private_data = NULL;
    dev->base_addr = 0;
    dev->irq_num = 0;
    dev->vendor_id = 0;
    dev->device_id_hw = 0;
    dev->dt_node = NULL;
    dev->next = NULL;
    
    return dev;
}

int device_register(struct device *device)
{
    if (!device || !device_subsystem_initialized) {
        return -1;
    }
    
    // Add to global device list
    device->next = device_list;
    device_list = device;
    device_count++;
    total_devices++;
    
    // Try to find and bind a driver
    struct device_driver *driver = driver_find_for_device(device);
    if (driver) {
        if (driver_bind_device(driver, device) == 0) {
            device_set_state(device, DEVICE_STATE_PROBED);
        }
    }
    
    return 0;
}

void device_unregister(struct device *device)
{
    if (!device || !device_subsystem_initialized) {
        return;
    }
    
    // Remove from device list
    if (device_list == device) {
        device_list = device->next;
    } else {
        struct device *current = device_list;
        while (current && current->next != device) {
            current = current->next;
        }
        if (current) {
            current->next = device->next;
        }
    }
    
    // Unbind driver if bound
    if (device->driver) {
        driver_unbind_device(device);
    }
    
    device_count--;
    if (device->state == DEVICE_STATE_ERROR) {
        error_devices--;
    } else if (device->state == DEVICE_STATE_ACTIVE) {
        active_devices--;
    }
    
    // Free device memory
    memory_free(device);
}

struct device *device_find_by_name(const char *name)
{
    if (!name || !device_subsystem_initialized) {
        return NULL;
    }
    
    struct device *current = device_list;
    while (current) {
        // Simple string comparison
        int i;
        int match = 1;
        for (i = 0; i < 64; i++) {
            if (current->name[i] != name[i]) {
                match = 0;
                break;
            }
            if (name[i] == 0) {
                break;
            }
        }
        if (match) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

struct device *device_find_by_type(uint32_t type)
{
    if (!device_subsystem_initialized) {
        return NULL;
    }
    
    struct device *current = device_list;
    while (current) {
        if (current->type == type) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

int device_get_by_type(uint32_t type, struct device **devices, int max_devices)
{
    if (!devices || max_devices <= 0 || !device_subsystem_initialized) {
        return 0;
    }
    
    int count = 0;
    struct device *current = device_list;
    while (current && count < max_devices) {
        if (current->type == type) {
            devices[count++] = current;
        }
        current = current->next;
    }
    
    return count;
}

int device_initialize(struct device *device)
{
    if (!device || !device->driver) {
        return -1;
    }
    
    // Initialize device with its driver
    int result = driver_init_device(device);
    if (result == 0) {
        device_set_state(device, DEVICE_STATE_INITIALIZED);
        device_set_flags(device, DEVICE_FLAG_INITIALIZED);
    } else {
        device_set_state(device, DEVICE_STATE_ERROR);
        device_set_flags(device, DEVICE_FLAG_ERROR);
        error_devices++;
    }
    
    return result;
}

void device_set_state(struct device *device, uint32_t state)
{
    if (!device) {
        return;
    }
    
    uint32_t old_state = device->state;
    device->state = state;
    
    // Update statistics
    if (old_state == DEVICE_STATE_ACTIVE && state != DEVICE_STATE_ACTIVE) {
        active_devices--;
    } else if (old_state != DEVICE_STATE_ACTIVE && state == DEVICE_STATE_ACTIVE) {
        active_devices++;
    }
    
    if (old_state == DEVICE_STATE_ERROR && state != DEVICE_STATE_ERROR) {
        error_devices--;
    } else if (old_state != DEVICE_STATE_ERROR && state == DEVICE_STATE_ERROR) {
        error_devices++;
    }
}

void device_set_flags(struct device *device, uint32_t flags)
{
    if (device) {
        device->flags |= flags;
    }
}

void device_clear_flags(struct device *device, uint32_t flags)
{
    if (device) {
        device->flags &= ~flags;
    }
}

int device_has_flags(struct device *device, uint32_t flags)
{
    if (!device) {
        return 0;
    }
    return (device->flags & flags) == flags;
}

void device_set_private_data(struct device *device, void *data)
{
    if (device) {
        device->private_data = data;
    }
}

void *device_get_private_data(struct device *device)
{
    return device ? device->private_data : NULL;
}

int device_read(struct device *device, void *buf, size_t len, off_t offset)
{
    if (!device || !device->driver) {
        return -1;
    }
    
    return driver_read(device, buf, len, offset);
}

int device_write(struct device *device, const void *buf, size_t len, off_t offset)
{
    if (!device || !device->driver) {
        return -1;
    }
    
    return driver_write(device, buf, len, offset);
}

int device_ioctl(struct device *device, unsigned int cmd, unsigned long arg)
{
    if (!device || !device->driver) {
        return -1;
    }
    
    return driver_ioctl(device, cmd, arg);
}

void device_list_all(void)
{
    if (!device_subsystem_initialized) {
        early_print("Device subsystem not initialized\n");
        return;
    }
    
    early_print("=== Device List ===\n");
    struct device *current = device_list;
    int count = 0;
    
    while (current) {
        device_show_info(current);
        current = current->next;
        count++;
    }
    
    early_print("Total devices: ");
    // Simple number to string conversion
    char num_str[16];
    int pos = 0;
    if (count == 0) {
        num_str[pos++] = '0';
    } else {
        char temp[16];
        int temp_pos = 0;
        while (count > 0) {
            temp[temp_pos++] = '0' + (count % 10);
            count /= 10;
        }
        while (temp_pos > 0) {
            num_str[pos++] = temp[--temp_pos];
        }
    }
    num_str[pos] = 0;
    early_print(num_str);
    early_print("\n");
}

void device_show_info(struct device *device)
{
    if (!device) {
        return;
    }
    
    early_print("Device: ");
    early_print(device->name);
    early_print(" (ID: ");
    
    // Convert device ID to string
    char id_str[16];
    int pos = 0;
    uint32_t id = device->device_id;
    if (id == 0) {
        id_str[pos++] = '0';
    } else {
        char temp[16];
        int temp_pos = 0;
        while (id > 0) {
            temp[temp_pos++] = '0' + (id % 10);
            id /= 10;
        }
        while (temp_pos > 0) {
            id_str[pos++] = temp[--temp_pos];
        }
    }
    id_str[pos] = 0;
    early_print(id_str);
    
    early_print(", Type: ");
    // Convert type to string
    char type_str[16];
    pos = 0;
    uint32_t type = device->type;
    if (type == 0) {
        type_str[pos++] = '0';
    } else {
        char temp[16];
        int temp_pos = 0;
        while (type > 0) {
            temp[temp_pos++] = '0' + (type % 10);
            type /= 10;
        }
        while (temp_pos > 0) {
            type_str[pos++] = temp[--temp_pos];
        }
    }
    type_str[pos] = 0;
    early_print(type_str);
    
    early_print(", State: ");
    switch (device->state) {
        case DEVICE_STATE_UNKNOWN: early_print("UNKNOWN"); break;
        case DEVICE_STATE_PROBED: early_print("PROBED"); break;
        case DEVICE_STATE_INITIALIZED: early_print("INITIALIZED"); break;
        case DEVICE_STATE_ACTIVE: early_print("ACTIVE"); break;
        case DEVICE_STATE_ERROR: early_print("ERROR"); break;
        default: early_print("INVALID"); break;
    }
    
    if (device->driver) {
        early_print(", Driver: ");
        early_print(device->driver->name);
    }
    
    early_print(")\n");
}

void device_get_stats(uint32_t *total, uint32_t *active, uint32_t *error)
{
    if (total) *total = total_devices;
    if (active) *active = active_devices;
    if (error) *error = error_devices;
}

// Architecture-specific implementations will be provided in arch/ directories
int __attribute__((weak)) arch_device_discovery_init(struct boot_info *boot_info)
{
    (void)boot_info; // Suppress unused parameter warning
    early_print("Warning: Using weak arch_device_discovery_init\n");
    return 0;
}

int __attribute__((weak)) arch_device_scan(void)
{
    early_print("Warning: Using weak arch_device_scan\n");
    return 0;
}
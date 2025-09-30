/*
 * MiniOS Device Driver Management Implementation
 * Phase 4: Device Drivers & System Services
 */

#include "driver.h"
#include "device.h"
#include "memory.h"
#include "kernel.h"

// Global driver list
static struct device_driver *driver_list = NULL;
static uint32_t driver_count = 0;

// Driver subsystem initialization flag
static int driver_subsystem_initialized = 0;

int driver_init(void)
{
    early_print("Initializing driver subsystem...\n");
    
    driver_list = NULL;
    driver_count = 0;
    driver_subsystem_initialized = 1;
    
    early_print("Driver subsystem initialized\n");
    return 0;
}

int driver_register(struct device_driver *driver)
{
    if (!driver || !driver_subsystem_initialized) {
        return -1;
    }
    
    // Validate required fields
    if (!driver->name) {
        return -1;
    }
    
    // Check if driver is already registered
    struct device_driver *existing = driver_find_by_name(driver->name);
    if (existing) {
        return -1; // Already registered
    }
    
    // Add to global driver list
    driver->next = driver_list;
    driver_list = driver;
    driver_count++;
    
    early_print("Driver registered: ");
    early_print(driver->name);
    if (driver->version) {
        early_print(" v");
        early_print(driver->version);
    }
    early_print("\n");
    
    return 0;
}

void driver_unregister(struct device_driver *driver)
{
    if (!driver || !driver_subsystem_initialized) {
        return;
    }
    
    // Remove from driver list
    if (driver_list == driver) {
        driver_list = driver->next;
    } else {
        struct device_driver *current = driver_list;
        while (current && current->next != driver) {
            current = current->next;
        }
        if (current) {
            current->next = driver->next;
        }
    }
    
    driver_count--;
    
    early_print("Driver unregistered: ");
    early_print(driver->name);
    early_print("\n");
}

struct device_driver *driver_find_by_name(const char *name)
{
    if (!name || !driver_subsystem_initialized) {
        return NULL;
    }
    
    struct device_driver *current = driver_list;
    while (current) {
        // Simple string comparison
        int i;
        int match = 1;
        for (i = 0; i < 256; i++) { // Reasonable limit
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

struct device_driver *driver_find_for_device(struct device *device)
{
    if (!device || !driver_subsystem_initialized) {
        return NULL;
    }
    
    struct device_driver *current = driver_list;
    while (current) {
        if (driver_match_device(current, device)) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

int driver_match_device(struct device_driver *driver, struct device *device)
{
    if (!driver || !device) {
        return 0;
    }
    
    // Check if driver supports device type
    if (driver->type != 0 && driver->type != device->type) {
        return 0;
    }
    
    // Check device ID table if available
    if (driver->id_table && driver->num_ids > 0) {
        for (uint32_t i = 0; i < driver->num_ids; i++) {
            struct device_id *id = &driver->id_table[i];
            
            // Check PCI vendor/device ID match
            if (id->vendor_id != 0 && id->device_id != 0) {
                if (device->vendor_id == id->vendor_id && 
                    device->device_id_hw == id->device_id) {
                    return 1;
                }
            }
            
            // Check compatible string match (for device tree)
            if (id->compatible) {
                // For now, we don't have device tree parsing implemented
                // This would compare against device tree compatible strings
                // return device_id_match_compatible(id, device->compatible);
            }
            
            // Check device type match
            if (id->type != 0 && device->type == id->type) {
                return 1;
            }
        }
    } else {
        // No ID table, match by type only
        return (driver->type == device->type);
    }
    
    return 0;
}

int driver_bind_device(struct device_driver *driver, struct device *device)
{
    if (!driver || !device) {
        return -1;
    }
    
    // Check if device is already bound to a driver
    if (device->driver) {
        return -1;
    }
    
    // Verify driver supports this device
    if (!driver_match_device(driver, device)) {
        return -1;
    }
    
    // Bind driver to device
    device->driver = driver;
    
    // Probe the device
    if (driver->probe) {
        int result = driver->probe(device);
        if (result < 0) {
            device->driver = NULL;
            return result;
        }
    }
    
    return 0;
}

void driver_unbind_device(struct device *device)
{
    if (!device || !device->driver) {
        return;
    }
    
    struct device_driver *driver = device->driver;
    
    // Stop device if it's running
    if (device->state == DEVICE_STATE_ACTIVE) {
        driver_stop_device(device);
    }
    
    // Clean up device
    if (driver->cleanup) {
        driver->cleanup(device);
    }
    
    // Unbind
    device->driver = NULL;
}

int driver_probe_device(struct device_driver *driver, struct device *device)
{
    if (!driver || !device) {
        return -1;
    }
    
    if (driver->probe) {
        return driver->probe(device);
    }
    
    return 0; // No probe function, assume success
}

int driver_init_device(struct device *device)
{
    if (!device || !device->driver) {
        return -1;
    }
    
    struct device_driver *driver = device->driver;
    
    if (driver->init) {
        return driver->init(device);
    }
    
    return 0; // No init function, assume success
}

int driver_start_device(struct device *device)
{
    if (!device || !device->driver) {
        return -1;
    }
    
    struct device_driver *driver = device->driver;
    
    if (driver->start) {
        int result = driver->start(device);
        if (result == 0) {
            device_set_state(device, DEVICE_STATE_ACTIVE);
            device_set_flags(device, DEVICE_FLAG_ACTIVE);
        }
        return result;
    }
    
    // No start function, set active anyway
    device_set_state(device, DEVICE_STATE_ACTIVE);
    device_set_flags(device, DEVICE_FLAG_ACTIVE);
    return 0;
}

int driver_stop_device(struct device *device)
{
    if (!device || !device->driver) {
        return -1;
    }
    
    struct device_driver *driver = device->driver;
    
    if (driver->stop) {
        int result = driver->stop(device);
        if (result == 0) {
            device_clear_flags(device, DEVICE_FLAG_ACTIVE);
        }
        return result;
    }
    
    // No stop function, clear active flag anyway
    device_clear_flags(device, DEVICE_FLAG_ACTIVE);
    return 0;
}

void driver_list_all(void)
{
    if (!driver_subsystem_initialized) {
        early_print("Driver subsystem not initialized\n");
        return;
    }
    
    early_print("=== Driver List ===\n");
    struct device_driver *current = driver_list;
    int count = 0;
    
    while (current) {
        driver_show_info(current);
        current = current->next;
        count++;
    }
    
    early_print("Total drivers: ");
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

void driver_show_info(struct device_driver *driver)
{
    if (!driver) {
        return;
    }
    
    early_print("Driver: ");
    early_print(driver->name);
    
    if (driver->version) {
        early_print(" v");
        early_print(driver->version);
    }
    
    early_print(" (Type: ");
    // Convert type to string
    char type_str[16];
    int pos = 0;
    uint32_t type = driver->type;
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
    
    early_print(")");
    
    if (driver->description) {
        early_print(" - ");
        early_print(driver->description);
    }
    
    if (driver->author) {
        early_print(" by ");
        early_print(driver->author);
    }
    
    early_print("\n");
}

void driver_get_stats(uint32_t *total_drivers, uint32_t *active_drivers)
{
    if (total_drivers) {
        *total_drivers = driver_count;
    }
    
    if (active_drivers) {
        // For simplicity, just return driver count as active count
        *active_drivers = driver_count;
    }
}

// Helper functions for device ID matching

int device_id_match_pci(struct device_id *device_id, uint32_t vendor_id, uint32_t dev_id)
{
    if (!device_id) {
        return 0;
    }
    
    return (device_id->vendor_id == vendor_id && device_id->device_id == dev_id);
}

int device_id_match_compatible(struct device_id *device_id, const char *compatible)
{
    if (!device_id || !device_id->compatible || !compatible) {
        return 0;
    }
    
    // Simple string comparison
    int i;
    for (i = 0; i < 256; i++) { // Reasonable limit
        if (device_id->compatible[i] != compatible[i]) {
            return 0;
        }
        if (compatible[i] == 0) {
            return 1; // Match found
        }
    }
    
    return 0; // No match or string too long
}

int device_id_match_type(struct device_id *device_id, uint32_t type)
{
    if (!device_id) {
        return 0;
    }
    
    return (device_id->type == type);
}

// Driver wrapper function implementations
int driver_read(struct device *device, void *buf, size_t len, off_t offset)
{
    if (device && device->driver && device->driver->read) {
        return device->driver->read(device, buf, len, offset);
    }
    return -1; // Operation not supported
}

int driver_write(struct device *device, const void *buf, size_t len, off_t offset)
{
    if (device && device->driver && device->driver->write) {
        return device->driver->write(device, buf, len, offset);
    }
    return -1; // Operation not supported
}

int driver_ioctl(struct device *device, unsigned int cmd, unsigned long arg)
{
    if (device && device->driver && device->driver->ioctl) {
        return device->driver->ioctl(device, cmd, arg);
    }
    return -1; // Operation not supported
}

int driver_enable_interrupt(struct device *device)
{
    if (device && device->driver && device->driver->enable_interrupt) {
        return device->driver->enable_interrupt(device);
    }
    return -1; // Operation not supported
}

int driver_disable_interrupt(struct device *device)
{
    if (device && device->driver && device->driver->disable_interrupt) {
        return device->driver->disable_interrupt(device);
    }
    return -1; // Operation not supported
}
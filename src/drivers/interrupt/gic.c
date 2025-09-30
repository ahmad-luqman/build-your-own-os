/*
 * MiniOS ARM64 GIC (Generic Interrupt Controller) Driver
 * Phase 4: Device Drivers & System Services
 */

#include "interrupt.h"
#include "memory.h"
#include "kernel.h"

#ifdef ARCH_ARM64

// GIC register offsets (distributor)
#define GICD_CTLR               0x000   // Distributor Control Register
#define GICD_TYPER              0x004   // Interrupt Controller Type Register
#define GICD_IIDR               0x008   // Distributor Implementer Identification Register
#define GICD_IGROUPR            0x080   // Interrupt Group Registers
#define GICD_ISENABLER          0x100   // Interrupt Set-Enable Registers
#define GICD_ICENABLER          0x180   // Interrupt Clear-Enable Registers
#define GICD_ISPENDR            0x200   // Interrupt Set-Pending Registers
#define GICD_ICPENDR            0x280   // Interrupt Clear-Pending Registers
#define GICD_ISACTIVER          0x300   // Interrupt Set-Active Registers
#define GICD_ICACTIVER          0x380   // Interrupt Clear-Active Registers
#define GICD_IPRIORITYR         0x400   // Interrupt Priority Registers
#define GICD_ITARGETSR          0x800   // Interrupt Processor Targets Registers
#define GICD_ICFGR              0xC00   // Interrupt Configuration Registers
#define GICD_SGIR               0xF00   // Software Generated Interrupt Register

// GIC register offsets (CPU interface)
#define GICC_CTLR               0x000   // CPU Interface Control Register
#define GICC_PMR                0x004   // Interrupt Priority Mask Register
#define GICC_BPR                0x008   // Binary Point Register
#define GICC_IAR                0x00C   // Interrupt Acknowledge Register
#define GICC_EOIR               0x010   // End of Interrupt Register
#define GICC_RPR                0x014   // Running Priority Register
#define GICC_HPPIR              0x018   // Highest Priority Pending Interrupt Register

// GIC constants
#define GIC_MAX_IRQS            1020
#define GIC_SGI_MAX             16      // Software Generated Interrupts (0-15)
#define GIC_PPI_MAX             32      // Private Peripheral Interrupts (16-31)
#define GIC_SPI_BASE            32      // Shared Peripheral Interrupts (32+)

/**
 * GIC controller structure
 */
struct gic_controller {
    volatile uint32_t *distributor_base;    // GICD base address
    volatile uint32_t *cpu_interface_base;  // GICC base address
    uint32_t num_irqs;                     // Number of supported IRQs
    uint32_t num_cpus;                     // Number of CPUs
};

// GIC controller instance
static struct gic_controller gic_controller_instance;
static int gic_initialized = 0;

// Default GIC base addresses (typical QEMU values)
#define GIC_DIST_BASE           0x08000000
#define GIC_CPU_BASE            0x08010000

// Helper functions for GIC register access
static inline uint32_t gic_dist_read(uint32_t offset) {
    return gic_controller_instance.distributor_base[offset / 4];
}

static inline void gic_dist_write(uint32_t offset, uint32_t value) {
    gic_controller_instance.distributor_base[offset / 4] = value;
}

static inline uint32_t gic_cpu_read(uint32_t offset) {
    return gic_controller_instance.cpu_interface_base[offset / 4];
}

static inline void gic_cpu_write(uint32_t offset, uint32_t value) {
    gic_controller_instance.cpu_interface_base[offset / 4] = value;
}

// GIC controller operations
static int gic_controller_init(void)
{
    early_print("GIC: Initializing Generic Interrupt Controller...\n");
    
    // Read GIC type register to determine number of IRQs
    uint32_t typer = gic_dist_read(GICD_TYPER);
    uint32_t num_irqs = ((typer & 0x1F) + 1) * 32;
    uint32_t num_cpus = ((typer >> 5) & 0x7) + 1;
    
    gic_controller_instance.num_irqs = num_irqs > GIC_MAX_IRQS ? GIC_MAX_IRQS : num_irqs;
    gic_controller_instance.num_cpus = num_cpus;
    
    early_print("GIC: Found ");
    // Convert num_irqs to string
    char irq_str[16];
    int pos = 0;
    uint32_t irqs = gic_controller_instance.num_irqs;
    if (irqs == 0) {
        irq_str[pos++] = '0';
    } else {
        char temp[16];
        int temp_pos = 0;
        while (irqs > 0) {
            temp[temp_pos++] = '0' + (irqs % 10);
            irqs /= 10;
        }
        while (temp_pos > 0) {
            irq_str[pos++] = temp[--temp_pos];
        }
    }
    irq_str[pos] = 0;
    early_print(irq_str);
    early_print(" IRQs, ");
    
    // Convert num_cpus to string
    char cpu_str[16];
    pos = 0;
    uint32_t cpus = gic_controller_instance.num_cpus;
    if (cpus == 0) {
        cpu_str[pos++] = '0';
    } else {
        char temp[16];
        int temp_pos = 0;
        while (cpus > 0) {
            temp[temp_pos++] = '0' + (cpus % 10);
            cpus /= 10;
        }
        while (temp_pos > 0) {
            cpu_str[pos++] = temp[--temp_pos];
        }
    }
    cpu_str[pos] = 0;
    early_print(cpu_str);
    early_print(" CPUs\n");
    
    // Disable distributor
    gic_dist_write(GICD_CTLR, 0);
    
    // Disable all interrupts
    for (uint32_t i = 0; i < gic_controller_instance.num_irqs; i += 32) {
        gic_dist_write(GICD_ICENABLER + (i / 32) * 4, 0xFFFFFFFF);
        gic_dist_write(GICD_ICPENDR + (i / 32) * 4, 0xFFFFFFFF);
        gic_dist_write(GICD_ICACTIVER + (i / 32) * 4, 0xFFFFFFFF);
    }
    
    // Set all interrupts to group 1 (non-secure)
    for (uint32_t i = 0; i < gic_controller_instance.num_irqs; i += 32) {
        gic_dist_write(GICD_IGROUPR + (i / 32) * 4, 0xFFFFFFFF);
    }
    
    // Set default priority (middle priority) for all interrupts
    for (uint32_t i = 0; i < gic_controller_instance.num_irqs; i += 4) {
        gic_dist_write(GICD_IPRIORITYR + i, 0x80808080);
    }
    
    // Set all SPI (Shared Peripheral Interrupts) to target CPU 0
    for (uint32_t i = GIC_SPI_BASE; i < gic_controller_instance.num_irqs; i += 4) {
        gic_dist_write(GICD_ITARGETSR + i, 0x01010101);
    }
    
    // Set all interrupts as level-triggered (most common)
    for (uint32_t i = 0; i < gic_controller_instance.num_irqs; i += 16) {
        gic_dist_write(GICD_ICFGR + (i / 16) * 4, 0x00000000);
    }
    
    // Enable distributor
    gic_dist_write(GICD_CTLR, 1);
    
    // Configure CPU interface
    // Set priority mask to lowest priority (allow all interrupts)
    gic_cpu_write(GICC_PMR, 0xFF);
    
    // Set binary point to 0 (no priority grouping)
    gic_cpu_write(GICC_BPR, 0);
    
    // Enable CPU interface
    gic_cpu_write(GICC_CTLR, 1);
    
    early_print("GIC: Initialization complete\n");
    return 0;
}

static void gic_enable_irq(uint32_t irq)
{
    if (irq >= gic_controller_instance.num_irqs) {
        return;
    }
    
    uint32_t reg = GICD_ISENABLER + (irq / 32) * 4;
    uint32_t bit = 1 << (irq % 32);
    gic_dist_write(reg, bit);
}

static void gic_disable_irq(uint32_t irq)
{
    if (irq >= gic_controller_instance.num_irqs) {
        return;
    }
    
    uint32_t reg = GICD_ICENABLER + (irq / 32) * 4;
    uint32_t bit = 1 << (irq % 32);
    gic_dist_write(reg, bit);
}

static void gic_set_priority(uint32_t irq, uint8_t priority)
{
    if (irq >= gic_controller_instance.num_irqs) {
        return;
    }
    
    uint32_t reg = GICD_IPRIORITYR + irq;
    uint32_t current = gic_dist_read(reg & ~3);
    uint32_t shift = (irq % 4) * 8;
    uint32_t mask = 0xFF << shift;
    
    current = (current & ~mask) | ((uint32_t)priority << shift);
    gic_dist_write(reg & ~3, current);
}

static void gic_set_type(uint32_t irq, uint32_t type)
{
    if (irq >= gic_controller_instance.num_irqs) {
        return;
    }
    
    // Only SPIs (>= 32) can have their type configured
    if (irq < GIC_SPI_BASE) {
        return;
    }
    
    uint32_t reg = GICD_ICFGR + (irq / 16) * 4;
    uint32_t current = gic_dist_read(reg);
    uint32_t shift = ((irq % 16) * 2) + 1;
    uint32_t mask = 1 << shift;
    
    if (type == IRQ_TYPE_EDGE || type == IRQ_TYPE_RISING_EDGE) {
        current |= mask;  // Set bit for edge-triggered
    } else {
        current &= ~mask; // Clear bit for level-triggered
    }
    
    gic_dist_write(reg, current);
}

static uint32_t gic_get_pending(void)
{
    return gic_cpu_read(GICC_IAR) & 0x3FF; // Interrupt ID is in lower 10 bits
}

static void gic_clear_pending(uint32_t irq)
{
    if (irq >= gic_controller_instance.num_irqs) {
        return;
    }
    
    uint32_t reg = GICD_ICPENDR + (irq / 32) * 4;
    uint32_t bit = 1 << (irq % 32);
    gic_dist_write(reg, bit);
}

static void gic_send_eoi(uint32_t irq)
{
    gic_cpu_write(GICC_EOIR, irq);
}

// GIC interrupt controller interface
static struct interrupt_controller gic_interrupt_controller = {
    .name = "ARM-GICv2",
    .num_irqs = 0,  // Will be set during init
    .base_irq = 0,
    .init = gic_controller_init,
    .enable_irq = gic_enable_irq,
    .disable_irq = gic_disable_irq,
    .set_priority = gic_set_priority,
    .set_type = gic_set_type,
    .get_pending = gic_get_pending,
    .clear_pending = gic_clear_pending,
    .send_eoi = gic_send_eoi,
};

int gic_init(void)
{
    if (gic_initialized) {
        return 0;
    }
    
    early_print("Initializing ARM64 GIC controller...\n");
    
    // Set up GIC controller base addresses
    gic_controller_instance.distributor_base = (volatile uint32_t *)GIC_DIST_BASE;
    gic_controller_instance.cpu_interface_base = (volatile uint32_t *)GIC_CPU_BASE;
    
    // Initialize the controller
    if (gic_controller_init() < 0) {
        early_print("GIC: Failed to initialize controller\n");
        return -1;
    }
    
    // Update controller info
    gic_interrupt_controller.num_irqs = gic_controller_instance.num_irqs;
    
    gic_initialized = 1;
    early_print("ARM64 GIC controller initialized successfully\n");
    
    return 0;
}

int gic_controller_register(void)
{
    if (!gic_initialized) {
        return -1;
    }
    
    return interrupt_controller_register(&gic_interrupt_controller);
}

// GIC-specific interrupt handling functions
uint32_t gic_acknowledge_irq(void)
{
    return gic_cpu_read(GICC_IAR) & 0x3FF;
}

void gic_end_of_interrupt(uint32_t irq)
{
    gic_cpu_write(GICC_EOIR, irq);
}

void gic_show_status(void)
{
    if (!gic_initialized) {
        early_print("GIC not initialized\n");
        return;
    }
    
    early_print("=== ARM64 GIC Status ===\n");
    early_print("Distributor Control: ");
    uint32_t dist_ctrl = gic_dist_read(GICD_CTLR);
    early_print(dist_ctrl & 1 ? "Enabled" : "Disabled");
    early_print("\n");
    
    early_print("CPU Interface Control: ");
    uint32_t cpu_ctrl = gic_cpu_read(GICC_CTLR);
    early_print(cpu_ctrl & 1 ? "Enabled" : "Disabled");
    early_print("\n");
    
    early_print("Priority Mask: 0x");
    uint32_t pmr = gic_cpu_read(GICC_PMR);
    // Convert to hex string
    char hex_str[9];
    for (int i = 7; i >= 0; i--) {
        uint8_t nibble = (pmr >> (i * 4)) & 0xF;
        hex_str[7-i] = nibble < 10 ? '0' + nibble : 'A' + nibble - 10;
    }
    hex_str[8] = 0;
    early_print(hex_str);
    early_print("\n");
    
    // Convert numbers to strings for output
    char irq_str[16], cpu_str[16];
    int pos = 0;
    
    // num_irqs to string
    uint32_t irqs = gic_controller_instance.num_irqs;
    if (irqs == 0) {
        irq_str[pos++] = '0';
    } else {
        char temp[16];
        int temp_pos = 0;
        while (irqs > 0) {
            temp[temp_pos++] = '0' + (irqs % 10);
            irqs /= 10;
        }
        while (temp_pos > 0) {
            irq_str[pos++] = temp[--temp_pos];
        }
    }
    irq_str[pos] = 0;
    
    // num_cpus to string
    pos = 0;
    uint32_t cpus = gic_controller_instance.num_cpus;
    if (cpus == 0) {
        cpu_str[pos++] = '0';
    } else {
        char temp[16];
        int temp_pos = 0;
        while (cpus > 0) {
            temp[temp_pos++] = '0' + (cpus % 10);
            cpus /= 10;
        }
        while (temp_pos > 0) {
            cpu_str[pos++] = temp[--temp_pos];
        }
    }
    cpu_str[pos] = 0;
    
    early_print("Supported IRQs: ");
    early_print(irq_str);
    early_print(", CPUs: ");
    early_print(cpu_str);
    early_print("\n");
}

#endif // ARCH_ARM64
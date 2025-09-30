/*
 * MiniOS Interrupt Management Interface
 * Phase 4: Device Drivers & System Services
 */

#ifndef INTERRUPT_H
#define INTERRUPT_H

#include <stdint.h>
#include <stddef.h>

// Forward declarations
struct device;

// Interrupt types
#define IRQ_TYPE_EDGE           0x00    // Edge-triggered interrupt
#define IRQ_TYPE_LEVEL          0x01    // Level-triggered interrupt
#define IRQ_TYPE_RISING_EDGE    0x02    // Rising edge-triggered
#define IRQ_TYPE_FALLING_EDGE   0x03    // Falling edge-triggered

// Interrupt priorities
#define IRQ_PRIORITY_HIGHEST    0
#define IRQ_PRIORITY_HIGH       64
#define IRQ_PRIORITY_NORMAL     128
#define IRQ_PRIORITY_LOW        192
#define IRQ_PRIORITY_LOWEST     255

// Interrupt handler function type
typedef void (*interrupt_handler_t)(uint32_t irq_num, void *context);

// Interrupt controller interface
struct interrupt_controller {
    const char *name;
    uint32_t num_irqs;
    uint32_t base_irq;
    
    // Controller operations
    int (*init)(void);
    void (*enable_irq)(uint32_t irq);
    void (*disable_irq)(uint32_t irq);
    void (*set_priority)(uint32_t irq, uint8_t priority);
    void (*set_type)(uint32_t irq, uint32_t type);
    uint32_t (*get_pending)(void);
    void (*clear_pending)(uint32_t irq);
    void (*send_eoi)(uint32_t irq);
};

// Interrupt descriptor
struct irq_desc {
    uint32_t irq_num;
    interrupt_handler_t handler;
    void *context;
    const char *name;
    uint32_t count;
    uint32_t type;
    uint8_t priority;
    uint32_t flags;
};

// Interrupt subsystem API

/**
 * Initialize interrupt subsystem
 * @return 0 on success, negative on error
 */
int interrupt_init(void);

/**
 * Register interrupt controller
 * @param controller Interrupt controller to register
 * @return 0 on success, negative on error
 */
int interrupt_controller_register(struct interrupt_controller *controller);

/**
 * Request an interrupt line
 * @param irq_num Interrupt number
 * @param handler Interrupt handler function
 * @param context Context pointer passed to handler
 * @param name Interrupt name for debugging
 * @return 0 on success, negative on error
 */
int request_irq(uint32_t irq_num, interrupt_handler_t handler, void *context, const char *name);

/**
 * Free an interrupt line
 * @param irq_num Interrupt number to free
 */
void free_irq(uint32_t irq_num);

/**
 * Enable interrupt
 * @param irq_num Interrupt number
 * @return 0 on success, negative on error
 */
int enable_irq(uint32_t irq_num);

/**
 * Disable interrupt
 * @param irq_num Interrupt number
 * @return 0 on success, negative on error
 */
int disable_irq(uint32_t irq_num);

/**
 * Set interrupt priority
 * @param irq_num Interrupt number
 * @param priority Priority level (0 = highest, 255 = lowest)
 * @return 0 on success, negative on error
 */
int set_irq_priority(uint32_t irq_num, uint8_t priority);

/**
 * Set interrupt type
 * @param irq_num Interrupt number
 * @param type Interrupt type (edge/level triggered)
 * @return 0 on success, negative on error
 */
int set_irq_type(uint32_t irq_num, uint32_t type);

/**
 * Get interrupt statistics
 * @param irq_num Interrupt number
 * @return Interrupt count, 0 if not found
 */
uint32_t get_irq_count(uint32_t irq_num);

/**
 * Disable all interrupts (critical section entry)
 * @return Previous interrupt state
 */
unsigned long disable_interrupts(void);

/**
 * Restore interrupt state (critical section exit)
 * @param flags Previous interrupt state from disable_interrupts()
 */
void restore_interrupts(unsigned long flags);

/**
 * Check if interrupts are enabled
 * @return 1 if enabled, 0 if disabled
 */
int interrupts_enabled(void);

/**
 * Handle interrupt (called from low-level interrupt entry)
 * @param irq_num Interrupt number
 */
void handle_interrupt(uint32_t irq_num);

// Debug and statistics functions
/**
 * Show interrupt statistics
 */
void show_interrupt_stats(void);

/**
 * Show registered interrupt controllers
 */
void show_interrupt_controllers(void);

// Architecture-specific functions

/**
 * Initialize architecture-specific interrupt handling
 * @return 0 on success, negative on error
 */
int arch_interrupt_init(void);

/**
 * Architecture-specific interrupt enable/disable
 */
unsigned long arch_disable_interrupts(void);
void arch_restore_interrupts(unsigned long flags);
int arch_interrupts_enabled(void);

#ifdef ARCH_ARM64
/**
 * ARM64 GIC (Generic Interrupt Controller) interface
 */

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

/**
 * Initialize ARM64 GIC controller
 * @return 0 on success, negative on error
 */
int gic_init(void);

/**
 * Register GIC controller with interrupt subsystem
 * @return 0 on success, negative on error
 */
int gic_controller_register(void);

#endif // ARCH_ARM64

#ifdef ARCH_X86_64
/**
 * x86-64 Interrupt Controller interfaces (PIC/APIC)
 */

// PIC (8259) constants
#define PIC1_COMMAND            0x20    // Master PIC command port
#define PIC1_DATA               0x21    // Master PIC data port
#define PIC2_COMMAND            0xA0    // Slave PIC command port
#define PIC2_DATA               0xA1    // Slave PIC data port

#define PIC_EOI                 0x20    // End of interrupt command

#define ICW1_ICW4               0x01    // ICW4 (not) needed
#define ICW1_SINGLE             0x02    // Single (cascade) mode
#define ICW1_INTERVAL4          0x04    // Call address interval 4 (8)
#define ICW1_LEVEL              0x08    // Level triggered (edge) mode
#define ICW1_INIT               0x10    // Initialization - required!

#define ICW4_8086               0x01    // 8086/88 (MCS-80/85) mode
#define ICW4_AUTO               0x02    // Auto (normal) EOI
#define ICW4_BUF_SLAVE          0x08    // Buffered mode/slave
#define ICW4_BUF_MASTER         0x0C    // Buffered mode/master
#define ICW4_SFNM               0x10    // Special fully nested (not)

// IDT entry structure
struct idt_entry {
    uint16_t offset_low;        // Lower 16 bits of handler address
    uint16_t selector;          // Code segment selector
    uint8_t ist;                // Interrupt Stack Table offset
    uint8_t type_attr;          // Type and attributes
    uint16_t offset_middle;     // Middle 16 bits of handler address
    uint32_t offset_high;       // Upper 32 bits of handler address
    uint32_t zero;              // Reserved
} __attribute__((packed));

// IDT descriptor
struct idt_descriptor {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

// IDT constants
#define IDT_ENTRIES             256
#define IDT_INTERRUPT_GATE      0x8E
#define IDT_TRAP_GATE           0x8F

/**
 * Initialize x86-64 PIC controller
 * @return 0 on success, negative on error
 */
int pic_init(void);

/**
 * Initialize x86-64 IDT (Interrupt Descriptor Table)
 * @return 0 on success, negative on error
 */
int idt_init(void);

/**
 * Register PIC controller with interrupt subsystem
 * @return 0 on success, negative on error
 */
int pic_controller_register(void);

/**
 * Set IDT entry
 * @param vector Interrupt vector number
 * @param handler Handler address
 * @param selector Code segment selector
 * @param type_attr Type and attributes
 */
void set_idt_entry(uint8_t vector, uint64_t handler, uint16_t selector, uint8_t type_attr);

#endif // ARCH_X86_64

#endif // INTERRUPT_H
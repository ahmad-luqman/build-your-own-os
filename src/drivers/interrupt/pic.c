/*
 * MiniOS x86-64 PIC (Programmable Interrupt Controller) Driver
 * Phase 4: Device Drivers & System Services
 */

#include "interrupt.h"
#include "memory.h"
#include "kernel.h"

#ifdef ARCH_X86_64

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

// PIC controller state
static int pic_initialized = 0;

// I/O port access functions
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

// Wait for PIC operation to complete
static void pic_wait(void) {
    // Port 0x80 is used for 'short' delay
    outb(0x80, 0);
}

// PIC controller operations
static int pic_controller_init(void)
{
    early_print("PIC: Initializing 8259 Programmable Interrupt Controller...\n");
    
    // Save current interrupt masks
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);
    (void)mask1;  // Suppress unused variable warning
    (void)mask2;  // Suppress unused variable warning
    
    // Start initialization sequence (ICW1)
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    pic_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    pic_wait();
    
    // Set interrupt vector offsets (ICW2)
    outb(PIC1_DATA, 0x20);      // Master PIC: IRQ 0-7 -> INT 0x20-0x27
    pic_wait();
    outb(PIC2_DATA, 0x28);      // Slave PIC: IRQ 8-15 -> INT 0x28-0x2F
    pic_wait();
    
    // Configure cascade (ICW3)
    outb(PIC1_DATA, 4);         // Master: Slave connected to IRQ2
    pic_wait();
    outb(PIC2_DATA, 2);         // Slave: Connected to master's IRQ2
    pic_wait();
    
    // Set mode (ICW4)
    outb(PIC1_DATA, ICW4_8086);
    pic_wait();
    outb(PIC2_DATA, ICW4_8086);
    pic_wait();
    
    // Disable all interrupts initially
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
    
    early_print("PIC: Initialization complete\n");
    return 0;
}

static void pic_enable_irq(uint32_t irq)
{
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    
    value = inb(port) & ~(1 << irq);
    outb(port, value);
}

static void pic_disable_irq(uint32_t irq)
{
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    
    value = inb(port) | (1 << irq);
    outb(port, value);
}

static void pic_set_priority(uint32_t irq, uint8_t priority)
{
    // PIC doesn't support individual interrupt priorities
    // Priority is determined by IRQ number (0 = highest, 7/15 = lowest)
    (void)irq;
    (void)priority;
}

static void pic_set_type(uint32_t irq, uint32_t type)
{
    // PIC doesn't support configurable interrupt types
    // All interrupts are edge-triggered
    (void)irq;
    (void)type;
}

static uint32_t pic_get_pending(void)
{
    // Read Interrupt Request Register (IRR) to see pending interrupts
    outb(PIC1_COMMAND, 0x0A);  // OCW3: Read IRR
    uint8_t irr1 = inb(PIC1_COMMAND);
    
    outb(PIC2_COMMAND, 0x0A);  // OCW3: Read IRR
    uint8_t irr2 = inb(PIC2_COMMAND);
    
    uint16_t pending = irr1 | (irr2 << 8);
    
    // Return the lowest pending interrupt number
    for (int i = 0; i < 16; i++) {
        if (pending & (1 << i)) {
            return i;
        }
    }
    
    return 0xFFFFFFFF; // No pending interrupts
}

static void pic_clear_pending(uint32_t irq)
{
    // PIC automatically clears pending interrupts when acknowledged
    (void)irq;
}

static void pic_send_eoi(uint32_t irq)
{
    // Send End of Interrupt (EOI) command
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

// PIC interrupt controller interface
static struct interrupt_controller pic_interrupt_controller = {
    .name = "Intel-8259-PIC",
    .num_irqs = 16,
    .base_irq = 0,
    .init = pic_controller_init,
    .enable_irq = pic_enable_irq,
    .disable_irq = pic_disable_irq,
    .set_priority = pic_set_priority,
    .set_type = pic_set_type,
    .get_pending = pic_get_pending,
    .clear_pending = pic_clear_pending,
    .send_eoi = pic_send_eoi,
};

int pic_init(void)
{
    if (pic_initialized) {
        return 0;
    }
    
    early_print("Initializing x86-64 PIC controller...\n");
    
    // Initialize the controller
    if (pic_controller_init() < 0) {
        early_print("PIC: Failed to initialize controller\n");
        return -1;
    }
    
    pic_initialized = 1;
    early_print("x86-64 PIC controller initialized successfully\n");
    
    return 0;
}

int pic_controller_register(void)
{
    if (!pic_initialized) {
        return -1;
    }
    
    return interrupt_controller_register(&pic_interrupt_controller);
}

// PIC utility functions
void pic_mask_all(void)
{
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

void pic_unmask_all(void)
{
    outb(PIC1_DATA, 0x00);
    outb(PIC2_DATA, 0x00);
}

uint16_t pic_get_mask(void)
{
    return inb(PIC1_DATA) | (inb(PIC2_DATA) << 8);
}

void pic_set_mask(uint16_t mask)
{
    outb(PIC1_DATA, mask & 0xFF);
    outb(PIC2_DATA, (mask >> 8) & 0xFF);
}

void pic_show_status(void)
{
    if (!pic_initialized) {
        early_print("PIC not initialized\n");
        return;
    }
    
    early_print("=== x86-64 PIC Status ===\n");
    
    uint16_t mask = pic_get_mask();
    early_print("Interrupt Mask: 0x");
    
    // Convert mask to hex string
    char hex_str[5];
    for (int i = 3; i >= 0; i--) {
        uint8_t nibble = (mask >> (i * 4)) & 0xF;
        hex_str[3-i] = nibble < 10 ? '0' + nibble : 'A' + nibble - 10;
    }
    hex_str[4] = 0;
    early_print(hex_str);
    early_print("\n");
    
    // Show enabled/disabled status
    early_print("Enabled IRQs: ");
    int first = 1;
    for (int i = 0; i < 16; i++) {
        if (!(mask & (1 << i))) {
            if (!first) {
                early_print(", ");
            }
            char irq_str[8];
            if (i < 10) {
                irq_str[0] = '0' + i;
                irq_str[1] = 0;
            } else {
                irq_str[0] = '1';
                irq_str[1] = '0' + (i - 10);
                irq_str[2] = 0;
            }
            early_print(irq_str);
            first = 0;
        }
    }
    if (first) {
        early_print("None");
    }
    early_print("\n");
    
    // Read and display pending interrupts
    uint32_t pending = pic_get_pending();
    early_print("Pending IRQ: ");
    if (pending == 0xFFFFFFFF) {
        early_print("None");
    } else {
        char pending_str[8];
        if (pending < 10) {
            pending_str[0] = '0' + pending;
            pending_str[1] = 0;
        } else {
            pending_str[0] = '1';
            pending_str[1] = '0' + (pending - 10);
            pending_str[2] = 0;
        }
        early_print(pending_str);
    }
    early_print("\n");
}

#endif // ARCH_X86_64
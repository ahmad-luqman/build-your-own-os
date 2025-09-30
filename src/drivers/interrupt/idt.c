/*
 * MiniOS x86-64 IDT (Interrupt Descriptor Table) Implementation
 * Phase 4: Device Drivers & System Services
 */

#include "interrupt.h"
#include "memory.h"
#include "kernel.h"

#ifdef ARCH_X86_64

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

// I/O port access functions
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline void outb_eoi(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

// IDT table and descriptor
static struct idt_entry idt[IDT_ENTRIES];
static struct idt_descriptor idt_desc;
static int idt_initialized = 0;

// External assembly interrupt stubs (will be implemented)
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);

// IRQ handlers
extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

void set_idt_entry(uint8_t vector, uint64_t handler, uint16_t selector, uint8_t type_attr)
{
    idt[vector].offset_low = handler & 0xFFFF;
    idt[vector].selector = selector;
    idt[vector].ist = 0;
    idt[vector].type_attr = type_attr;
    idt[vector].offset_middle = (handler >> 16) & 0xFFFF;
    idt[vector].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[vector].zero = 0;
}

// Load IDT
static void load_idt(void)
{
    idt_desc.limit = sizeof(idt) - 1;
    idt_desc.base = (uint64_t)&idt;
    
    __asm__ volatile("lidtq %0" : : "m"(idt_desc));
}

int idt_init(void)
{
    if (idt_initialized) {
        return 0;
    }
    
    early_print("IDT: Initializing Interrupt Descriptor Table...\n");
    
    // Clear IDT
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt[i].offset_low = 0;
        idt[i].selector = 0;
        idt[i].ist = 0;
        idt[i].type_attr = 0;
        idt[i].offset_middle = 0;
        idt[i].offset_high = 0;
        idt[i].zero = 0;
    }
    
    // For now, set up basic exception handlers (stubs)
    // In a full implementation, these would be actual assembly stubs
    
    // CPU exceptions (0-31)
    for (int i = 0; i < 32; i++) {
        // Use a default handler address (will cause triple fault if called)
        // In real implementation, these would be proper interrupt stubs
        set_idt_entry(i, 0, 0x08, IDT_INTERRUPT_GATE);
    }
    
    // Hardware IRQs (32-47) - mapped from PIC IRQ 0-15
    for (int i = 32; i < 48; i++) {
        // Use a default handler address
        set_idt_entry(i, 0, 0x08, IDT_INTERRUPT_GATE);
    }
    
    // Load the IDT
    load_idt();
    
    idt_initialized = 1;
    early_print("IDT: Initialization complete\n");
    
    return 0;
}

// Basic interrupt handlers (stubs for now)
void default_exception_handler(uint32_t exception, uint64_t error_code)
{
    early_print("Exception ");
    
    // Convert exception number to string
    char exc_str[8];
    if (exception < 10) {
        exc_str[0] = '0' + exception;
        exc_str[1] = 0;
    } else {
        exc_str[0] = '0' + (exception / 10);
        exc_str[1] = '0' + (exception % 10);
        exc_str[2] = 0;
    }
    early_print(exc_str);
    
    early_print(" occurred");
    
    if (error_code != 0) {
        early_print(" with error code 0x");
        
        // Convert error code to hex
        char hex_str[17];
        for (int i = 15; i >= 0; i--) {
            uint8_t nibble = (error_code >> (i * 4)) & 0xF;
            hex_str[15-i] = nibble < 10 ? '0' + nibble : 'A' + nibble - 10;
        }
        hex_str[16] = 0;
        early_print(hex_str);
    }
    
    early_print("\n");
    early_print("System halted.\n");
    
    // Halt the system
    __asm__ volatile("cli; hlt");
    while(1);
}

void default_irq_handler(uint32_t irq)
{
    early_print("IRQ ");
    
    // Convert IRQ number to string
    char irq_str[8];
    if (irq < 10) {
        irq_str[0] = '0' + irq;
        irq_str[1] = 0;
    } else {
        irq_str[0] = '0' + (irq / 10);
        irq_str[1] = '0' + (irq % 10);
        irq_str[2] = 0;
    }
    early_print(irq_str);
    early_print(" received\n");
    
    // Send EOI to PIC
    if (irq >= 8) {
        outb_eoi(0xA0, 0x20); // Send EOI to slave PIC
    }
    outb_eoi(0x20, 0x20); // Send EOI to master PIC
}



void idt_show_status(void)
{
    if (!idt_initialized) {
        early_print("IDT not initialized\n");
        return;
    }
    
    early_print("=== x86-64 IDT Status ===\n");
    early_print("IDT Base: 0x");
    
    // Convert base address to hex
    uint64_t base = idt_desc.base;
    char hex_str[17];
    for (int i = 15; i >= 0; i--) {
        uint8_t nibble = (base >> (i * 4)) & 0xF;
        hex_str[15-i] = nibble < 10 ? '0' + nibble : 'A' + nibble - 10;
    }
    hex_str[16] = 0;
    early_print(hex_str);
    early_print("\n");
    
    early_print("IDT Limit: ");
    // Convert limit to string
    uint16_t limit = idt_desc.limit;
    char limit_str[8];
    int pos = 0;
    if (limit == 0) {
        limit_str[pos++] = '0';
    } else {
        char temp[8];
        int temp_pos = 0;
        while (limit > 0) {
            temp[temp_pos++] = '0' + (limit % 10);
            limit /= 10;
        }
        while (temp_pos > 0) {
            limit_str[pos++] = temp[--temp_pos];
        }
    }
    limit_str[pos] = 0;
    early_print(limit_str);
    early_print(" (");
    
    // Calculate number of entries
    uint16_t entries = (idt_desc.limit + 1) / 16;
    char entries_str[8];
    pos = 0;
    if (entries == 0) {
        entries_str[pos++] = '0';
    } else {
        char temp[8];
        int temp_pos = 0;
        while (entries > 0) {
            temp[temp_pos++] = '0' + (entries % 10);
            entries /= 10;
        }
        while (temp_pos > 0) {
            entries_str[pos++] = temp[--temp_pos];
        }
    }
    entries_str[pos] = 0;
    early_print(entries_str);
    early_print(" entries)\n");
    
    // Count configured entries
    int configured = 0;
    for (int i = 0; i < IDT_ENTRIES; i++) {
        if (idt[i].offset_low != 0 || idt[i].offset_middle != 0 || idt[i].offset_high != 0) {
            configured++;
        }
    }
    
    early_print("Configured entries: ");
    char config_str[8];
    pos = 0;
    if (configured == 0) {
        config_str[pos++] = '0';
    } else {
        char temp[8];
        int temp_pos = 0;
        while (configured > 0) {
            temp[temp_pos++] = '0' + (configured % 10);
            configured /= 10;
        }
        while (temp_pos > 0) {
            config_str[pos++] = temp[--temp_pos];
        }
    }
    config_str[pos] = 0;
    early_print(config_str);
    early_print("\n");
}

#endif // ARCH_X86_64
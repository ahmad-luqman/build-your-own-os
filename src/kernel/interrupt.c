/*
 * MiniOS Interrupt Subsystem Implementation
 * Phase 4: Device Drivers & System Services
 */

#include "interrupt.h"
#include "memory.h"
#include "kernel.h"

// Interrupt subsystem state
static int interrupt_subsystem_initialized = 0;
static struct interrupt_controller *controllers[4];
static int num_controllers = 0;
static struct irq_desc irq_descriptors[64];  // Reduced from 256 for debugging

// Architecture-specific functions
#ifdef ARCH_ARM64
extern int gic_init(void);
extern int gic_controller_register(void);
extern void gic_show_status(void);
#endif

#ifdef ARCH_X86_64
extern int pic_init(void);
extern int idt_init(void);
extern int pic_controller_register(void);
extern void pic_show_status(void);
extern void idt_show_status(void);
#endif

int interrupt_init(void)
{
    early_print("Initializing interrupt subsystem...\n");
    early_print("Interrupt: Function entered successfully\n");

    // TEMPORARILY SKIP INTERRUPT INITIALIZATION FOR PHASE 4 DEBUGGING
    early_print("Interrupt: Skipping interrupt initialization for debugging\n");
    interrupt_subsystem_initialized = 1;
    early_print("Interrupt subsystem initialized (stub mode)\n");
    return 0;
}

int interrupt_controller_register(struct interrupt_controller *controller)
{
    if (!controller || num_controllers >= 4) {
        return -1;
    }
    
    early_print("Registering interrupt controller: ");
    early_print(controller->name);
    early_print("\n");
    
    controllers[num_controllers] = controller;
    num_controllers++;
    
    early_print("Controller registered successfully\n");
    return 0;
}

int request_irq(uint32_t irq_num, interrupt_handler_t handler, void *context, const char *name)
{
    if (!interrupt_subsystem_initialized || irq_num >= 256 || !handler) {
        return -1;
    }
    
    // Check if IRQ is already in use
    if (irq_descriptors[irq_num].handler != NULL) {
        return -1;
    }
    
    // Set up interrupt descriptor
    irq_descriptors[irq_num].handler = handler;
    irq_descriptors[irq_num].context = context;
    irq_descriptors[irq_num].name = name;
    irq_descriptors[irq_num].count = 0;
    
    return 0;
}

void free_irq(uint32_t irq_num)
{
    if (!interrupt_subsystem_initialized || irq_num >= 256) {
        return;
    }
    
    // Disable the interrupt
    disable_irq(irq_num);
    
    // Clear interrupt descriptor
    irq_descriptors[irq_num].handler = NULL;
    irq_descriptors[irq_num].context = NULL;
    irq_descriptors[irq_num].name = NULL;
    irq_descriptors[irq_num].count = 0;
}

int enable_irq(uint32_t irq_num)
{
    if (!interrupt_subsystem_initialized || irq_num >= 256) {
        return -1;
    }
    
    // Find appropriate controller and enable IRQ
    for (int i = 0; i < num_controllers; i++) {
        if (controllers[i] && 
            irq_num >= controllers[i]->base_irq &&
            irq_num < controllers[i]->base_irq + controllers[i]->num_irqs) {
            
            if (controllers[i]->enable_irq) {
                controllers[i]->enable_irq(irq_num - controllers[i]->base_irq);
            }
            return 0;
        }
    }
    
    return -1;
}

int disable_irq(uint32_t irq_num)
{
    if (!interrupt_subsystem_initialized || irq_num >= 256) {
        return -1;
    }
    
    // Find appropriate controller and disable IRQ
    for (int i = 0; i < num_controllers; i++) {
        if (controllers[i] && 
            irq_num >= controllers[i]->base_irq &&
            irq_num < controllers[i]->base_irq + controllers[i]->num_irqs) {
            
            if (controllers[i]->disable_irq) {
                controllers[i]->disable_irq(irq_num - controllers[i]->base_irq);
            }
            return 0;
        }
    }
    
    return -1;
}

int set_irq_priority(uint32_t irq_num, uint8_t priority)
{
    if (!interrupt_subsystem_initialized || irq_num >= 256) {
        return -1;
    }
    
    irq_descriptors[irq_num].priority = priority;
    
    // Find appropriate controller and set priority
    for (int i = 0; i < num_controllers; i++) {
        if (controllers[i] && 
            irq_num >= controllers[i]->base_irq &&
            irq_num < controllers[i]->base_irq + controllers[i]->num_irqs) {
            
            if (controllers[i]->set_priority) {
                controllers[i]->set_priority(irq_num - controllers[i]->base_irq, priority);
            }
            return 0;
        }
    }
    
    return -1;
}

int set_irq_type(uint32_t irq_num, uint32_t type)
{
    if (!interrupt_subsystem_initialized || irq_num >= 256) {
        return -1;
    }
    
    irq_descriptors[irq_num].type = type;
    
    // Find appropriate controller and set type
    for (int i = 0; i < num_controllers; i++) {
        if (controllers[i] && 
            irq_num >= controllers[i]->base_irq &&
            irq_num < controllers[i]->base_irq + controllers[i]->num_irqs) {
            
            if (controllers[i]->set_type) {
                controllers[i]->set_type(irq_num - controllers[i]->base_irq, type);
            }
            return 0;
        }
    }
    
    return -1;
}

uint32_t get_irq_count(uint32_t irq_num)
{
    if (!interrupt_subsystem_initialized || irq_num >= 256) {
        return 0;
    }
    
    return irq_descriptors[irq_num].count;
}

unsigned long disable_interrupts(void)
{
    return arch_disable_interrupts();
}

void restore_interrupts(unsigned long flags)
{
    arch_restore_interrupts(flags);
}

int interrupts_enabled(void)
{
    return arch_interrupts_enabled();
}

void handle_interrupt(uint32_t irq_num)
{
    if (!interrupt_subsystem_initialized || irq_num >= 256) {
        return;
    }
    
    // Increment interrupt count
    irq_descriptors[irq_num].count++;
    
    // Call registered handler if available
    if (irq_descriptors[irq_num].handler) {
        irq_descriptors[irq_num].handler(irq_num, irq_descriptors[irq_num].context);
    }
    
    // Send EOI to appropriate controller
    for (int i = 0; i < num_controllers; i++) {
        if (controllers[i] && 
            irq_num >= controllers[i]->base_irq &&
            irq_num < controllers[i]->base_irq + controllers[i]->num_irqs) {
            
            if (controllers[i]->send_eoi) {
                controllers[i]->send_eoi(irq_num - controllers[i]->base_irq);
            }
            break;
        }
    }
}

void show_interrupt_stats(void)
{
    if (!interrupt_subsystem_initialized) {
        early_print("Interrupt subsystem not initialized\n");
        return;
    }
    
    early_print("=== Interrupt Statistics ===\n");
    early_print("Interrupt subsystem: Active\n");
}

void show_interrupt_controllers(void)
{
    if (!interrupt_subsystem_initialized) {
        early_print("Interrupt subsystem not initialized\n");
        return;
    }
    
    early_print("=== Interrupt Controllers ===\n");
    
    for (int i = 0; i < num_controllers; i++) {
        if (controllers[i]) {
            early_print("Controller: ");
            early_print(controllers[i]->name);
            early_print("\n");
        }
    }
    
    // Show architecture-specific controller status
#ifdef ARCH_ARM64
    gic_show_status();
#endif

#ifdef ARCH_X86_64
    pic_show_status();
    idt_show_status();
#endif
}

// Weak implementations of architecture-specific functions
int __attribute__((weak)) arch_interrupt_init(void)
{
    early_print("Warning: Using weak arch_interrupt_init\n");
    return 0;
}

unsigned long __attribute__((weak)) arch_disable_interrupts(void)
{
    // Default: do nothing
    return 0;
}

void __attribute__((weak)) arch_restore_interrupts(unsigned long flags)
{
    // Default: do nothing
    (void)flags;
}

int __attribute__((weak)) arch_interrupts_enabled(void)
{
    // Default: assume enabled
    return 1;
}
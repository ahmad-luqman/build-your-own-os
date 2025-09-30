/*
 * x86-64 Bootloader Linker Script
 * For standalone bootloader binary
 */

ENTRY(_start)

SECTIONS
{
    . = 1M;

    .multiboot : {
        *(.multiboot)
    }

    .text : {
        *(.text*)
    }

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data*)
    }

    .bss : {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    }

    __kernel_start = .;
    __kernel_end = .;
    
    /* Provide symbols for assembly code */
    PROVIDE(__bss_start = __bss_start);
    PROVIDE(__bss_end = __bss_end);
}
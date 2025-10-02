/*
 * x86-64 Kernel Linker Script
 */

ENTRY(_start)

PHDRS
{
    note PT_NOTE FLAGS(0x4);
    text PT_LOAD FLAGS(0x5);
    data PT_LOAD FLAGS(0x6);
}

SECTIONS
{
    . = 1M;
    __kernel_start = .;

    /* Place NOTE section first for PVH boot compatibility */
    .note : ALIGN(4) {
        *(.note*)
    } :note :text

    .multiboot : {
        *(.multiboot)
    } :text

    .text : {
        *(.text*)
    } :text

    .rodata : {
        *(.rodata*)
    } :text

    .data : {
        *(.data*)
    } :data

    .bss : {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    } :data

    __kernel_end = .;
}

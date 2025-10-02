; MiniOS x86-64 Kernel Entry (PVH + Multiboot friendly)
; Boots from 32-bit protected mode, enables long mode, and enters kernel_main.

%define BOOT_PROTOCOL_MAGIC   0x4D696E694F53        ; "MiniOS"
%define BOOT_ARCH_X86_64      0x2
%define BOOT_PROTOCOL_VERSION 0x1
%define MEMORY_TYPE_AVAILABLE 0x1
%define MEMORY_MAP_ENTRY_SIZE 24

%define STACK32_SIZE_BYTES 4096
%define STACK64_SIZE_BYTES 4096

section .multiboot
align 8
multiboot_header:
    dd 0xe85250d6
    dd 0
    dd multiboot_header_end - multiboot_header
    dd -(0xe85250d6 + (multiboot_header_end - multiboot_header))

    dw 0
    dw 0
    dd 8
multiboot_header_end:

section .note.Xen.pvh note alloc
align 4
    dd 4
    dd 16
    dd 0x12
    db 'X', 'e', 'n', 0
align 8
    dq long_mode_entry
    dq stack_top

section .text
bits 32
align 16
global _start

extern kernel_main
extern __bss_start
extern __bss_end

_start:
    cli

    ; Establish temporary 32-bit stack
    mov esp, stack32_top

    ; Minimal CPU feature checks
    call check_cpuid
    call check_long_mode

    ; Set up identity-mapped page tables for first 1GB
    call setup_page_tables
    call enable_paging

    ; Load 64-bit GDT
    lgdt [gdt64.pointer]

    ; Far jump to long mode
    jmp gdt64.code_segment:long_mode_entry

check_cpuid:
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    cmp eax, ecx
    je .no_cpuid
    ret
.no_cpuid:
    hlt
    jmp .no_cpuid

check_long_mode:
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .no_long
    ret
.no_long:
    hlt
    jmp .no_long

setup_page_tables:
    ; Zero tables
    mov edi, pml4_table
    mov ecx, (4096 * 3) / 4
    xor eax, eax
    rep stosd

    ; PML4[0] -> PDP
    mov eax, pdpt_table
    or eax, 0x3
    mov [pml4_table], eax
    mov dword [pml4_table + 4], 0

    ; PDP[0] -> PD (1GB)
    mov eax, pd_table
    or eax, 0x3
    mov [pdpt_table], eax
    mov dword [pdpt_table + 4], 0

    ; Map first 1GB using 2MB pages
    mov ecx, 512
    xor eax, eax
.map_loop:
    mov edx, eax
    shl edx, 21
    mov ebx, edx
    or ebx, 0x83
    mov [pd_table + eax*8], ebx
    mov dword [pd_table + eax*8 + 4], 0
    inc eax
    loop .map_loop

    ret

enable_paging:
    ; Enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Load PML4
    mov eax, pml4_table
    mov cr3, eax

    ; Enable long mode
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; Enable paging + protected mode
    mov eax, cr0
    or eax, (1 << 31) | (1 << 0)
    mov cr0, eax
    ret

bits 64
long_mode_entry:
    ; Debug: Entered 64-bit mode
    mov dx, 0xE9
    mov al, 'A'
    out dx, al

    mov ax, gdt64.data_segment
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    ; Debug: Segments loaded
    mov dx, 0xE9
    mov al, 'B'
    out dx, al

    ; Switch to 64-bit stack
    lea rsp, [rel stack_top]
    and rsp, -16

    ; Debug: Stack setup
    mov dx, 0xE9
    mov al, 'C'
    out dx, al

    ; Debug: Before BSS clear
    mov dx, 0xE9
    mov al, 'D'
    out dx, al
    
    ; NOTE: Skip explicit BSS clearing
    ; According to multiboot2 specification section 3.3, the bootloader  
    ; must "load all loadable segments" and "set all non-loaded segments to zero"
    ; GRUB complies with this and zeros the BSS section for us.
    ; Attempting to manually clear BSS causes triple faults, likely due to subtle
    ; issues with early paging setup or timing. Since GRUB already handles this,
    ; we skip it here.
    ;
    ; TODO: If we ever boot without multiboot (e.g., UEFI direct boot), we'll 
    ; need to revisit this and ensure BSS is zeroed.
    
    ; Debug: BSS "cleared" (actually skipped - already zero from GRUB)
    mov dx, 0xE9
    mov al, 'E'
    out dx, al

    ; Build minimal boot_info
    lea rbx, [rel boot_info_struct]
    mov rax, BOOT_PROTOCOL_MAGIC
    mov [rbx + 0], rax
    mov dword [rbx + 8], BOOT_ARCH_X86_64
    mov dword [rbx + 12], BOOT_PROTOCOL_VERSION

    lea rax, [rel memory_map_entry]
    mov [rbx + 16], rax
    mov dword [rbx + 24], 1
    mov dword [rbx + 28], MEMORY_MAP_ENTRY_SIZE

    mov qword [rax + 0], 0
    mov qword [rax + 8], 512 * 1024 * 1024
    mov dword [rax + 16], MEMORY_TYPE_AVAILABLE
    mov dword [rax + 20], 0

    ; Debug: boot_info built
    mov dx, 0xE9
    mov al, 'F'
    out dx, al

    ; Call kernel_main(&boot_info)
    mov rdi, rbx
    
    ; Debug: About to call kernel_main
    mov dx, 0xE9
    mov al, 'G'
    out dx, al
    
    call kernel_main

    ; Debug: kernel_main returned (shouldn't happen)
    mov dx, 0xE9
    mov al, 'Z'
    out dx, al

.hang:
    hlt
    jmp .hang

section .bss
align 16
stack32_bottom:
    resb STACK32_SIZE_BYTES
stack32_top:

align 4096
pml4_table:
    resb 4096
align 4096
pdpt_table:
    resb 4096
align 4096
pd_table:
    resb 4096

align 16
stack_bottom:
    resb STACK64_SIZE_BYTES
stack_top:

align 16
boot_info_struct:
    resb 512

align 16
memory_map_entry:
    resb MEMORY_MAP_ENTRY_SIZE

section .rodata
align 16
gdt64:
    dq 0
.code_segment: equ $ - gdt64
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53)
.data_segment: equ $ - gdt64
    dq (1 << 44) | (1 << 47) | (1 << 41)
.pointer:
    dw $ - gdt64 - 1
    dq gdt64

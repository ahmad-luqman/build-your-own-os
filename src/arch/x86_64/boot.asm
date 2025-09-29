; MiniOS x86-64 Bootloader
; Multiboot2 compliant bootloader for x86-64

section .multiboot
align 8
multiboot_header:
    dd 0xe85250d6                ; Multiboot2 magic number
    dd 0                         ; Architecture (0 = i386)
    dd multiboot_header_end - multiboot_header  ; Header length
    dd -(0xe85250d6 + 0 + (multiboot_header_end - multiboot_header)) ; Checksum

    ; End tag
    dw 0    ; Type
    dw 0    ; Flags  
    dd 8    ; Size
multiboot_header_end:

section .text
bits 32
global _start

_start:
    ; We're loaded by bootloader (GRUB) in 32-bit protected mode
    ; Need to set up 64-bit long mode
    
    ; Disable interrupts
    cli
    
    ; Set up initial stack
    mov esp, stack_top
    
    ; Save multiboot info
    mov [multiboot_magic], eax
    mov [multiboot_info], ebx
    
    ; Check for required CPU features
    call check_multiboot
    call check_cpuid
    call check_long_mode
    
    ; Set up paging for long mode
    call setup_page_tables
    call enable_paging
    
    ; Load 64-bit GDT
    lgdt [gdt64.pointer]
    
    ; Jump to 64-bit code
    jmp gdt64.code_segment:long_mode_start

check_multiboot:
    cmp eax, 0x36d76289
    jne .no_multiboot
    ret
.no_multiboot:
    mov al, "M"
    jmp error

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
    mov al, "C"
    jmp error

check_long_mode:
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .no_long_mode
    ret
.no_long_mode:
    mov al, "L"
    jmp error

setup_page_tables:
    ; Clear page tables
    mov edi, page_table_l4
    mov cr3, edi
    xor eax, eax
    mov ecx, 4096
    rep stosd
    mov edi, cr3
    
    ; Set up page tables (identity map first 2MB)
    mov DWORD [page_table_l4], page_table_l3 + 0x003
    mov DWORD [page_table_l3], page_table_l2 + 0x003  
    mov DWORD [page_table_l2], 0x000083  ; 2MB page, present, writable
    
    ret

enable_paging:
    ; Enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax
    
    ; Enable long mode
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr
    
    ; Enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax
    
    ret

error:
    ; Print error character and halt
    mov dword [0xb8000], 0x4f524f45  ; "ER" in red
    mov dword [0xb8004], eax          ; Error code
    hlt

bits 64
long_mode_start:
    ; Clear segment registers
    mov ax, gdt64.data_segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Clear BSS
    mov rdi, __bss_start
    mov rcx, __bss_end
    sub rcx, rdi
    mov rax, 0
    rep stosb
    
    ; Call kernel main
    extern kernel_main
    call kernel_main
    
    ; Halt if kernel returns
.halt:
    hlt
    jmp .halt

section .bss
align 16
stack_bottom:
    resb 4096
stack_top:

align 4096
page_table_l4:
    resb 4096
page_table_l3:
    resb 4096
page_table_l2:
    resb 4096

multiboot_magic:
    resd 1
multiboot_info:
    resd 1

section .rodata
gdt64:
    dq 0
.code_segment: equ $ - gdt64
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53)
.data_segment: equ $ - gdt64
    dq (1 << 44) | (1 << 47) | (1 << 41)
.pointer:
    dw $ - gdt64 - 1
    dq gdt64
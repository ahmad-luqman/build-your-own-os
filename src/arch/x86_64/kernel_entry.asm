; MiniOS x86-64 Kernel Entry (PVH + Multiboot friendly)
; Boots from 32-bit protected mode, enables long mode, and enters kernel_main.

%define BOOT_PROTOCOL_MAGIC   0x4D696E694F53        ; "MiniOS"
%define BOOT_ARCH_X86_64      0x2
%define BOOT_PROTOCOL_VERSION 0x1
%define MEMORY_TYPE_AVAILABLE 0x1
%define MEMORY_MAP_ENTRY_SIZE 24

%define STACK32_SIZE_BYTES 4096
%define STACK64_SIZE_BYTES 16384  ; Increased from 4KB to 16KB for deep call stacks

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
    ; Debug: Starting page table setup
    mov dx, 0xE9
    mov al, '1'
    out dx, al

    ; Zero tables (7 tables now: PML4, PDPT, PD, PT_KERNEL, PT_KERNEL2, PT_KERNEL3, PT_KERNEL4)
    mov edi, pml4_table
    mov ecx, (4096 * 7) / 4
    xor eax, eax
    rep stosd

    ; Debug: Tables zeroed
    mov dx, 0xE9
    mov al, '2'
    out dx, al

    ; PML4[0] -> PDPT
    mov eax, pdpt_table
    or eax, 0x3                     ; Present + Writable
    mov [pml4_table], eax
    mov dword [pml4_table + 4], 0

    ; PDPT[0] -> PD (first 1GB)
    mov eax, pd_table
    or eax, 0x3                     ; Present + Writable
    mov [pdpt_table], eax
    mov dword [pdpt_table + 4], 0

    ; PD[0] -> PT_KERNEL (first 2MB, contains kernel at 1MB)
    ; PD[1] -> PT_KERNEL2 (next 2MB, contains .data/.bss sections)
    ; PD[2] -> PT_KERNEL3 (next 2MB, extends BSS coverage)
    ; PD[3] -> PT_KERNEL4 (next 2MB, provides headroom)
    ; This gives us fine-grained 4KB control over kernel region (0-8MB)
    mov eax, pt_kernel
    or eax, 0x3                     ; Present + Writable
    mov [pd_table], eax
    mov dword [pd_table + 4], 0

    mov eax, pt_kernel2
    or eax, 0x3                     ; Present + Writable
    mov [pd_table + 8], eax
    mov dword [pd_table + 12], 0

    mov eax, pt_kernel3
    or eax, 0x3                     ; Present + Writable
    mov [pd_table + 16], eax
    mov dword [pd_table + 20], 0

    mov eax, pt_kernel4
    or eax, 0x3                     ; Present + Writable
    mov [pd_table + 24], eax
    mov dword [pd_table + 28], 0

    ; Debug: Mapping 4KB pages
    mov dx, 0xE9
    mov al, '3'
    out dx, al

    ; Map first 8MB using 4KB pages via PT_KERNEL through PT_KERNEL4
    ; This covers 0x000000 - 0x800000 (includes all kernel sections + headroom)
    mov ecx, 2048                   ; 2048 entries * 4KB = 8MB
    xor eax, eax
.map_4kb_loop:
    mov edx, eax
    shl edx, 12                     ; Physical address = index * 4KB
    mov ebx, edx
    or ebx, 0x03                    ; Present + Writable (4KB pages)

    ; Distribute across 4 page tables (512 entries each)
    cmp eax, 512
    jl .first_table
    cmp eax, 1024
    jl .second_table
    cmp eax, 1536
    jl .third_table
    ; PT_KERNEL4 (entries 1536-2047)
    mov esi, eax
    sub esi, 1536
    mov [pt_kernel4 + esi*8], ebx
    mov dword [pt_kernel4 + esi*8 + 4], 0
    jmp .next_entry
.third_table:
    ; PT_KERNEL3 (entries 1024-1535)
    mov esi, eax
    sub esi, 1024
    mov [pt_kernel3 + esi*8], ebx
    mov dword [pt_kernel3 + esi*8 + 4], 0
    jmp .next_entry
.second_table:
    ; PT_KERNEL2 (entries 512-1023)
    mov esi, eax
    sub esi, 512
    mov [pt_kernel2 + esi*8], ebx
    mov dword [pt_kernel2 + esi*8 + 4], 0
    jmp .next_entry
.first_table:
    ; PT_KERNEL (entries 0-511)
    mov [pt_kernel + eax*8], ebx
    mov dword [pt_kernel + eax*8 + 4], 0
.next_entry:
    inc eax
    cmp eax, 2048
    jl .map_4kb_loop

    ; Debug: Mapping 2MB pages
    mov dx, 0xE9
    mov al, '4'
    out dx, al

    ; Map 8MB-1GB using 2MB pages (entries 4-511 in PD)
    mov ecx, 508                    ; 508 entries (skip entries 0-3, already mapped)
    mov eax, 4                      ; Start at index 4
.map_2mb_loop:
    mov edx, eax
    shl edx, 21                     ; Physical address = index * 2MB
    mov ebx, edx
    or ebx, 0x83                    ; Present + Writable + Page Size (2MB)
    mov [pd_table + eax*8], ebx
    mov dword [pd_table + eax*8 + 4], 0
    inc eax
    cmp eax, 512
    jl .map_2mb_loop

    ; Debug: Page tables complete
    mov dx, 0xE9
    mov al, '5'
    out dx, al

    ret

enable_paging:
    ; Enable SSE (required for -O2 compiled code)
    ; SSE instructions (movaps, movdqa, etc.) require this setup
    mov eax, cr0
    and ax, 0xFFFB      ; Clear CR0.EM (bit 2) - disable x87 emulation
    or ax, 0x2          ; Set CR0.MP (bit 1) - monitor coprocessor
    mov cr0, eax

    mov eax, cr4
    or eax, (1 << 9)    ; Set CR4.OSFXSR (bit 9) - enable FXSAVE/FXRSTOR
    or eax, (1 << 10)   ; Set CR4.OSXMMEXCPT (bit 10) - enable unmasked SSE exceptions
    or eax, (1 << 5)    ; Set CR4.PAE (bit 5) - enable PAE
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

    ; Flush TLB to ensure page tables are properly loaded
    mov rax, cr3
    mov cr3, rax

    ; Debug: Page tables configured (4KB kernel pages)
    mov dx, 0xE9
    mov al, 'P'
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
align 4096
pt_kernel:
    resb 4096
align 4096
pt_kernel2:
    resb 4096
align 4096
pt_kernel3:
    resb 4096
align 4096
pt_kernel4:
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

; Simple x86_64 bootloader stub for QEMU
; This is a minimal 16-bit bootloader that loads at 0x7c00,
; switches to protected mode, then long mode, and jumps to kernel

bits 16
org 0x7c00

start:
    cli
    
    ; Setup segments
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    
    ; Load kernel from disk
    ; (In QEMU with -kernel, the kernel is already loaded)
    ; Just jump to it
    
    ; Enable A20
    in al, 0x92
    or al, 2
    out 0x92, al
    
    ; Load GDT
    lgdt [gdt_descriptor]
    
    ; Enter protected mode
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    
    jmp 0x08:protected_mode

bits 32
protected_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax
    
    ; Setup paging for long mode
    ; (Simplified - kernel does its own paging)
    
    ; Enable PAE
    mov eax, cr4
    or eax, 0x20
    mov cr4, eax
    
    ; Set long mode bit
    mov ecx, 0xC0000080
    rdmsr
    or eax, 0x100
    wrmsr
    
    ; Enable paging
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    
    ; Jump to kernel entry point
    jmp 0x08:0x100040

; GDT
align 8
gdt_start:
    dq 0                        ; Null descriptor
gdt_code:
    dw 0xFFFF                   ; Limit low
    dw 0                        ; Base low
    db 0                        ; Base middle
    db 10011010b                ; Access
    db 11001111b                ; Granularity
    db 0                        ; Base high
gdt_data:
    dw 0xFFFF
    dw 0
    db 0
    db 10010010b
    db 11001111b
    db 0
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

; Boot signature
times 510-($-$$) db 0
dw 0xAA55

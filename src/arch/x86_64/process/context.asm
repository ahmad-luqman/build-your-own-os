; x86-64 Context Switching Implementation
; 
; Low-level context switching for x86-64 architecture

section .text

; External functions
extern syscall_dispatch

; Context switch between two tasks
; void context_switch(struct cpu_context *old_ctx, struct cpu_context *new_ctx)
global context_switch
context_switch:
    ; RDI = old context pointer
    ; RSI = new context pointer
    
    ; If old_ctx is NULL, just restore new context
    test rdi, rdi
    jz restore_context
    
    ; Save current context to old_ctx
    mov [rdi + 0], rax       ; rax
    mov [rdi + 8], rbx       ; rbx
    mov [rdi + 16], rcx      ; rcx
    mov [rdi + 24], rdx      ; rdx
    mov [rdi + 32], rsi      ; rsi
    mov [rdi + 40], rdi      ; rdi
    mov [rdi + 48], rbp      ; rbp
    mov [rdi + 56], rsp      ; rsp
    mov [rdi + 64], r8       ; r8
    mov [rdi + 72], r9       ; r9
    mov [rdi + 80], r10      ; r10
    mov [rdi + 88], r11      ; r11
    mov [rdi + 96], r12      ; r12
    mov [rdi + 104], r13     ; r13
    mov [rdi + 112], r14     ; r14
    mov [rdi + 120], r15     ; r15
    
    ; Save RIP (return address)
    mov rax, [rsp]           ; Get return address from stack
    mov [rdi + 128], rax     ; rip
    
    ; Save RFLAGS
    pushfq
    pop rax
    mov [rdi + 136], rax     ; rflags
    
    ; Save segment registers
    mov ax, cs
    mov [rdi + 144], rax     ; cs
    mov ax, ss
    mov [rdi + 152], rax     ; ss
    mov ax, ds
    mov [rdi + 160], rax     ; ds
    mov ax, es
    mov [rdi + 168], rax     ; es
    mov ax, fs
    mov [rdi + 176], rax     ; fs
    mov ax, gs
    mov [rdi + 184], rax     ; gs
    
    ; Save CR3 (page directory)
    mov rax, cr3
    mov [rdi + 192], rax     ; cr3

restore_context:
    ; Restore new context from new_ctx (RSI)
    ; Restore CR3 first
    mov rax, [rsi + 192]     ; cr3
    mov cr3, rax
    
    ; Restore segment registers
    mov rax, [rsi + 160]     ; ds
    mov ds, ax
    mov rax, [rsi + 168]     ; es
    mov es, ax
    mov rax, [rsi + 176]     ; fs
    mov fs, ax
    mov rax, [rsi + 184]     ; gs
    mov gs, ax
    
    ; Restore RFLAGS
    mov rax, [rsi + 136]     ; rflags
    push rax
    popfq
    
    ; Restore general purpose registers (except RSP and RIP)
    mov rax, [rsi + 0]       ; rax
    mov rbx, [rsi + 8]       ; rbx
    mov rcx, [rsi + 16]      ; rcx
    mov rdx, [rsi + 24]      ; rdx
    ; Skip RSI for now
    mov rdi, [rsi + 40]      ; rdi
    mov rbp, [rsi + 48]      ; rbp
    ; Skip RSP for now
    mov r8, [rsi + 64]       ; r8
    mov r9, [rsi + 72]       ; r9
    mov r10, [rsi + 80]      ; r10
    mov r11, [rsi + 88]      ; r11
    mov r12, [rsi + 96]      ; r12
    mov r13, [rsi + 104]     ; r13
    mov r14, [rsi + 112]     ; r14
    mov r15, [rsi + 120]     ; r15
    
    ; Restore RSP and jump to new RIP
    mov rsp, [rsi + 56]      ; rsp
    push qword [rsi + 128]   ; Push new RIP onto stack
    mov rsi, [rsi + 32]      ; Restore RSI last
    ret                      ; Jump to new RIP

; Setup initial context for a new task
; void arch_setup_task_context(struct cpu_context *ctx, task_entry_t entry, void *arg, void *stack_top)
global arch_setup_task_context
arch_setup_task_context:
    ; RDI = context pointer
    ; RSI = task entry point
    ; RDX = task argument
    ; RCX = stack top
    
    ; Clear the context (simple approach - just zero key fields)
    mov qword [rdi + 0], 0    ; rax
    mov qword [rdi + 8], 0    ; rbx
    mov qword [rdi + 16], 0   ; rcx
    mov qword [rdi + 24], 0   ; rdx
    
    ; Set up initial register state
    mov [rdi + 40], rdx      ; rdi = task argument (first parameter in x86-64 ABI)
    mov [rdi + 128], rsi     ; rip = task entry point
    
    ; Set up stack pointer (align to 16 bytes and leave space for return address)
    and rcx, ~15             ; Align stack to 16 bytes
    sub rcx, 8               ; Leave space for return address
    mov [rdi + 56], rcx      ; rsp = aligned stack_top
    
    ; Set up RFLAGS with interrupts enabled
    mov rax, 0x0202          ; IF=1, reserved bit=1
    mov [rdi + 136], rax     ; rflags
    
    ; Set up segment registers (kernel segments)
    mov rax, 0x08            ; Kernel code segment
    mov [rdi + 144], rax     ; cs
    mov rax, 0x10            ; Kernel data segment
    mov [rdi + 152], rax     ; ss
    mov [rdi + 160], rax     ; ds
    mov [rdi + 168], rax     ; es
    mov [rdi + 176], rax     ; fs
    mov [rdi + 184], rax     ; gs
    
    ; Set up MMU state (inherit current CR3)
    mov rax, cr3
    mov [rdi + 192], rax     ; cr3
    
    ret

; System call entry point (SYSCALL handler)
global syscall_entry_syscall
syscall_entry_syscall:
    ; Save caller's context
    ; SYSCALL instruction saves RIP to RCX and RFLAGS to R11
    ; We need to save other registers
    
    ; Save registers
    push rax                 ; System call number
    push rbx
    push rcx                 ; Caller's RIP
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11                 ; Caller's RFLAGS
    push r12
    push r13
    push r14
    push r15
    
    ; System call arguments in registers (x86-64 ABI):
    ; RAX = system call number
    ; RDI, RSI, RDX, R10, R8, R9 = arguments 0-5
    ; Note: R10 is used instead of RCX because SYSCALL uses RCX
    
    mov rdi, rax             ; System call number -> arg 0
    ; RSI already has arg 1
    ; RDX already has arg 2
    mov rcx, r10             ; arg 3 (R10 -> RCX for function call ABI)
    ; R8 already has arg 4
    ; R9 already has arg 5
    
    ; Call system call dispatcher
    call syscall_dispatch
    
    ; Result is in RAX, save it
    mov rbx, rax
    
    ; Restore caller's context
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11                  ; Caller's RFLAGS
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx                  ; Caller's RIP
    add rsp, 8               ; Skip saved RAX (system call number)
    
    ; Move result to RAX
    mov rax, rbx
    
    ; Return to caller using SYSRET
    ; RCX has caller's RIP, R11 has caller's RFLAGS
    o64 sysret
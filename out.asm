bits 64
global _start

section .text

; _print_int: prints integer in rdi followed by newline
_print_int:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    mov rax, rdi
    lea rsi, [rbp-20]
    mov byte [rsi], 0x0a
    dec rsi
    mov rcx, 10
    test rax, rax
    jns .pos
    neg rax
.pos:
    xor rdx, rdx
    div rcx
    add dl, '0'
    mov [rsi], dl
    dec rsi
    test rax, rax
    jnz .pos
    inc rsi
    lea rdx, [rbp-19]
    sub rdx, rsi
    inc rdx
    mov rax, 1
    mov rdi, 1
    syscall
    leave
    ret

_start:
    push rbp
    mov rbp, rsp
    sub rsp, 64

    mov qword [rbp-8], 0
    mov qword [rbp-16], 0
    mov qword [rbp-24], 0

    mov rax, 1234
    mov [rbp-8], rax
    mov rax, 5678
    mov [rbp-16], rax
    mov rax, [rbp-8]
    push rax
    mov rax, [rbp-16]
    mov rbx, rax
    pop rax
    add rax, rbx
    mov [rbp-24], rax
    mov rax, [rbp-24]
    ; --- DISPLAY (integer) ---
    mov rdi, rax
    call _print_int
    ; STOP RUN
    mov rax, 60
    xor rdi, rdi
    syscall
    mov rax, 60
    xor rdi, rdi
    syscall

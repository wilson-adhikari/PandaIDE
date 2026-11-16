section .text
global _start

_start:
    mov rax, 1
    mov rdi, 1
    lea rsi, [message]
    mov rdx, 13
    syscall

section .data
message db "Hello, World!", 0
[bits 64]
section .text
global _start
extern main

_start:
    mov  rdi, [rsp]       ; argc (pushed by kernel)
    lea  rsi, [rsp + 8]   ; argv
    call main
    mov  rax, 60          ; SYS_EXIT
    xor  rdi, rdi
    syscall
.halt:
    hlt
    jmp .halt

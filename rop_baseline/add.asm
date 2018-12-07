global add

section .data

section .text

add:
    push exit
    push gadget
    push gadget
    push gadget
    push gadget


    push gadget
    push gadget
    push gadget
    push gadget
    push gadget
    ret


gadget:
    nop
    nop
    nop
    ret
exit:
    mov   rax, 60               ; sys_exit
    mov   rdi, 0                ; exit(0)
    syscall                     ; transfer control to the kernel

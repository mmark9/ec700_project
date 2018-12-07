global add

section .data

section .text

add:
    ;mov   eax, [esp+4]   ; argument 1
    ;add   eax, [esp+8]   ; argument 2

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

    push gadget
    ret


gadget:
    nop
    nop
    nop
    ret
exit:
    add     esp,12                              ;clean stack (3 arguments * 4)
    push    0                                   ;exit code
    mov     eax,0x1                             ;system call number (sys_exit)                         
    int 0x80                                    ;call kernel

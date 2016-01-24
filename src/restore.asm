_TEXT	SEGMENT

PUBLIC	RestoreX64

RestoreX64 PROC
    mov	rsp, rcx
    pop	r15
    pop	r14
    pop	r13
    pop	r12
    pop	r11
    pop	r10
    pop	r9
    pop	r8
    pop	rdi
    pop	rsi
    pop	rbp
    pop	rdx
    pop	rcx
    pop	rbx
    pop	rax
    popfq
    ret
RestoreX64 ENDP

_TEXT ENDS

END

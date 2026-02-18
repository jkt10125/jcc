.text
    .globl lang_main
    .type lang_main, @function
lang_main:
    pushq %rbp
    movq %rsp, %rbp
    subq $24, %rsp
    movq $0, -8(%rbp)
    movq $0, -16(%rbp)
    movq $0, -24(%rbp)
    movq $0, %rax
    movq %rax, %rsi
    leaq add(%rip), %rax
    movq %rax, %rdx
    movq mem(%rip), %rbx
    leaq (%rbx,%rsi,8), %rcx
    movq %rdx, (%rcx)
    movq $5, %rax
    movq %rax, -8(%rbp)
    movq $10, %rax
    movq %rax, -16(%rbp)
    movq $1, %rax
    movq %rax, %rsi
    movq $20, %rax
    movq %rax, %rdx
    movq mem(%rip), %rbx
    leaq (%rbx,%rsi,8), %rcx
    movq %rdx, (%rcx)
    movq -8(%rbp), %rax
    movq %rax, %rdi
    movq -16(%rbp), %rax
    movq %rax, %rsi
    pushq %rsi
    pushq %rdi
    movq $0, %rax
    movq %rax, %rsi
    movq mem(%rip), %rbx
    leaq (%rbx,%rsi,8), %rcx
    movq (%rcx), %rax
    popq %rdi
    popq %rsi
    call *%rax
    pushq %rax
    movq $1, %rax
    movq %rax, %rsi
    movq mem(%rip), %rbx
    leaq (%rbx,%rsi,8), %rcx
    movq (%rcx), %rax
    popq %rbx
    subq %rax, %rbx
    movq %rbx, %rax
    movq %rax, -24(%rbp)
    movq -24(%rbp), %rax
    movq %rax, %rdi
    call printInt
    movq -24(%rbp), %rax
    addq $24, %rsp
    popq %rbp
    ret
    movq $0, %rax
    addq $24, %rsp
    popq %rbp
    ret
    .globl add
    .type add, @function
add:
    pushq %rbp
    movq %rsp, %rbp
    subq $16, %rsp
    movq %rdi, -8(%rbp)
    movq %rsi, -16(%rbp)
    movq -8(%rbp), %rax
    pushq %rax
    movq -16(%rbp), %rax
    popq %rbx
    addq %rbx, %rax
    movq %rax, %rax
    addq $16, %rsp
    popq %rbp
    ret
    movq $0, %rax
    addq $16, %rsp
    popq %rbp
    ret

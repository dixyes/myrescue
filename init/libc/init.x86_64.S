
.text
#define syscall_decl(name, num, ...) \
.global name; \
name: \
    mov $ ##num , %rax; \
    syscall; \
    ret
#define syscall_declx(name, num, ...) \
.global name; \
name: \
    mov %rcx, %r10; \
    mov $ ##num , %rax; \
    syscall; \
    ret

#include "syscalldef.h"

.global breakpoint

.global myclone
myclone:
    mov 8(%rsp), %r12 // save arg
    mov %rcx, %r10 // calling convention convert
    mov $56, %rax // SYS_CLONE = 56 for x86_64
    syscall // call clone(2) with flags, stack, parent_tidptr, child_tidptr, tls
    test %rax, %rax // check if we are child or parent
    jz 1f
    ret
    1: xor %rbp, %rbp // clean rbp according to calling convention
    mov %r12, %rdi
    call *%r9 // call fn(arg)
    mov %rax, %rdi
    call exit // exit with fn result
    jmp breakpoint

// the binary entrypoint
.global _start
_start:
    // parepare argc argv envp
    mov %rsp, %rdi // argc
    lea 8(%rsp), %rsi // argv
    lea 16(%rsp, %rdi, 8), %rdx // envp
    // call main
    call main
    // exit
    mov $60, %rax
    syscall
    jmp breakpoint

breakpoint:
    ud2

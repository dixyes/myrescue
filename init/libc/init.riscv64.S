
.text
#define hash_symbol #
#define syscall_decl(name, num, ...) \
.global name; \
name: \
    li a7, num; \
    scall; \
    ret

#include "syscalldef.h"

.global breakpoint

.global myclone
myclone:
    mv s2, a5 // save arg
    li a7, 220 // SYS_clone = 220 for riscv64
    scall // call clone(2) with flags, stack, parent_tidptr, child_tidptr, tls
    beqz a0, .L1 // check if we are child or parent
    ret // if parent, return
    .L1:
    mv a5, s2
    jalr ra, a5, 0 // call fn(arg)
    li a7, 93
    scall // exit with fn result
    j breakpoint

// the binary entrypoint
.global _start
_start:
    // parepare argc argv envp
    ld a0, 0(sp) // argc
    add a1, sp, 8 // argv
    sll a2, a0, 3 // envp
    add a2, a2, a1
    add a2, a2, 8
    // call reloc
    call reloc
    // call main
    call main
    // exit
    li a7, 93
    scall
    j breakpoint

breakpoint:
    .4byte 0x00100073 

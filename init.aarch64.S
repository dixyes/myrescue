
.text
#define hash_symbol #
#define syscall_decl(name, num, ...) \
.global name; \
name: \
    mov x8, hash_symbol num; \
    svc 0; \
    ret

#include "syscalldef.h"

.global breakpoint

.global myclone
myclone:
    mov x19, x6 // save arg
    mov x8, #220 // SYS_clone = 220 for aarch64
    svc #0 // call clone(2) with flags, stack, parent_tidptr, child_tidptr, tls
    cbz x0, .L1 // check if we are child or parent
    ret // if parent, return
    .L1:
    mov x0, x19
    blr x5 // call fn(arg)
    mov x8, #93
    svc #0 // exit with fn result
    b breakpoint

// the binary entrypoint
.global _start
_start:
    // parepare argc argv envp
    ldr x0, [sp] // argc
    add x1, sp, #8 // argv
    add x2, x1, x0, lsl #3 // envp
    add x2, x2, #8
    // call main
    bl main
    // exit
    mov x8, #93
    svc #0
    b breakpoint

breakpoint:
    .inst   0xe7f000f0

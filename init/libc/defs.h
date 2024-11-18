
#ifndef __DEFS_H
# define __DEFS_H

// types
#define NULL ((void*)0)

typedef long long int64_t;
typedef unsigned long long uint64_t;
typedef int int32_t;
typedef unsigned uint32_t;
typedef unsigned short uint16_t;
typedef unsigned long long size_t;
typedef unsigned long long off_t;
typedef unsigned long long time_t;
typedef uint64_t pid_t;
typedef uint64_t intptr_t;
typedef unsigned uid_t;
typedef unsigned long long id_t;
typedef unsigned long long idtype_t;

struct timespec {
    time_t  tv_sec;  /* Seconds */
    long    tv_nsec; /* Nanoseconds */
};

union sigval {
    int     sigval_int;
    void   *sigval_ptr;
};

typedef struct {
    int      si_signo;
    int      si_errno;
    int      si_code;
    pid_t    si_pid;
    uint32_t si_uid;
    int      si_status;
} __attribute__((packed)) siginfo_t;

typedef struct {
    void  *ss_sp;     /* Base address of stack */
    int    ss_flags;  /* Flags */
    size_t ss_size;   /* Number of bytes in stack */
} stack_t;

typedef unsigned long sigset_t;

struct sigaction {
    union {
        void (*sa_handler)(int);
        void (*sa_sigaction)(int, void *, void *);
    };
    uint64_t   sa_flags;
    void     (*sa_restorer)(void);
    sigset_t   sa_mask;
} __attribute__((packed));

#if defined(__aarch64__) || (defined(__riscv_xlen) && __riscv_xlen == 64) || defined(__x86_64__)
typedef struct {
    uint64_t d_tag;
    union {
        uint64_t d_val;
        void *d_ptr;
    } d_un;
} Elf_Dyn;

typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    off_t p_offset;
    void *p_vaddr;
    void *p_paddr;
    size_t p_filesz;
    size_t p_memsz;
    size_t p_align;
} Elf_Phdr;

typedef struct {
    void *r_offset;
    uint64_t r_info;
    int64_t r_addend;
} Elf_Rela;

typedef struct {
    uint64_t type;
    union {
        uint64_t a_val;
        void *a_ptr;
        void (*a_fcn)();
    } a_un;
} auxv_t;
#else
# error not supported
#endif

#define ELF64_R_TYPE(info) ((info) & 0xffffffff)

#if defined(__aarch64__)
# define R_AARCH64_RELATIVE 1027
#elif defined(__x86_64__)
# define R_X86_64_RELATIVE 8
#elif defined(__riscv)
#else
#  error not supported
#endif


// syscalls
#if defined(__aarch64__)
# define syscall_decl(name, num, ...) \
int64_t name(__VA_ARGS__);
#elif defined(__x86_64__)
# define syscall_decl(name, num, ...) \
int64_t __attribute__((sysv_abi)) name(__VA_ARGS__);
# define syscall_declx syscall_decl
#elif defined(__riscv)
# define syscall_decl(name, num, ...) \
int64_t name(__VA_ARGS__);
#else
# error not supported
#endif

#include "syscalldef.h"

// macros

#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define PROT_EXEC 0x4

#define MAP_PRIVATE 0x02
#define MAP_ANONYMOUS 0x20
#define MAP_STACK 0x20000
#define MAP_UNINITIALIZED 0x4000000
#define MAP_FAILED ((void *)-1)

#define CLONE_FS 0x00000200

#define P_PID 1
#define SIGINT 2
#define SIGKILL 9
#define SIGUSR2 12
#define SIGTERM 15
#define SIGCHLD 17
#define SIGSTKSZ 8192
#define SA_ONSTACK 0x08000000
#define SA_RESTART 0x10000000
#define SA_RESTORER 0x04000000
#define CLD_EXITED 1
#define WNOHANG 1
#define WEXITED 4

#define RB_POWER_OFF 0x4321fedc
#define RB_HALT_SYSTEM 0xcdef0123
#define RB_AUTOBOOT 0x1234567

#define AT_FDCWD -100
#define AT_STATX_FORCE_SYNC 0x2000
#define STATX_TYPE 0x0001U
#define STATX_MODE 0x0002U
#define X_OK 1
#define O_RDONLY 0
#define O_WRONLY 1

#define AT_NULL 0
#define AT_PHDR 3
#define AT_PHENT 4
#define AT_PHNUM 5
#define AT_PAGESZ 6
#define AT_BASE 7
#define AT_SYSINFO_EHDR	33
#define AT_MINSIGSTKSZ 51

#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2

#define DT_RELA 7
#define DT_RELASZ 8
#define DT_RELAENT 9

// utilities

static inline size_t strlen(const char* buf) {
    // dummy implementation
    size_t ret = 0;
    while (buf[ret++] != '\0');
    return ret-1;
}

static inline void writehex(int fd, uint64_t val) {
    char buf[17];
    buf[16] = '\0';
    for (int i = 0; i < 16; i++) {
        buf[15-i] = "0123456789abcdef"[val & 0xf];
        val >>= 4;
    }
    write(fd, buf, 16);
}

int64_t myclone(uint64_t flags, void* stack, void* parent_tidptr,
    void* child_tidptr, void* tls, void* fn, void* arg);

__attribute__((noreturn)) void breakpoint();
__attribute__((noreturn)) static inline void abort(const char* msg) {
    write(2, msg, strlen(msg));
    breakpoint();
}

#ifdef __IN_LIBC
void *__auxv;
size_t __pagesize;
size_t __minsigstksize;
#else
extern void *__auxv;
extern size_t __pagesize;
extern size_t __minsigstksize;
#endif

#endif // __DEFS_H

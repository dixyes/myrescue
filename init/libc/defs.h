
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

// syscalls
# if defined(__aarch64__)
#  define syscall_decl(name, num, ...) \
int64_t name(__VA_ARGS__);
# elif defined(__x86_64__)
#  define syscall_decl(name, num, ...) \
int64_t __attribute__((sysv_abi)) name(__VA_ARGS__);
#define syscall_declx syscall_decl
# elif defined(__riscv)
#  define syscall_decl(name, num, ...) \
int64_t name(__VA_ARGS__);
# else
#  error not supported
# endif

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

// utilities

static inline size_t strlen(const char* buf) {
    // dummy implementation
    size_t ret = 0;
    while (buf[ret++] != '\0');
    return ret-1;
}

int64_t myclone(uint64_t flags, void* stack, void* parent_tidptr,
    void* child_tidptr, void* tls, void* fn, void* arg);

__attribute__((noreturn)) void breakpoint();
__attribute__((noreturn)) static inline void abort(const char* msg) {
    write(2, msg, strlen(msg));
    breakpoint();
}

#endif // __DEFS_H

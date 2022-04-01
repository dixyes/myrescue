
// -static-pie -nodefaultlibs -nostartfiles -nostdlib -e _start -ffreestanding -fno-stack-protector

#define NULL ((void*)0)

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

struct statx_timestamp {
    int64_t tv_sec;    /* Seconds since the Epoch (UNIX time) */
    uint32_t tv_nsec;   /* Nanoseconds since tv_sec */
} __attribute__((packed));

struct statx {
    uint32_t stx_mask;        /* Mask of bits indicating
                                filled fields */
    uint32_t stx_blksize;     /* Block size for filesystem I/O */
    uint64_t stx_attributes;  /* Extra file attribute indicators */
    uint32_t stx_nlink;       /* Number of hard links */
    uint32_t stx_uid;         /* User ID of owner */
    uint32_t stx_gid;         /* Group ID of owner */
    uint16_t stx_mode;        /* File type and mode */
    uint64_t stx_ino;         /* Inode number */
    uint64_t stx_size;        /* Total size in bytes */
    uint64_t stx_blocks;      /* Number of 512B blocks allocated */
    uint64_t stx_attributes_mask;
                            /* Mask to show what's supported
                                in stx_attributes */

    /* The following fields are file timestamps */
    struct statx_timestamp stx_atime;  /* Last access */
    struct statx_timestamp stx_btime;  /* Creation */
    struct statx_timestamp stx_ctime;  /* Last status change */
    struct statx_timestamp stx_mtime;  /* Last modification */

    /* If this file represents a device, then the next two
        fields contain the ID of the device */
    uint32_t stx_rdev_major;  /* Major ID */
    uint32_t stx_rdev_minor;  /* Minor ID */

    /* The next two fields contain the ID of the device
        containing the filesystem where the file resides */
    uint32_t stx_dev_major;   /* Major ID */
    uint32_t stx_dev_minor;   /* Minor ID */
    uint64_t stx_mnt_id;      /* Mount ID */
} __attribute__((packed));

#define syscall_decl(name, num, ...) \
int64_t __attribute__((sysv_abi, naked)) name(__VA_ARGS__) { \
    asm volatile( \
    /* #name ":\n\t" */ \
        "mov $" #num ", %rax\n\t" \
        "syscall\n\t" \
        "ret\n\t" \
    ); \
}
#define syscall_declx(name, num, ...) \
int64_t __attribute__((sysv_abi, naked)) name(__VA_ARGS__) { \
    asm volatile( \
    /* #name ":\n\t" */ \
        "mov %rcx, %r10\n\t" \
        "mov $" #num ", %rax\n\t" \
        "syscall\n\t" \
        "ret\n\t" \
    ); \
}

syscall_decl(write, 1, uint64_t fd, const char *buf, size_t len)
//syscall_decl(stat, 4, const char *restrict pathname, struct stat *restrict statbuf)
syscall_declx(mmap, 9, void *addr, size_t length, int prot, int flags, int fd, off_t offset)
syscall_declx(rt_sigaction, 13, int signum, const struct sigaction *restrict act, struct sigaction *restrict oldact, size_t sigsetsize)
syscall_decl(rt_sigreturn, 15)
syscall_decl(access, 21, const char *pathname, int mode)
syscall_decl(nanosleep, 35, const struct timespec *req, struct timespec *rem)
syscall_decl(execve, 59, const char *pathname, char *const argv[], char *const envp[])
syscall_decl(exit, 60, uint64_t ret)
//syscall_declx(wait4, 61, pid_t pid, int *wstatus, int options, struct rusage *rusage)
syscall_decl(kill, 62, uint64_t pid, uint64_t signal)
syscall_decl(sigaltstack, 131, const stack_t *restrict ss, stack_t *restrict old_ss)
syscall_decl(sync, 162)
syscall_declx(reboot, 169, uint64_t m1, uint64_t m2, uint64_t cmd, void* arg)
syscall_decl(restart_syscall, 219)
syscall_declx(waitid, 247, idtype_t idtype, id_t id, siginfo_t *infop, int options)
syscall_declx(statx, 332, int dirfd, const char *restrict pathname, int flags, unsigned int mask, struct statx *restrict statxbuf)

#define syscall_failed(x) (ret < 0 && ret > -4096)

int64_t __attribute__((sysv_abi, naked)) myclone(uint64_t flags, void* stack, void* parent_tidptr, void* child_tidptr, void* tls, void* fn, void* arg) {
    asm volatile(
        "mov 8(%rsp), %r12\n\t" // save arg
        "mov %rcx, %r10\n\t" // calling convention convert
        "mov $56, %rax\n\t" // SYS_CLONE = 56 for x86_64
        "syscall\n\t" // call clone(2) with flags, stack, parent_tidptr, child_tidptr, tls
        "test %rax, %rax\n\t" // check if we are child or parent
        "jz 1f\n\t"
        "ret\n\t"
        "1: xor %rbp, %rbp\n\t" // clean rbp according to calling convention
        "mov %r12, %rdi\n\t"
        "call *%r9\n\t" // call fn(arg)
        "mov %rax, %rdi\n\t"
        "call exit\n\t" // exit with fn result
    );
}

// shabby strlen
size_t strlen(const char* buf) {
    size_t ret = 0;
    while (buf[ret++] != '\0');
    return ret-1;
}

int __attribute__((sysv_abi, naked, noreturn)) _start(void* this, int argc, char** argv, void* init, void* fini, void* reg) {
    asm volatile (
        "xor %rbp,%rbp\n\t"
        "pop %rdi\n\t" // argc
        "mov %rsp,%rsi\n\t" // argv
        "call main\n\t"
        "mov %rax, %rdi\n\t"
        "mov $60, %rax\n\t" // exit
        "syscall\n\t"
    );
}

#define write_stdout(msg) write(1, msg, sizeof(msg))
#define write_stderr(msg) write(2, msg, sizeof(msg))

typedef struct _fork_cmd_t {
    const char *cmd;
    char** argv;
    char** envp;
} fork_cmd_t;

int invoke_cmd(fork_cmd_t* cmd) {
    write_stdout("[myinit] calling ");
    write(1, cmd->cmd, strlen(cmd->cmd));
    write_stdout("\n");
    execve(cmd->cmd, cmd->argv, cmd->envp);
    return 0;
}

#define CLONE_STACK_SIZE 4096

#define check_syscall(msg) \
    if (syscall_failed(ret)) { \
        write_stderr("[myinit] failed " msg "\n"); \
        return -ret; \
    }
#define check_and_assign(type, var, msg) \
    check_syscall(msg); \
    type var = (type) ret;

void signal_restorer(void) {
    rt_sigreturn();
}

void signal_handler(int signal) {
    uint64_t cmd;
    switch (signal) {
        case SIGINT:
            // graceful exit
            exit(0);
        case SIGTERM:
            // reboot routine
            cmd = RB_AUTOBOOT;
            write_stderr("[myinit] received sigterm, rebooting\n");
            break;
        case SIGUSR2:
            // poweroff routine
            cmd = RB_POWER_OFF;
            write_stderr("[myinit] received sigusr2, shutting down\n");
            break;
        default:
            asm volatile ("ud2\n\t");
            return;
    }
    int64_t ret;

    // start poweroff/reboot routine
    sync();

    ret = kill(-1, SIGTERM);
    if (syscall_failed(ret)) {
        write_stderr("[myinit] kill(-1, SIGTERM) failed\n");
    }

    struct timespec ts = {
        .tv_sec = 2,
        .tv_nsec = 0,
    };
    nanosleep(&ts, &ts);

    sync();

    ret = kill(-1, SIGKILL);
    if (syscall_failed(ret)) {
        write_stderr("[myinit] kill(-1, SIGKILL) failed\n");
    }

    ret = reboot(0xfee1dead, 0x28121969/* torvalds' birth date */, cmd, NULL);
    if (syscall_failed(ret)) {
        write_stderr("[myinit] failed reboot syscall\n");
        asm volatile ("ud2\n\t");
    }
}

int main(int argc, char** argv) {
    int64_t ret;

    write_stdout("[myinit] dix's simple init daemon for x86_64 starting\n");

    ret = mmap(
        NULL,
        SIGSTKSZ,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK | MAP_UNINITIALIZED,
        -1,
        0
    );
    check_and_assign(void*, sigstack, "mmap for signal");

    stack_t _, ss={
        .ss_size = SIGSTKSZ,
        .ss_sp = sigstack,
    };

    ret = sigaltstack(&ss, &_);
    check_syscall("sigaltstack");

    struct sigaction __, sa = {
        .sa_handler = signal_handler,
        .sa_flags = SA_RESTART | SA_RESTORER | SA_ONSTACK,
        //.sa_mask = (1 << (SIGTERM - 1)) | (1 << (SIGUSR2 - 1)),
        .sa_restorer = signal_restorer,
    };
    // add signal handler
    ret = rt_sigaction(SIGTERM, &sa, &__, sizeof(sigset_t));
    check_syscall("sigaction SIGTERM (reboot)");
    ret = rt_sigaction(SIGUSR2, &sa, &__, sizeof(sigset_t));
    check_syscall("sigaction SIGUSR2 (poweroff)");
    ret = rt_sigaction(SIGINT, &sa, &__, sizeof(sigset_t));
    check_syscall("sigaction SIGINT (?)");

    ret = mmap(
        NULL,
        CLONE_STACK_SIZE,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK | MAP_UNINITIALIZED,
        -1,
        0
    );
    check_and_assign(void*, stack, "mmap for forking");

    siginfo_t siginfo;
    fork_cmd_t cmd = {
        .cmd = "/init.pre.sh",
        .argv = argv,
    };

    if (syscall_failed(access("/init.pre.sh", X_OK))) {
        write_stderr("[myinit] /init.pre.sh cannot be invoked, skipping\n");
    } else {
        ret = myclone(
            CLONE_FS | SIGCHLD, // allow child pivot root
            stack + CLONE_STACK_SIZE,
            NULL,
            NULL,
            NULL,
            invoke_cmd,
            &cmd
        );
        check_and_assign(pid_t, pid, "clone for init pre");

        ret = waitid(P_PID, pid, &siginfo, WEXITED);
        check_syscall("waitid for init pre");

        if (siginfo.si_code != CLD_EXITED || siginfo.si_status != 0) {
            write_stderr("[myinit] /init.pre.sh failed, this may cause strange problems\n");
        }
    }
    
    cmd.cmd = NULL;
    const char * const shell_guess[] = {
        "/init.shell.sh",
        "/bin/bash",
        "/usr/bin/bash",
        "/bin/ash",
        "/usr/bin/ash",
        "/bin/sh",
        "/usr/bin/sh",
    }; 
    for (int i = 0; i < sizeof(shell_guess) / sizeof(char*); i++) {
        if (!syscall_failed(access(shell_guess[i], X_OK))) {
            cmd.cmd = shell_guess[i];
            break;
        }
    }
    if (!cmd.cmd) {
        write_stderr("[myinit] cannot find any shell, exiting");
        return -2; // ENOENT
    }

    while (1) {
        ret = myclone(
            CLONE_FS | SIGCHLD, // allow child pivot root
            stack + CLONE_STACK_SIZE,
            NULL,
            NULL,
            NULL,
            invoke_cmd,
            &cmd
        );
        check_and_assign(pid_t, pid, "clone for init shell");

        ret = waitid(P_PID, pid, &siginfo, WEXITED);
        check_syscall("waitid for init shell");

        write_stderr("[myinit] shell exited, reopening\n");
    }
    // never here
    asm volatile ("ud2\n\t");
}

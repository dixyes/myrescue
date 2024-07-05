
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

#if defined(__aarch64__)
# define syscall_decl(name, num, ...) \
int64_t name(__VA_ARGS__);
#elif defined(__x86_64__)
# define syscall_decl(name, num, ...) \
int64_t __attribute__((sysv_abi)) name(__VA_ARGS__);
#define syscall_declx syscall_decl
#else
#error not supported
#endif

#include "syscalldef.h"

#define syscall_failed(ret) (ret < 0 && ret > -4096)

int64_t myclone(uint64_t flags, void* stack, void* parent_tidptr, void* child_tidptr, void* tls, void* fn, void* arg);

#define write_stdout(msg) write(1, msg, sizeof(msg))
#define write_stderr(msg) write(2, msg, sizeof(msg))

__attribute__((noreturn)) void breakpoint();
__attribute__((noreturn)) void abort(const char* msg) {
    write_stderr(msg);
    breakpoint();
}

// dummy strlen
size_t strlen(const char* buf) {
    size_t ret = 0;
    while (buf[ret++] != '\0');
    return ret-1;
}

typedef struct _fork_cmd_t {
    const char *cmd;
    const char * const* argv;
    const char * const* envp;
} fork_cmd_t;

int invoke_cmd(fork_cmd_t* cmd) {
    write_stdout("[myinit] calling ");
    write(1, cmd->cmd, strlen(cmd->cmd));
    write_stdout("\n");
    int64_t ret = execve(cmd->cmd, (char**)cmd->argv, (char**)cmd->envp);
    if (syscall_failed(ret)) {
        write_stderr("[myinit] failed to execve\n");
        return -1;
    }
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
            abort("unknown signal\n");
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
        abort("reboot failed\n");
    }
}

int main(int argc, char** argv, char** envp) {
    int64_t ret;

    write_stdout("[myinit] dix's simple init daemon starting\n");

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
        .argv = (void*)argv,
        .envp = (void*)envp,
    };

    ret = faccessat2(AT_FDCWD, "/init.pre.sh", X_OK, 0);
    if (syscall_failed(ret)) {
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
        write(1, (void*)&ret, 8);
        check_and_assign(pid_t, pid, "clone for init pre");

        ret = waitid(P_PID, pid, &siginfo, WEXITED);
        check_syscall("waitid for init pre");

        if (siginfo.si_code != CLD_EXITED || siginfo.si_status != 0) {
            write_stderr("[myinit] /init.pre.sh failed, this may cause strange problems\n");
        }
    }
    
    cmd.cmd = NULL;
    // strange arm64 gcc code generation
    // if let arr[] = {"a", "b"}, arr[1] will become 0x10a0 things
    // i guess there is magic in gcc init routines
    const char * shell_guess_str = "/init.shell.sh\0" "/usr/bin/bash\0" "/usr/bin/ash\0" "/usr/bin/sh";
    const char * const shell_guess[] = {
        &shell_guess_str[0],
        &shell_guess_str[19],
        &shell_guess_str[15],
        &shell_guess_str[33],
        &shell_guess_str[29],
        &shell_guess_str[46],
        &shell_guess_str[42],
    };
    for (int i = 0; i < sizeof(shell_guess) / sizeof(char*); i++) {
        const char *path = shell_guess[i];
        write_stderr("[myinit] trying shell ");
        write(2, path, strlen(path));
        write_stderr("\n");
        ret = faccessat2(AT_FDCWD, path, X_OK, 0);
        if (!syscall_failed(ret)) {
            cmd.cmd = path;
            break;
        }
    }
    if (!cmd.cmd) {
        write_stderr("[myinit] cannot find any shell, exiting\n");
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
    abort("myinit: unreachable code\n");
}


// -static-pie -nodefaultlibs -nostartfiles -nostdlib -e _start -ffreestanding -fno-stack-protector

#include "libc/defs.h"

#define syscall_failed(ret) (ret < 0 && ret > -4096)

#define write_stdout(msg) write(1, msg, sizeof(msg))
#define write_stderr(msg) write(2, msg, sizeof(msg))

#define CLONE_STACK_SIZE 4096

#define check_syscall(msg) \
    if (syscall_failed(ret)) { \
        write_stderr("[myinit] failed " msg "\n"); \
        write(1, (void*)&ret, sizeof(ret)); \
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

int main(int argc, char** argv, char** envp) {
    int64_t ret;
    write(1, "[myinit] starting\n", 18);

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

    struct sigaction dummysa, sa = {
        .sa_handler = signal_handler,
        .sa_flags = SA_RESTART | SA_RESTORER | SA_ONSTACK,
        //.sa_mask = (1 << (SIGTERM - 1)) | (1 << (SIGUSR2 - 1)),
        .sa_restorer = signal_restorer,
    };

    // add signal handler
    ret = rt_sigaction(SIGTERM, &sa, &dummysa, sizeof(sigset_t));
    check_syscall("sigaction SIGTERM (reboot)");
    ret = rt_sigaction(SIGUSR2, &sa, &dummysa, sizeof(sigset_t));
    check_syscall("sigaction SIGUSR2 (poweroff)");
    ret = rt_sigaction(SIGINT, &sa, &dummysa, sizeof(sigset_t));
    check_syscall("sigaction SIGINT (?)");

    siginfo_t siginfo;
    // fork_cmd_t cmd = {
    //     .cmd = "/init.sh",
    //     .argv = (const char * const []){"/init.sh", NULL},
    //     .envp = (void*)envp,
    // };
    ret = mmap(
        NULL,
        CLONE_STACK_SIZE,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK | MAP_UNINITIALIZED,
        -1,
        0
    );
    check_and_assign(void*, stack, "mmap for forking");

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
    const char * const shell_guess[] = {
        "/init.shell.sh",
        "/usr/bin/bash",
        "/usr/bin/ash",
        "/usr/bin/sh",
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


#include "defs.h"

#define syscall_failed(ret) (ret < 0 && ret > -4096)

#define write_stdout(msg) write(1, msg, sizeof(msg) - 1)
#define write_stderr(msg) write(2, msg, sizeof(msg) - 1)

#define check_syscall(msg) \
    if (syscall_failed(ret)) { \
        write_stderr("[myinit] failed " msg ", ret: "); \
        writehex(2, -ret); \
        write_stderr("\n"); \
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
            breakpoint();
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
        write_stderr("[myinit] kill(-1, SIGTERM) failed, ret: ");
        writehex(2, -ret);
        write_stderr("\n");
    }

    struct timespec ts = {
        .tv_sec = 2,
        .tv_nsec = 0,
    };
    nanosleep(&ts, &ts);

    sync();

    ret = kill(-1, SIGKILL);
    if (syscall_failed(ret)) {
        write_stderr("[myinit] kill(-1, SIGKILL) failed, ret: ");
        writehex(2, -ret);
        write_stderr("\n");
    }

    ret = reboot(0xfee1dead, 0x28121969/* torvalds' birth date */, cmd, NULL);
    if (syscall_failed(ret)) {
        write_stderr("[myinit] failed reboot syscall, ret: ");
        writehex(2, -ret);
        write_stderr("\n");
        abort("reboot failed\n");
    }
}

typedef struct _fork_cmd_t {
    const char *cmd;
    const char * const* argv;
    const char * const* envp;
    const char *tty_name;
} fork_cmd_t;

void invoke_cmd(fork_cmd_t* cmd) {
    if (cmd->tty_name) {
        int fd = openat(AT_FDCWD, cmd->tty_name, O_RDWR, 0666);
        if (syscall_failed(fd)) {
            write_stderr("[myinit] failed to open tty\n");
            goto skip;
        }
        setsid();
        ioctl(fd, TIOCSCTTY, (void*)1);
        // fd = openat(AT_FDCWD, "/dev/tty", O_RDWR, 0666);
        // if (syscall_failed(fd)) {
        //     write_stderr("[myinit] failed to open tty\n");
        //     goto skip;
        // }
        close(0);
        close(1);
        close(2);
        dup3(fd, 0, 0);
        dup3(fd, 1, 0);
        dup3(fd, 2, 0);
        skip:
        while (fd > 2) {
            close(fd--);
        }
    }
    write_stdout("[myinit] calling ");
    write(1, cmd->cmd, strlen(cmd->cmd));
    write_stdout("\n");
    int64_t ret = execve(cmd->cmd, (char**)cmd->argv, (char**)cmd->envp);
    write_stderr("[myinit] failed to execve, ret: ");
    writehex(2, -ret);
    write_stderr("\n");
    exit(1);
}

pid_t exec_cmd(fork_cmd_t* cmd, void *stack) {
    int64_t ret;

    ret = myclone(
        CLONE_FS | SIGCHLD, // allow child pivot root
        stack + __pagesize,
        NULL,
        NULL,
        NULL,
        invoke_cmd,
        cmd
    );
    if (syscall_failed(ret)) {
        write_stderr("[myinit] failed to clone, ret: ");
        writehex(2, -ret);
        write_stderr("\n");
        return (pid_t)ret;
    }

    return (pid_t)ret;
}

int setupSignal() {
    write_stdout("[myinit] setting up signal handler\n");
    int64_t ret;
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

    return 0;
}

const char * const shell_guess[] = {
    "/init.shell.sh",
    "/usr/bin/bash",
    "/usr/bin/ash",
    "/usr/bin/sh",
    "/bin/bash",
    "/bin/ash",
    "/bin/sh",
};

const char *guess_shell() {
    int64_t ret;
    for (int i = 0; i < (int)(sizeof(shell_guess) / sizeof(char*)); i++) {
        const char *path = shell_guess[i];
        write_stderr("[myinit] trying shell ");
        write(2, path, strlen(path));
        write_stderr("\n");
        ret = faccessat2(AT_FDCWD, path, X_OK, 0);
        if (!syscall_failed(ret)) {
            return path;
        }
    }
    return NULL;
}

int main(int argc, char** argv, char** envp) {
    (void) argc;
    (void) argv;
    int64_t ret;
    write_stdout("[myinit] starting\n");

    if (setupSignal()) {
        write_stderr("[myinit] failed to setup signal handler\n");
        return -1;
    }

    ret = mmap(
        NULL,
        __pagesize,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK | MAP_UNINITIALIZED,
        -1,
        0
    );
    check_and_assign(void*, stack, "mmap for clone");

    siginfo_t siginfo;
    // TODO: use c implementation ?
    fork_cmd_t cmd = {
        .cmd = "/init.pre.sh",
        .argv = (const char* const[]){"/init.pre.sh", NULL},
        .envp = (void*)envp,
    };
    pid_t pid = exec_cmd(&cmd, stack);
    if (pid < 0) {
        write_stderr("[myinit] failed to exec preinit\n");
        return -1;
    }
    ret = waitid(P_PID, pid, &siginfo, WEXITED, NULL);
    check_syscall("waitid preinit");
    if (siginfo.si_code != CLD_EXITED || siginfo.si_status != 0) {
        write_stderr("[myinit] /init.pre.sh failed, this may cause strange problems\n");
    }

    const char *shell = guess_shell();
    if (!shell) {
        write_stderr("[myinit] failed to guess shell\n");
        return -1;
    }
    cmd.cmd = shell;
    cmd.argv = (const char* const[]){shell, NULL};

    typedef struct {
        pid_t pid;
        int32_t _;
        char *tty_name;
    } __attribute__((packed)) subprocess_t;
    size_t mapbufsize = __pagesize;
    ret = mmap(
        NULL,
        mapbufsize,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );
    check_and_assign(subprocess_t *, pidttymap, "mmap for pidttymap");
    ret = mmap(
        NULL,
        __pagesize,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );
    check_and_assign(char *, tty_names, "mmap for tty_names");
    char tty_name[256] = "/dev/";

    ret = openat(AT_FDCWD, "/sys/class/tty/console/active", O_RDONLY|O_CLOEXEC, 0644);
    check_and_assign(int, consolesfd, "open consolesfd");

    uint64_t slot = 0;
    size_t tty_names_i = 0;
    for (; slot < mapbufsize / sizeof(*pidttymap);) {
        subprocess_t *pmap = &pidttymap[slot];
        int i = 5;
        for (
            ;
            (ret = read(consolesfd, &tty_name[i], 1)) == 1;
            i++
        ) {
            if (tty_name[i] == ' ' || tty_name[i] == '\n') {
                break;
            }
            if (i >= 255) {
                write_stderr("[myinit] tty name too long\n");
                return -1;
            }
        }
        if (ret == 0) {
            break;
        }
        tty_name[i] = '\0';
        write_stderr("[myinit] found tty ");
        write(2, tty_name, strlen(tty_name));
        write_stderr("\n");

        // try open tty
        ret = openat(AT_FDCWD, tty_name, O_RDWR, 0644);
        if (syscall_failed(ret)) {
            write_stderr("[myinit] failed to open tty\n");
            continue;
        }
        // give up this tty
        ioctl(ret, TIOCNOTTY, NULL);
        close(ret);


        size_t copied = strcpyn(&tty_names[tty_names_i], tty_name, __pagesize - tty_names_i);
        if (copied != strlen(tty_name) + 1) {
            write_stderr("[myinit] failed to copy tty name\n");
            break;
        }
        pmap->tty_name = &tty_names[tty_names_i];
        cmd.tty_name = &tty_names[tty_names_i];
        tty_names_i += copied;

        pid = exec_cmd(&cmd, stack);
        if (pid < 0) {
            write_stderr("[myinit] failed to exec shell on tty ");
            write(2, tty_name, strlen(tty_name));
            write_stderr("\n");
            return -1;
        }
        pmap->pid = pid;
        slot++;
    }
    pidttymap[slot].pid = 0;
    pidttymap[slot].tty_name = NULL;

    while (1) {
        ret = waitid(P_ALL, 0, &siginfo, WEXITED, NULL);
        check_syscall("waitid child");
        for (uint64_t i = 0; i < slot; i++) {
            subprocess_t *pmap = &pidttymap[i];

            if (pmap->pid != siginfo.si_pid) {
                continue;
            }
            write_stderr("[myinit] subprocess exited, tty: ");
            write(2, pmap->tty_name, strlen(pmap->tty_name));
            write_stderr("\n");

            cmd.tty_name = pmap->tty_name;
            pid = exec_cmd(&cmd, stack);
            if (pid < 0) {
                write_stderr("[myinit] failed to exec shell on tty ");
                write(2, pmap->tty_name, strlen(pmap->tty_name));
                write_stderr("\n");
                continue;
            }
            pmap->pid = pid;
            break;
        }
        nanosleep(&(struct timespec){.tv_sec = 1, .tv_nsec = 0}, NULL);
    }

    // never here
    abort("[myinit] unreachable code\n");
}

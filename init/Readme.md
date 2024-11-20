# simple init for rescue use

# routine

1. try run /init.pre.sh
2. for shell in /init.shell.sh bash ash sh, test it exists
3. fork, set session, set up console, run shell
4. if shell exits, sleep 1s, goto 3

when SIGINT'd, exit. SIGTERM for reboot, SIGUSR2 for halt.

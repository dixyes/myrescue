
CFLAGS := -Wall -Wextra -Werror
CFLAGS += -Os -g
CFLAGS += -static-pie -ffreestanding -fno-stack-protector -fno-builtin
CFLAGS += -Ilibc
CFLAGS_MAIN := -nodefaultlibs -nostartfiles -nostdlib -e _start

all: init

libc:
	$(MAKE) -C libc CC=$(CC)

main.o: main.c libc/defs.h libc/syscalldef.h
	$(CC) $(CFLAGS) -c -o $@ main.c

init: main.o libc
	$(CC) $(CFLAGS) $(CFLAGS_MAIN) -o $@ main.o libc/libc.a

clean:
	rm -f init *.o
	$(MAKE) -C libc clean

.PHONY: clean all libc

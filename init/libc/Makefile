
CFLAGS := -Wall -Wextra -Werror
CFLAGS += -Os -g
CFLAGS += -static-pie -ffreestanding -fno-stack-protector -fno-builtin
ARCH := $(shell uname -m)

all: libc.a

init.$(ARCH).o: init.$(ARCH).S syscalldef.h
	$(CC) $(CFLAGS) -c -o $@ init.$(ARCH).S

lib.o: lib.c defs.h syscalldef.h
	$(CC) $(CFLAGS) -c -o $@ lib.c

libc.a: lib.o init.$(ARCH).o
	$(AR) rcs $@ $^

clean:
	rm -f *.o *.a

.PHONY: clean all

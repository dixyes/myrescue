
CFLAGS := -Wall -Wextra -Werror
CFLAGS += -Os -g
CFLAGS += -static-pie -ffreestanding -fno-stack-protector -fno-builtin
ARCH := $(shell uname -m)

all: libc.a

lib.o: lib.c
	$(CC) $(CFLAGS) -c -o $@ $<

libc.a: lib.o init.$(ARCH).o
	$(AR) rcs $@ $^

clean:
	rm -f *.o *.a

.PHONY: clean all


CFLAGS += -static-pie -nodefaultlibs -nostartfiles -nostdlib -ffreestanding -fno-stack-protector -Ilibc
CFLAGS_MAIN := -e _start

all: init

libc:
	$(MAKE) -C libc

init: main.o libc/libc.a
	$(CC) $(CFLAGS) $(CFLAGS_MAIN) -o $@ $^

clean:
	rm -f init init.o
	$(MAKE) -C libc clean

.PHONY: clean all libc

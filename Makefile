SRCS = \
	src/entry.S\
	src/start.c\
	src/user.S\
	src/libs/libfuncs.c\
	src/libs/ds.c\
	src/libs/printf.c\
	src/libs/time.c\
	src/memory/memory.c\
	src/memory/malloc.c\
	src/trap/trap.c\
	src/trap/interrupt.S

OBJS = \
	src/entry.o\
	src/start.o\
	src/user.o\
	src/libs/libfuncs.o\
	src/libs/ds.o\
	src/libs/printf.o\
	src/libs/syscall.o\
	src/libs/time.o\
	src/memory/memory.o\
	src/memory/malloc.o\
	src/trap/trap.o\
	src/trap/interrupt.o\
	src/proc/proc.o

USER_LIB_OBJS = \
	user/libs/printf.o\
	user/libs/syscall.o

USER_OBJS = \
	user/root_proc.o

# Try to infer the correct TOOLPREFIX if not set
ifndef TOOLPREFIX
TOOLPREFIX := $(shell if riscv64-unknown-elf-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-unknown-elf-'; \
	elif riscv64-linux-gnu-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-linux-gnu-'; \
	elif riscv64-unknown-linux-gnu-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-unknown-linux-gnu-'; \
	else echo "***" 1>&2; \
	echo "*** Error: Couldn't find a riscv64 version of GCC/binutils." 1>&2; \
	echo "*** To turn off this error, run 'gmake TOOLPREFIX= ...'." 1>&2; \
	echo "***" 1>&2; exit 1; fi)
endif

QEMU = qemu-system-riscv64

CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)gas
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump

CFLAGS = -g -nostdlib -mcmodel=medany
LDFLAGS = -z max-page-size=4096

kernel: $(OBJS) src/kernel.ld
	$(LD) $(LDFLAGS) -T src/kernel.ld -o kernel $(OBJS) 
	$(OBJDUMP) -S kernel > kernel.asm
	$(OBJDUMP) -t kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > kernel.sym

src/user.o: src/user.S $(USER_OBJS)
	$(CC) -c $(CFLAGS) src/user.S -o src/user.o

# include .depend
# depend: .depend

# .depend: $(SRCS)
# 	rm -f ./.depend
# 	$(CC) -MM $(SRCS) > ./.depend;

user/root_proc.o: $(USER_LIB_OBJS) user/entry.o user/root_proc.c user/user.ld
	$(CC) -c -nostdlib -mcmodel=medany user/root_proc.c -o user/root_proc.tmp
	$(LD) $(LDFLAGS) -T user/user.ld -o user/root_proc.o  $(USER_LIB_OBJS) user/entry.o user/root_proc.tmp
	rm user/root_proc.tmp

# test: user/entry.o user/test.o user/user.ld
# 	$(LD) $(LDFLAGS) -T user/user.ld -o user/test user/entry.o user/test.o
# 	$(OBJDUMP) -S user/test > user/test.asm
# 	$(OBJDUMP) -t user/test | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > user/test.sym

clean:
	rm -f $(OBJS)
	rm -f $(USER_LIB_OBJS)
	rm -f $(USER_OBJS)
	rm -f src/kernel
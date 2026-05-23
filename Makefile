CC      := x86_64-elf-gcc
LD      := x86_64-elf-ld

ifeq (, $(shell which $(CC) 2>/dev/null))
  CC := gcc
  LD := ld
endif

ifeq ($(wildcard ../zirvlibc/include/unistd.h),)
  LIBC_DIR := ../libs/zirvlibc
else
  LIBC_DIR := ../zirvlibc
endif

CFLAGS := \
    -std=c11 \
    -ffreestanding \
    -fno-stack-protector \
    -fno-pic \
    -mno-red-zone \
    -mno-mmx -mno-sse -mno-sse2 \
    -Wall -Wextra -O2 \
    -I$(LIBC_DIR)/include

LDFLAGS := \
    -nostdlib \
    -static \
    -no-pie \
    -z max-page-size=0x1000

LIBC_SRCS := \
    $(LIBC_DIR)/src/string.c \
    $(LIBC_DIR)/src/ctype.c \
    $(LIBC_DIR)/src/stdio.c \
    $(LIBC_DIR)/src/stdlib.c \
    $(LIBC_DIR)/src/unistd.c \
    $(LIBC_DIR)/src/syscall.c \
    $(LIBC_DIR)/src/datetime.c

LIBC_BUILD := build/libc
LIBC_OBJS := $(patsubst $(LIBC_DIR)/src/%.c,$(LIBC_BUILD)/%.o,$(LIBC_SRCS))

CRT0 := src/crt0.o

UTILS := hello cat sysinfo clear echo reboot shutdown suspend poweroff ping sleep true false yes uname hostname ifconfig lspci nokia
UTIL_ELFS := $(addsuffix .elf,$(UTILS))
UTIL_OBJS := $(addprefix src/,$(addsuffix .o,$(UTILS)))

.PHONY: all clean

all: $(UTIL_ELFS)

$(LIBC_OBJS): $(LIBC_BUILD)/%.o: $(LIBC_DIR)/src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

src/crt0.o: src/crt0.asm
	nasm -f elf64 -o $@ $<

%.elf: src/%.o $(CRT0) $(LIBC_OBJS)
	$(LD) $(LDFLAGS) -o $@ $< $(CRT0) $(LIBC_OBJS)

clean:
	rm -f $(UTIL_ELFS) $(CRT0) $(UTIL_OBJS)
	rm -rf $(LIBC_BUILD)

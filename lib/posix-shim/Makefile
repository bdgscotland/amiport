# Makefile for amiport POSIX shim library
#
# Cross-compiles using m68k-amigaos-gcc (bebbo toolchain)
# Produces libamiport.a

CC = m68k-amigaos-gcc
AR = m68k-amigaos-ar
RANLIB = m68k-amigaos-ranlib

CFLAGS = -O2 -noixemul -m68020 -Wall -Iinclude
TARGET = libamiport.a

SRCDIR = src
SRCS = $(SRCDIR)/file_io.c \
       $(SRCDIR)/dir_ops.c \
       $(SRCDIR)/getopt.c \
       $(SRCDIR)/signal_emu.c \
       $(SRCDIR)/errno_map.c \
       $(SRCDIR)/time_funcs.c \
       $(SRCDIR)/stat.c

OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $(OBJS)
	$(RANLIB) $@

$(SRCDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

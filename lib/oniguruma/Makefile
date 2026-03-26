# Makefile for Oniguruma regex library (AmigaOS 68k, ASCII-only build)
#
# Minimal build: core engine + ASCII + UTF-8 encodings.
# No Unicode property tables, no threading, no JIT.
# Produces liboniguruma.a

# Use wrapper scripts that auto-detect Docker vs native toolchain
TOOLCHAIN_BIN = ../../toolchain/scripts
CC = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
AR = $(TOOLCHAIN_BIN)/m68k-amigaos-ar
RANLIB = $(TOOLCHAIN_BIN)/m68k-amigaos-ranlib

# -O0 for safety on 68k — -O2 causes regex failures on real AmigaOS
# (vamos works but FS-UAE/real hardware fails — likely GCC codegen bug)
# -m68000 for vamos compatibility
CFLAGS = -O0 -noixemul -m68000 -Wall -Wno-unused-parameter \
         -DHAVE_CONFIG_H -Isrc -Iinclude
TARGET = liboniguruma.a

SRCDIR = src
SRCS = $(SRCDIR)/regcomp.c \
       $(SRCDIR)/regexec.c \
       $(SRCDIR)/regparse.c \
       $(SRCDIR)/regenc.c \
       $(SRCDIR)/regerror.c \
       $(SRCDIR)/regsyntax.c \
       $(SRCDIR)/regtrav.c \
       $(SRCDIR)/regversion.c \
       $(SRCDIR)/st.c \
       $(SRCDIR)/regext.c \
       $(SRCDIR)/ascii.c \
       $(SRCDIR)/utf8.c \
       $(SRCDIR)/unicode_stubs.c

OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $(OBJS)
	$(RANLIB) $@

$(SRCDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean

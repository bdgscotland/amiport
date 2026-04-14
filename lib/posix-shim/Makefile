# Makefile for amiport POSIX shim library
#
# Cross-compiles using m68k-amigaos-gcc (bebbo toolchain)
# Produces libamiport.a

# Use wrapper scripts that auto-detect Docker vs native toolchain
TOOLCHAIN_BIN = ../../toolchain/scripts
CC = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
AR = $(TOOLCHAIN_BIN)/m68k-amigaos-ar
RANLIB = $(TOOLCHAIN_BIN)/m68k-amigaos-ranlib

CFLAGS = -O2 -noixemul -m68000 -Wall -Iinclude

# ----------------------------------------------------------------------
# Dual-flavor 68020 build (Track A, 2026-04-14)
#
# Mirrors the pattern from lib/zlib/Makefile + lib/libgit2/Makefile
# (commits 4a8a362 + successor). -m68000 -> -m68020 via $(subst) so
# any future CFLAGS change propagates to the 020 variant automatically.
#
# Unlike zlib and libgit2, posix-shim is built at -O2 because these
# shims don't return structs by value and the crash-patterns #16 risk
# does not apply. Same -O2 in both flavors -- the 020 win is CPU-
# instruction-level, not optimizer-level.
#
# libamiport.a (000) stays the default for every existing port and
# the vamos test suite. libamiport-020.a is opt-in for amigit-020 and
# any future 68020+ consumer.
# ----------------------------------------------------------------------
CFLAGS_020 = $(subst -m68000,-m68020,$(CFLAGS))

TARGET     = libamiport.a
TARGET_020 = libamiport-020.a

SRCDIR = src
SRCS = $(SRCDIR)/file_io.c \
       $(SRCDIR)/dir_ops.c \
       $(SRCDIR)/getopt.c \
       $(SRCDIR)/signal_emu.c \
       $(SRCDIR)/errno_map.c \
       $(SRCDIR)/time_funcs.c \
       $(SRCDIR)/stat.c \
       $(SRCDIR)/process.c \
       $(SRCDIR)/fnmatch.c \
       $(SRCDIR)/scandir.c \
       $(SRCDIR)/string_bsd.c \
       $(SRCDIR)/stdio_ext.c \
       $(SRCDIR)/glob.c \
       $(SRCDIR)/argv_expand.c \
       $(SRCDIR)/termios.c \
       $(SRCDIR)/usergroup.c \
       $(SRCDIR)/sysinfo.c \
       $(SRCDIR)/fts.c

OBJS     = $(SRCS:.c=.o)
OBJS_020 = $(SRCS:.c=.020.o)

# Profiler object -- built separately with -DAMIPORT_PROFILE
PROF_SRC = $(SRCDIR)/profile.c
PROF_OBJ = $(SRCDIR)/profile.o

.PHONY: all all-020 dual clean

# 'all' still builds only the 000 flavor (+ profiler object), preserving
# existing behaviour. 020 is opt-in via 'make all-020' or 'make dual'.
all:     $(TARGET) $(PROF_OBJ)
all-020: $(TARGET_020)
dual:    $(TARGET) $(TARGET_020) $(PROF_OBJ)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $(OBJS)
	$(RANLIB) $@

$(TARGET_020): $(OBJS_020)
	$(AR) rcs $@ $(OBJS_020)
	$(RANLIB) $@

$(PROF_OBJ): $(PROF_SRC)
	$(CC) $(CFLAGS) -DAMIPORT_PROFILE -c -o $@ $<

$(SRCDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(SRCDIR)/%.020.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS_020) -c -o $@ $<

clean:
	rm -f $(OBJS) $(OBJS_020) $(PROF_OBJ) $(TARGET) $(TARGET_020)

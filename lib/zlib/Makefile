# Makefile for zlib 1.3.1 (AmigaOS 68k via bebbo-gcc)
#
# Produces libz.a — the industry-standard DEFLATE / gzip / zlib compression
# library. Ported for amiport to support downstream ports that need
# compression (libgit2/amigit per PDR-010, plus future png/zip/gzip ports).
#
# Upstream: https://github.com/madler/zlib
# License: zlib License (BSD-like)

TOOLCHAIN_BIN = ../../toolchain/scripts
CC = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
AR = $(TOOLCHAIN_BIN)/m68k-amigaos-ar
RANLIB = $(TOOLCHAIN_BIN)/m68k-amigaos-ranlib

# -O0 default for safety on 68k -- bebbo-gcc 6.5.0b has codegen bugs at
# -O1/-O2 for struct-by-value returns > 8 bytes (crash-patterns #16) and
# various obscure interactions (lib/oniguruma/ lost a day to this).
# -m68000 because vamos emulates 68000; libraries must not contain
# 68020+ instructions or tests crash with ALERT: code=00068020
#
# Per-file -O1 below for three hot-path files (inffast, adler32, crc32)
# -- perf-optimizer verdict: no struct-by-value returns in these files,
# safe for -O1, ~2-3x speedup on the libgit2 inflate path. Test suite
# must re-pass on FS-UAE after any change to these rules.
#
# -DNO_DIVIDE replaces `% 65521` in adler32 with shift+subtract (CHOP).
# Eliminates ~1800 software divides per 10 MB buffer. Zero-risk win.
CFLAGS = -O0 -noixemul -m68000 -Wall -Wno-unused-parameter \
         -Iinclude -Isrc \
         -DHAVE_HIDDEN \
         -DHAVE_UNISTD_H \
         -DNO_DIVIDE

# Per-file -O1 for hot-path files. Scalar-only, no struct returns, audited
# safe by perf-optimizer (2026-04-12).
HOTPATH_CFLAGS = $(CFLAGS) -O1

TARGET = libz.a

SRCDIR = src

# Core compression engine (mandatory, no stdio dependencies)
CORE_SRCS = \
    $(SRCDIR)/adler32.c \
    $(SRCDIR)/compress.c \
    $(SRCDIR)/crc32.c \
    $(SRCDIR)/deflate.c \
    $(SRCDIR)/infback.c \
    $(SRCDIR)/inffast.c \
    $(SRCDIR)/inflate.c \
    $(SRCDIR)/inftrees.c \
    $(SRCDIR)/trees.c \
    $(SRCDIR)/uncompr.c \
    $(SRCDIR)/zutil.c

# gzip file I/O layer (uses libnix open/read/write/lseek/close)
GZ_SRCS = \
    $(SRCDIR)/gzclose.c \
    $(SRCDIR)/gzlib.c \
    $(SRCDIR)/gzread.c \
    $(SRCDIR)/gzwrite.c

SRCS = $(CORE_SRCS) $(GZ_SRCS)
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $(OBJS)
	$(RANLIB) $@

$(SRCDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Hot-path files compiled at -O1. Order matters -- these override the
# pattern rule above. Any -O1-related regression should be isolated by
# reverting individual files to the pattern rule (which uses -O0).
$(SRCDIR)/inffast.o: $(SRCDIR)/inffast.c
	$(CC) $(HOTPATH_CFLAGS) -c $< -o $@

$(SRCDIR)/adler32.o: $(SRCDIR)/adler32.c
	$(CC) $(HOTPATH_CFLAGS) -c $< -o $@

$(SRCDIR)/crc32.o: $(SRCDIR)/crc32.c
	$(CC) $(HOTPATH_CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean

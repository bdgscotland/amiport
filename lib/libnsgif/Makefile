# lib/libnsgif/Makefile -- NetSurf GIF image decoder
#
# Upstream: https://github.com/netsurf-browser/libnsgif @ commit 5d5d750
# (v1.0.x). Copyright 2003-2023 NetSurf Browser Project, MIT-licensed
# (see COPYING).
#
# 2 .c files / 2689 LOC: gif.c (top-level decoder, ~2076 LOC) +
# lzw.c (LZW decompressor, ~613 LOC). Pure C99 with libnix Tier 1
# stdlib only (assert / inttypes / stdbool / stdint / stdlib / string).
# No POSIX surface, no float math, no static globals.
#
# Source-analyzer audit verdict: CLEAN (2026-05-02).
# - Zero `vsnprintf` calls
# - Zero `float` / `double` declarations
# - No threading, no filesystem / network calls
#
# Required by: ports/netsurf (Phase 1 final consumer).
#
# CPU TARGET: -m68040 -m68881 (NetSurf-Vampire dep stack convention; see
# lib/libwapcaplet/Makefile header for rationale).
#
# DEPENDS ON: nothing (only libc/libnix). Standalone library.

TOOLCHAIN_BIN = ../../toolchain/scripts
CC            = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
AR            = $(TOOLCHAIN_BIN)/m68k-amigaos-ar
RANLIB        = $(TOOLCHAIN_BIN)/m68k-amigaos-ranlib

# -O1 -fno-strict-aliasing whole-archive. Same convention as all 5 prior
# NetSurf-Vampire dep stack libs. The two TUs (gif.c + lzw.c) are both
# scalar-heavy decoders with no struct-by-value returns >8 bytes;
# bebbo-gcc 13.3 -O1 is stable.
CFLAGS  = -O1 -fno-strict-aliasing -noixemul -m68040 -m68881 -std=c99 -Wall -Wextra
CFLAGS += -DNDEBUG
CFLAGS += -Iinclude -Isrc

TARGET = libnsgif.a

OBJS = src/gif.o \
       src/lzw.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^
	$(RANLIB) $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean

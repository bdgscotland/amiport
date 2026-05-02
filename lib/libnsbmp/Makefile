# lib/libnsbmp/Makefile -- NetSurf BMP/ICO image decoder
#
# Upstream: https://github.com/netsurf-browser/libnsbmp @ commit ea063c9
# (v0.1.7). Copyright 2006 Richard Wilson + 2008 Sean Fox, MIT-licensed
# (see COPYING).
#
# Single TU 1388 LOC: BMP / ICO decoder with caller-supplied bitmap
# allocation callbacks. Pure C99 with libnix Tier 1 stdlib only
# (assert / stdbool / stddef / stdint / stdio / stdlib / string).
# No POSIX surface, no float math, no static globals.
#
# Source-analyzer audit verdict: CLEAN (2026-05-02).
# - Zero `vsnprintf` calls (the libnix NULL-buffer pitfall doesn't apply)
# - Zero `float` / `double` declarations (no soft-float pull)
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

# -O1 -fno-strict-aliasing whole-archive (single TU). Same convention as
# all 5 prior NetSurf-Vampire dep stack libs. Single-TU lib means no
# struct-by-value-return cross-TU concerns; bebbo-gcc 13.3 -O1 is stable
# for this workload.
CFLAGS  = -O1 -fno-strict-aliasing -noixemul -m68040 -m68881 -std=c99 -Wall -Wextra
CFLAGS += -DNDEBUG
CFLAGS += -Iinclude

TARGET = libnsbmp.a

OBJS = src/libnsbmp.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^
	$(RANLIB) $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean

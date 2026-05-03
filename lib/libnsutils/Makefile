# lib/libnsutils/Makefile -- NetSurf utility library
#
# Upstream: https://github.com/netsurf-browser/libnsutils @ commit 0bd3906,
# MIT-licensed (see COPYING). Copyright 2014-2022 Vincent Sanders + others.
#
# 3 .c files / ~600 LOC: time.c (monotonic ms via ReadEClock on Amiga),
# unistd.c (pread/pwrite emulation), base64.c (RFC4648 base64 + URL-safe).
# 6 public headers: time.h, unistd.h, base64.h, errors.h, endian.h
# (inline-only), assert.h (macro-only).
#
# Source-analyzer audit: CAVEATS -> CLEAN after applying one fix.
# - unistd.c: removed ftruncate fallback in nsu_pwrite (libnix has no
#   ftruncate; the past-EOF seek path is rarely exercised in NetSurf's
#   real use of pwrite). Documented in the source.
# - time.c: uses ReadEClock from <proto/timer.h>. Amiga branch fires
#   when __amiga is defined (bebbo-gcc DOES define __amiga + AMIGA +
#   __AMIGA__ etc.). Caller MUST open timer.device and provide
#   TimerBase global before invoking nsu_getmonotonic_ms() -- libnix
#   does NOT auto-open timer.device.
# - base64.c: pure C, no portability concerns.
#
# After the fix:
# - Zero `float` / `double` declarations (no soft-float pull)
# - No struct-by-value returns >8 bytes
# - Pure ASCII (no UTF-8 preprocessor trap)
# - Includes <proto/timer.h> for ReadEClock; pure libc/libnix otherwise
#
# CONSUMER REQUIREMENT: open timer.device + set TimerBase before
# calling nsu_getmonotonic_ms(). NetSurf does this as part of its
# normal Amiga init.
#
# Required by: ports/netsurf (Phase 1 final consumer).
#
# CPU TARGET: -m68040 -m68881 (NetSurf-Vampire dep stack convention; see
# lib/libwapcaplet/Makefile header for rationale).
#
# DEPENDS ON: nothing (only libc/libnix + dos.library/timer.device API
# headers that ship with bebbo-gcc).

TOOLCHAIN_BIN = ../../toolchain/scripts
CC            = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
AR            = $(TOOLCHAIN_BIN)/m68k-amigaos-ar
RANLIB        = $(TOOLCHAIN_BIN)/m68k-amigaos-ranlib

# -O1 -fno-strict-aliasing whole-archive. All 3 TUs are trivially
# -O1-safe per crash-patterns #16 (no struct-by-value returns >8 bytes,
# integer arithmetic only).
CFLAGS  = -O1 -fno-strict-aliasing -noixemul -m68040 -m68881 -std=c99
CFLAGS += -DNDEBUG -D_DEFAULT_SOURCE
CFLAGS += -Iinclude -Isrc

TARGET = libnsutils.a

OBJS  = src/time.o
OBJS += src/unistd.o
OBJS += src/base64.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^
	$(RANLIB) $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

src/time.o:   src/time.c   include/nsutils/time.h   include/nsutils/errors.h
src/unistd.o: src/unistd.c include/nsutils/unistd.h
src/base64.o: src/base64.c include/nsutils/base64.h include/nsutils/errors.h

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean

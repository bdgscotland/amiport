# lib/libnspsl/Makefile -- NetSurf Public Suffix List lookup
#
# Upstream: https://github.com/netsurf-browser/libnspsl @ commit 82815c2,
# MIT-licensed (see COPYING). Copyright 2016-2024 Vincent Sanders + others.
#
# 1 .c file (~208 LOC) + 1 pre-generated table file (~13K LOC of compressed
# Public Suffix List data via Huffman case-insensitive comparison). Single
# entry point: `const char *nspsl_getpublicsuffix(const char *hostname)`.
#
# The PSL data is generated from `public_suffix_list.dat` (vendored
# alongside) via the `src/genpubsuffix.pl` Perl tool. The output
# `src/psl.inc` is committed to the tree so the library-build does NOT
# depend on Perl at library-build time.
#
# To regenerate after refreshing the upstream PSL data:
#   wget -O public_suffix_list.dat https://publicsuffix.org/list/public_suffix_list.dat
#   perl src/genpubsuffix.pl public_suffix_list.dat > src/psl.inc
#
# Source-analyzer audit verdict: CLEAN.
# - Only uses <stdint.h> and <string.h> -- maximally freestanding.
# - Pure integer arithmetic on packed 32-bit pnode values (no float).
# - No struct-by-value returns >8 bytes.
# - All ASCII (no UTF-8 preprocessor trap).
# - No malloc/free anywhere -- all data is static const.
# - Returns pointer INTO the caller's input string (no allocation;
#   nothing for the caller to free).
#
# Required by: ports/netsurf (Phase 1 final consumer -- cookie domain
# scope checks, hyperlink target validation).
#
# CPU TARGET: -m68040 -m68881 (NetSurf-Vampire dep stack convention).
#
# DEPENDS ON: nothing. No NetSurf-internal deps; no libwapcaplet despite
# what the Phase D-prime planning notes assumed (the public API is just
# `const char *` in -> `const char *` out).

TOOLCHAIN_BIN = ../../toolchain/scripts
CC            = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
AR            = $(TOOLCHAIN_BIN)/m68k-amigaos-ar
RANLIB        = $(TOOLCHAIN_BIN)/m68k-amigaos-ranlib

# -O1 -fno-strict-aliasing whole-archive. Single TU is trivially
# -O1-safe per crash-patterns #16 (no struct returns, integer-only).
CFLAGS  = -O1 -fno-strict-aliasing -noixemul -m68040 -m68881 -std=c99
CFLAGS += -DNDEBUG -D_DEFAULT_SOURCE
CFLAGS += -Iinclude -Isrc

TARGET = libnspsl.a

OBJS = src/nspsl.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^
	$(RANLIB) $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

src/nspsl.o: src/nspsl.c src/psl.inc include/nspsl.h

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean

# lib/libnslog/Makefile -- NetSurf logging library
#
# Upstream: https://github.com/netsurf-browser/libnslog @ commit bedff21
# (v0.1.3). Copyright 2014-2017 Vincent Sanders + Daniel Silverstone,
# MIT-licensed (see COPYING).
#
# 2 hand-written TUs (core.c ~202 LOC, filter.c ~405 LOC) + 2 generated
# TUs (filter-lexer.c wrapper + filter-parser.c, total ~6000 LOC of
# flex/bison output). The .l / .y sources are vendored alongside for
# reproducibility but the .inc / .c / .h outputs are committed and used
# directly at library-build time -- no host flex/bison required.
#
# To regenerate the flex/bison outputs after upstream changes the .l/.y:
#
#     # On macOS host (uses Homebrew bison 3.x; system bison 2.3 is too old):
#     /opt/homebrew/opt/bison/bin/bison -d -t \
#         --define=api.prefix='{filter_}' --report=all \
#         --output=src/filter-parser.c --defines=src/filter-parser.h \
#         src/filter-parser.y
#     flex --outfile=src/filter-lexer.inc \
#          --header-file=src/filter-lexer.h \
#          src/filter-lexer.l
#
# Source-analyzer audit verdict: CAVEATS -> CLEAN after applying one fix
# at core.c:137 (vsnprintf NULL probe -> 1024-byte stack probe). See
# crash-patterns #5. Fix is in our copy of core.c; original at
# /tmp/libnslog-upstream/src/core.c (or upstream HEAD).
#
# After the fix:
# - Zero `float` / `double` declarations (no soft-float pull)
# - Zero struct-by-value returns >8 bytes (-O1 codegen safe per #16)
# - Zero POSIX surface beyond stdio/stdlib/string/assert/stdarg
#   (no signal, pthread, file I/O, network, clock_gettime, etc.)
# - Pure ASCII (no UTF-8 preprocessor trap)
#
# CONSUMER CLEANUP: call `nslog_cleanup()` before program exit. Frees
# all category names, the cork chain, and clears the active filter.
# AmigaOS `-noixemul` does NOT reclaim process memory at exit.
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

# -O1 -fno-strict-aliasing whole-archive. All 4 TUs cleared by Stage 7
# perf audit: hand-written TUs trivially -O1-safe (no struct returns >8
# bytes, no float math); generated flex/bison output is well-trodden
# code that compiles cleanly under -O1.
CFLAGS  = -O1 -fno-strict-aliasing -noixemul -m68040 -m68881 -std=c99
CFLAGS += -DNDEBUG -D_DEFAULT_SOURCE
CFLAGS += -Iinclude -Isrc
# Suppress flex/bison-generated noise (unused-function warnings on yyfree, etc.)
CFLAGS += -Wno-unused-function -Wno-unused-variable

TARGET = libnslog.a

OBJS  = src/core.o
OBJS += src/filter.o
OBJS += src/filter-lexer.o
OBJS += src/filter-parser.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^
	$(RANLIB) $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# filter-lexer.c is a 3-line wrapper that #includes filter-lexer.inc.
# It depends on filter-lexer.h + filter-lexer.inc (both pre-generated).
src/filter-lexer.o: src/filter-lexer.c src/filter-lexer.h src/filter-lexer.inc src/filter-parser.h

src/filter-parser.o: src/filter-parser.c src/filter-parser.h

src/core.o: src/core.c src/nslog_internal.h include/nslog/nslog.h

src/filter.o: src/filter.c src/nslog_internal.h src/filter-parser.h include/nslog/nslog.h

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean

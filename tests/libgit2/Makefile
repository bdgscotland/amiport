# Makefile for lib/libgit2 unit tests (PDR-010 Phase 2 Stage 5)
#
# Cross-compiles test_libgit2 for AmigaOS 68k and runs it via vamos.
# Usage: make          (build and run)
#        make build    (build only)
#        make run      (run only -- assumes already built)
#        make clean
#
# Link order is critical: libgit2 -> libz -> libamiport -> libnix (implicit).
# libgit2 depends on libz for loose object deflate/inflate; both depend on
# libamiport for the posix-shim additions (pread, realpath, utimes, etc.)
# force-injected into libgit2 via amigaos_compat.h.

TOOLCHAIN_BIN = ../../toolchain/scripts
CC = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc

# -O0 mandatory: matches libgit2.a build flags (crash-patterns #16 --
# bebbo-gcc 6.5.0b struct-by-value return corruption at -O1/-O2).
# -std=gnu99: libgit2 headers use C99 (bool, designated initializers).
# -m68000: vamos emulates 68000; 68020 instructions cause ALERT.
# -include amigaos_compat.h: reproduces libgit2.a's force-include so the
#   test TU sees the same macro namespace. Required even though
#   test_libgit2.c does not directly call pread/realpath/etc -- the
#   -include must resolve at link time for the libgit2 symbols to bind
#   correctly.
# AMIPORT_VAMOS_LIMITED: skips tests that depend on vamos's opendir/readdir/
# lstat for just-written files on the T: volume. vamos's directory enumeration
# does not reliably surface newly-written workdir files or newly-created refs,
# which fails libgit2's workdir iterator and status/ref listing. The affected
# tests remain compiled and available -- only the RUN_TEST() invocations are
# gated -- so a future FS-UAE library-test harness can exercise them by
# removing this flag. See tests/libgit2/PLAN.md for the full gate analysis.
CFLAGS = -std=gnu99 -O0 -noixemul -m68000 -Wall \
         -Wno-unused-parameter -Wno-unused-function \
         -DAMIPORT_VAMOS_LIMITED \
         -I../../lib/libgit2/include \
         -I../../lib/libgit2/src/util \
         -I../../lib/posix-shim/include \
         -include ../../lib/libgit2/src/util/amigaos_compat.h \
         -I../shim

LDFLAGS = -L../../lib/libgit2 -lgit2 \
          -L../../lib/zlib -lz \
          -L../../lib/posix-shim -lamiport \
          -lm

# vamos default stack is 8 KB -- libgit2 tree/pack walk is deeply recursive.
# Pass 256 KB via -s. Real AmigaOS reads the __stack cookie (declared in
# test_libgit2.c); vamos ignores it and honors this flag.
#
# vamos default RAM is 1 MB, insufficient for the 1.1 MB test binary +
# 256 KB stack + libgit2 static-alloc workspace. Pass -m 4096 (4 MB).
# Also requires the local amitools monkey-patch raising the DOS
# LockManager ceiling from 1024 to 65536 -- see tests/libgit2/PLAN.md
# for context.
VAMOS_STACK = 256
VAMOS_MEM   = 4096
VAMOS = vamos -q -s $(VAMOS_STACK) -m $(VAMOS_MEM)

LIBGIT2_DIR = ../../lib/libgit2
ZLIB_DIR    = ../../lib/zlib
SHIM_DIR    = ../../lib/posix-shim

.PHONY: all build run clean

all: build run

build: $(LIBGIT2_DIR)/libgit2.a $(ZLIB_DIR)/libz.a $(SHIM_DIR)/libamiport.a test_libgit2

$(LIBGIT2_DIR)/libgit2.a:
	$(MAKE) -C $(LIBGIT2_DIR)

$(ZLIB_DIR)/libz.a:
	$(MAKE) -C $(ZLIB_DIR)

$(SHIM_DIR)/libamiport.a:
	$(MAKE) -C $(SHIM_DIR)

test_libgit2: test_libgit2.c ../shim/test_framework.h
	$(CC) $(CFLAGS) -o $@ test_libgit2.c $(LDFLAGS)

run: test_libgit2
	$(VAMOS) ./test_libgit2

clean:
	rm -f test_libgit2

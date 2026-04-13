# Makefile for lib/zlib unit tests
#
# Cross-compiles test_zlib for AmigaOS 68k and runs it via vamos.
# Usage: make          (build and run)
#        make build    (build only)
#        make run      (run only — assumes already built)
#        make clean

TOOLCHAIN_BIN = ../../toolchain/scripts
CC = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc

# -O0 to match libz.a. bebbo-gcc -O2 on 68k produced an illegal-instruction
# trap in vamos for this test harness (observed 2026-04-12). Matches the
# general rule in known-pitfalls: default bundled-library code to -O0 until
# proven safe, same as lib/oniguruma/.
CFLAGS = -O0 -noixemul -m68000 -Wall \
         -I../../lib/zlib/include \
         -I../shim

LDFLAGS = -L../../lib/zlib -lz

# vamos default stack is 8 KB (crash-patterns: vamos __stack cookie not read
# at runtime). This test has ~16 KB of locals per deep frame -- must pass -s.
VAMOS_STACK = 256
VAMOS = vamos -q -s $(VAMOS_STACK)

ZLIB_DIR = ../../lib/zlib

.PHONY: all build run clean

all: build run

build: $(ZLIB_DIR)/libz.a test_zlib

$(ZLIB_DIR)/libz.a:
	$(MAKE) -C $(ZLIB_DIR)

test_zlib: test_zlib.c ../shim/test_framework.h $(ZLIB_DIR)/libz.a
	$(CC) $(CFLAGS) -o $@ test_zlib.c $(LDFLAGS)

run: test_zlib
	$(VAMOS) ./test_zlib

clean:
	rm -f test_zlib

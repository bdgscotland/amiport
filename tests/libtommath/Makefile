# LibTomMath unit tests
#
# Run via: make -C tests/libtommath
# Or: make test-libtommath (from project root, once added to top-level Makefile)

TOOLCHAIN_BIN = ../../toolchain/scripts
CC = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc

CFLAGS = -O0 -noixemul -m68020 -std=gnu99 -Wall \
         -I../../lib/libtommath/include \
         -I../shim

LDFLAGS = -L../../lib/libtommath -ltommath -noixemul

VAMOS_STACK = 256
VAMOS_CPU = 68020
VAMOS = vamos -q -C $(VAMOS_CPU) -s $(VAMOS_STACK)

LIBTOMMATH_DIR = ../../lib/libtommath

.PHONY: all build run clean

all: build run

build: $(LIBTOMMATH_DIR)/libtommath.a test_libtommath

$(LIBTOMMATH_DIR)/libtommath.a:
	$(MAKE) -C $(LIBTOMMATH_DIR)

test_libtommath: test_libtommath.c ../shim/test_framework.h $(LIBTOMMATH_DIR)/libtommath.a
	$(CC) $(CFLAGS) -o $@ test_libtommath.c $(LDFLAGS)

run: test_libtommath
	$(VAMOS) ./test_libtommath

clean:
	rm -f test_libtommath *.o

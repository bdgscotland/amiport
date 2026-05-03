# tests/libpng/Makefile -- unit tests for lib/libpng
#
# libpng + zlib link order: -lpng -lz. libpng calls into zlib for IDAT
# inflate/deflate.
#
# Cookies: 256 KB stack/MEMORY_STEP (standalone-lib class).

TOOLCHAIN_BIN = ../../toolchain/scripts
CC            = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
VAMOS         = vamos

CFLAGS  = -O0 -noixemul -m68040 -m68881 -std=gnu99 -Wall -Wextra
CFLAGS += -I../../lib/libpng/include
CFLAGS += -I../../lib/libpng/src
CFLAGS += -I../../lib/zlib/include
CFLAGS += -I../shim

LDFLAGS = -noixemul -m68040 -m68881
LIBS    = ../../lib/libpng/libpng.a ../../lib/zlib/libz.a

TARGET = test_libpng
VAMOS_STACK = 1024
VAMOS_CPU   = 68040
VAMOS_MEM   = 4096

all: $(TARGET)

$(TARGET): test_libpng.c $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ test_libpng.c $(LIBS)

run: $(TARGET)
	$(VAMOS) -C $(VAMOS_CPU) -s $(VAMOS_STACK) -m $(VAMOS_MEM) ./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean

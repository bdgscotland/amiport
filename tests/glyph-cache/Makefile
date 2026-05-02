# tests/glyph-cache/Makefile -- unit tests for lib/glyph-cache

TOOLCHAIN_BIN = ../../toolchain/scripts
CC            = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
AR            = $(TOOLCHAIN_BIN)/m68k-amigaos-ar
VAMOS         = vamos

CFLAGS  = -O0 -noixemul -m68000 -std=gnu99 -Wall -Wextra
CFLAGS += -I../../lib/glyph-cache/include
CFLAGS += -I../shim

LDFLAGS = -noixemul -m68000
LIBS    = ../../lib/glyph-cache/libglyphcache.a

TARGET       = test_glyph_cache
VAMOS_STACK  = 256

all: $(TARGET)

$(TARGET): test_glyph_cache.c ../../lib/glyph-cache/libglyphcache.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ test_glyph_cache.c $(LIBS)

run: $(TARGET)
	$(VAMOS) -s $(VAMOS_STACK) ./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean

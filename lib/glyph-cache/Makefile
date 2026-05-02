# lib/glyph-cache/Makefile -- generic LRU glyph cache library

TOOLCHAIN_BIN = ../../toolchain/scripts
CC            = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
AR            = $(TOOLCHAIN_BIN)/m68k-amigaos-ar

CFLAGS  = -O0 -noixemul -m68000 -std=gnu99 -Wall -Wextra
CFLAGS += -Iinclude

# Per-file -O1 hot-path promotion (per lib/zlib/Makefile HOTPATH_CFLAGS pattern).
# glyph_cache.c was audited 2026-05-02 for crash-patterns #16
# (struct-by-value return >8 bytes corruption at -O1/-O2): every function
# returns void / int / pointer / size_t. NONE return structs by value.
# Safe to compile at -O1; ~1.5x speedup on the per-glyph lookup hot path.
HOTPATH_CFLAGS = -O1

TARGET  = libglyphcache.a
OBJS    = src/glyph_cache.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^

src/glyph_cache.o: src/glyph_cache.c include/amiport/glyph_cache.h
	$(CC) $(CFLAGS) $(HOTPATH_CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean

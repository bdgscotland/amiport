# lib/glyph-cache/Makefile -- generic LRU glyph cache library

TOOLCHAIN_BIN = ../../toolchain/scripts
CC            = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
AR            = $(TOOLCHAIN_BIN)/m68k-amigaos-ar

CFLAGS  = -O0 -noixemul -m68000 -std=gnu99 -Wall -Wextra
CFLAGS += -Iinclude

TARGET  = libglyphcache.a
OBJS    = src/glyph_cache.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^

%.o: %.c include/amiport/glyph_cache.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean

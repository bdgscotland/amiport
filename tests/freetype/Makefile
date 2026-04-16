TOOLCHAIN_BIN = ../../toolchain/scripts
CC = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc

CFLAGS = -O0 -noixemul -m68000 -std=gnu99 -Wall -Wno-unused-parameter \
         -I../../lib/freetype/include \
         -I../shim

LDFLAGS = -L../../lib/freetype -lfreetype -lm

VAMOS_STACK = 256
VAMOS = vamos -q -s $(VAMOS_STACK) -m 4096

FREETYPE_DIR = ../../lib/freetype

.PHONY: all build run clean

all: build run

build: $(FREETYPE_DIR)/libfreetype.a test_freetype

$(FREETYPE_DIR)/libfreetype.a:
	$(MAKE) -C $(FREETYPE_DIR)

test_freetype: test_freetype.c ../shim/test_framework.h \
               $(FREETYPE_DIR)/libfreetype.a
	$(CC) $(CFLAGS) -o $@ test_freetype.c $(LDFLAGS)

run: test_freetype
	$(VAMOS) ./test_freetype

clean:
	rm -f test_freetype


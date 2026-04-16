TOOLCHAIN_BIN = ../../toolchain/scripts
CC = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc

CFLAGS = -O0 -noixemul -m68000 -std=gnu99 -Wall -Wno-unused-parameter \
         -I../../lib/freetype/include \
         -I../shim

LDFLAGS = -L../../lib/freetype -lfreetype -lm

VAMOS_STACK = 256
VAMOS = vamos -q -s $(VAMOS_STACK) -m 4096

FREETYPE_DIR = ../../lib/freetype

.PHONY: all build run demo clean

all: build run

build: $(FREETYPE_DIR)/libfreetype.a test_freetype render_demo

$(FREETYPE_DIR)/libfreetype.a:
	$(MAKE) -C $(FREETYPE_DIR)

test_freetype: test_freetype.c ../shim/test_framework.h \
               $(FREETYPE_DIR)/libfreetype.a
	$(CC) $(CFLAGS) -o $@ test_freetype.c $(LDFLAGS)

# Visual rasterization proof -- renders the letter 'A' at 20px and
# dumps the AA bitmap as ASCII art. Empirically validates the -O1
# hot-path promotions (smooth.c, truetype.c, ftbitmap.c) produce
# correct output, not just "buffer != NULL". Run with 'make demo'.
render_demo: render_demo.c $(FREETYPE_DIR)/libfreetype.a
	$(CC) $(CFLAGS) -o $@ render_demo.c $(LDFLAGS)

run: test_freetype
	$(VAMOS) ./test_freetype

demo: render_demo
	$(VAMOS) ./render_demo

clean:
	rm -f test_freetype render_demo


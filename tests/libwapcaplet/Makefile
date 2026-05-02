# tests/libwapcaplet/Makefile -- unit tests for lib/libwapcaplet
#
# Tests run on vamos with -C 68040 because the library is built
# -m68040 -m68881 (matches the planned ports/netsurf consumer ABI).
# If vamos balks at FPU instructions in this combination, fall back
# to FS-UAE A3000+040 (per .claude/rules/library-pipeline.md
# "Choosing vamos vs FS-UAE" guidance).

TOOLCHAIN_BIN = ../../toolchain/scripts
CC            = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
VAMOS         = vamos

CFLAGS  = -O0 -noixemul -m68040 -m68881 -std=gnu99 -Wall -Wextra
CFLAGS += -I../../lib/libwapcaplet/include
CFLAGS += -I../shim

LDFLAGS = -noixemul -m68040 -m68881
LIBS    = ../../lib/libwapcaplet/libwapcaplet.a

TARGET       = test_libwapcaplet
VAMOS_STACK  = 1024
VAMOS_CPU    = 68040
VAMOS_MEM    = 4096

all: $(TARGET)

$(TARGET): test_libwapcaplet.c ../../lib/libwapcaplet/libwapcaplet.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ test_libwapcaplet.c $(LIBS)

run: $(TARGET)
	$(VAMOS) -C $(VAMOS_CPU) -s $(VAMOS_STACK) -m $(VAMOS_MEM) ./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean

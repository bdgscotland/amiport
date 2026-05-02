# tests/libnsbmp/Makefile -- unit tests for lib/libnsbmp
#
# libnsbmp is a single-TU standalone library. No NetSurf-internal deps.
# Cookies: 256 KB stack/MEMORY_STEP (libwapcaplet-class).

TOOLCHAIN_BIN = ../../toolchain/scripts
CC            = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
VAMOS         = vamos

CFLAGS  = -O0 -noixemul -m68040 -m68881 -std=gnu99 -Wall -Wextra
CFLAGS += -I../../lib/libnsbmp/include
CFLAGS += -I../shim

LDFLAGS = -noixemul -m68040 -m68881
LIBS    = ../../lib/libnsbmp/libnsbmp.a

TARGET = test_libnsbmp
VAMOS_STACK = 1024
VAMOS_CPU   = 68040
VAMOS_MEM   = 4096

all: $(TARGET)

$(TARGET): test_libnsbmp.c $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ test_libnsbmp.c $(LIBS)

run: $(TARGET)
	$(VAMOS) -C $(VAMOS_CPU) -s $(VAMOS_STACK) -m $(VAMOS_MEM) ./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean

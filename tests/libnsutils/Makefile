# tests/libnsutils/Makefile -- unit tests for lib/libnsutils
#
# libnsutils is a small standalone library (3 TUs, 3 KB archive). Tests
# need timer.device opened explicitly because nsu_getmonotonic_ms uses
# ReadEClock; libnix does NOT auto-open timer.device.
#
# Cookies: 256 KB stack/MEMORY_STEP (libnsbmp/libnsgif/libnslog class).

TOOLCHAIN_BIN = ../../toolchain/scripts
CC            = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
VAMOS         = vamos

CFLAGS  = -O0 -noixemul -m68040 -m68881 -std=gnu99 -Wall -Wextra
CFLAGS += -I../../lib/libnsutils/include
CFLAGS += -I../shim

LDFLAGS = -noixemul -m68040 -m68881
LIBS    = ../../lib/libnsutils/libnsutils.a

TARGET = test_libnsutils
VAMOS_STACK = 1024
VAMOS_CPU   = 68040
VAMOS_MEM   = 4096

all: $(TARGET)

$(TARGET): test_libnsutils.c $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ test_libnsutils.c $(LIBS)

run: $(TARGET)
	$(VAMOS) -C $(VAMOS_CPU) -s $(VAMOS_STACK) -m $(VAMOS_MEM) ./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean

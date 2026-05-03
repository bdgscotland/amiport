# tests/libnspsl/Makefile -- unit tests for lib/libnspsl
#
# libnspsl is a small standalone library (1 TU + ~13K LOC of generated
# static PSL data, 67 KB archive). No NetSurf-internal deps. Cookies:
# 256 KB stack/MEMORY_STEP (libnsbmp/libnsgif/libnslog/libnsutils class).

TOOLCHAIN_BIN = ../../toolchain/scripts
CC            = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
VAMOS         = vamos

CFLAGS  = -O0 -noixemul -m68040 -m68881 -std=gnu99 -Wall -Wextra
CFLAGS += -I../../lib/libnspsl/include
CFLAGS += -I../shim

LDFLAGS = -noixemul -m68040 -m68881
LIBS    = ../../lib/libnspsl/libnspsl.a

TARGET = test_libnspsl
VAMOS_STACK = 1024
VAMOS_CPU   = 68040
VAMOS_MEM   = 4096

all: $(TARGET)

$(TARGET): test_libnspsl.c $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ test_libnspsl.c $(LIBS)

run: $(TARGET)
	$(VAMOS) -C $(VAMOS_CPU) -s $(VAMOS_STACK) -m $(VAMOS_MEM) ./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean

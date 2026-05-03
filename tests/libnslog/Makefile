# tests/libnslog/Makefile -- unit tests for lib/libnslog
#
# libnslog is a small standalone library (4 TUs, 21 KB archive) with no
# NetSurf-internal deps. Cookies: 256 KB stack/MEMORY_STEP
# (libwapcaplet/libnsbmp/libnsgif class).
#
# `-std=gnu99` (NOT plain c99) because the NSLOG() macro uses GCC
# variadic macro syntax `args...`. Library itself builds clean with
# `-std=c99`; only the consumer needs `-std=gnu99` to invoke the macro.

TOOLCHAIN_BIN = ../../toolchain/scripts
CC            = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
VAMOS         = vamos

CFLAGS  = -O0 -noixemul -m68040 -m68881 -std=gnu99 -Wall -Wextra
CFLAGS += -I../../lib/libnslog/include
CFLAGS += -I../shim

LDFLAGS = -noixemul -m68040 -m68881
LIBS    = ../../lib/libnslog/libnslog.a

TARGET = test_libnslog
VAMOS_STACK = 1024
VAMOS_CPU   = 68040
VAMOS_MEM   = 4096

all: $(TARGET)

$(TARGET): test_libnslog.c $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ test_libnslog.c $(LIBS)

run: $(TARGET)
	$(VAMOS) -C $(VAMOS_CPU) -s $(VAMOS_STACK) -m $(VAMOS_MEM) ./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean

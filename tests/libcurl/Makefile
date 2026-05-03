# tests/libcurl/Makefile -- unit tests for lib/libcurl (HTTP-only build)
#
# libcurl needs socket symbols (IoctlSocket, socket, connect, gethostbyname).
# Tests do NOT actually fetch (vamos has no network). We provide stub
# implementations inline in test_libcurl.c so the link succeeds without
# pulling lib/bsdsocket-shim. Real-network tests defer to FS-UAE on real
# hardware where bsdsocket.library is available.
#
# Cookies: 1 MB stack/MEMORY_STEP per the libnix-stack-scaling pitfall.
# libcurl is large enough to push libnix's startup-allocation past 512 KB.

TOOLCHAIN_BIN = ../../toolchain/scripts
CC            = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
VAMOS         = vamos

CFLAGS  = -O0 -noixemul -m68040 -m68881 -std=gnu99 -Wall -Wextra
CFLAGS += -I../../lib/libcurl/include
CFLAGS += -I../shim

LDFLAGS = -noixemul -m68040 -m68881
LIBS    = ../../lib/libcurl/libcurl.a

TARGET = test_libcurl
VAMOS_STACK = 4096
VAMOS_CPU   = 68040
VAMOS_MEM   = 8192

all: $(TARGET)

$(TARGET): test_libcurl.c socket_stubs.c $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ test_libcurl.c socket_stubs.c $(LIBS)

run: $(TARGET)
	$(VAMOS) -C $(VAMOS_CPU) -s $(VAMOS_STACK) -m $(VAMOS_MEM) ./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean
